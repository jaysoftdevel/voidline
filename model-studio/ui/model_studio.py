#!/usr/bin/env python3
"""VOIDLINE Model Studio: desktop-native ttk/Tk Linux interface.

Only the drawing surface is a custom Canvas. Menus, controls, dialogs, tab panes,
project navigation and inspectors are native Tcl/Tk/ttk widgets.
The versioned C++ authoring backend remains responsible for source generation,
validation, game archive imports/exports and runtime model baking.
"""
from __future__ import annotations
import copy
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import threading
import tkinter as tk
from tkinter import ttk, filedialog
from editor_logic import (can_edit_joint, rig_status, run_cycles_per_second, run_phase,
                          hit_level_object, move_level_object, create_level_object,
                          delete_level_object, level_selected_anchor, edit_scalar_property, TOOL_KEYS,
                          ACTION_STATES, movement_sequences, ensure_sequences, create_model_from_template,
                          assign_model_to_slot, polygon_vertices, edit_structure_vertex,
                          add_structure_vertex, remove_structure_vertex,
                          copy_model_payload, paste_model_payload,
                          copy_movement_payload, paste_movement_payload,
                          discover_game_workspace, running_foot, running_hip_offset)
from studio_dialogs import StudioDialog, DIALOG_FONT

try:
    from PIL import Image, ImageTk
except ImportError:
    Image = ImageTk = None

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "build" / "voidline-editor-core"
if not CORE.exists():
    CORE = ROOT / "bin" / "linux" / "voidline-editor-core"
if not CORE.exists():
    CORE = ROOT.parent / "build" / "model-studio" / "voidline-editor-core"
VERSION = "0.0.23"
NAMES = ("pelvis", "waist", "neck", "head", "near shoulder", "far shoulder",
         "near elbow", "far elbow", "near hand", "far hand", "near knee", "far knee", "near ankle", "far ankle")
BASE = ((0,-33),(0,-41),(2,-49),(4,-59),(6,-46),(-5,-46),(14,-35),(-13,-35),
        (18,-28),(-16,-29),(8,-19),(-8,-19),(10,-5),(-10,-5))
DEPTH = (0,0,0,0,7,-7,8,-8,9,-9,6,-6,7,-7)
BONES = ((0,1),(1,2),(2,3),(2,4),(2,5),(4,6),(6,8),(5,7),(7,9),
         (0,10),(10,12),(0,11),(11,13))
POSES = ("base", "aim", "run")
ENEMIES = ("Drone", "Ranger", "Turret", "Stalker", "Guardian", "Scavenger",
           "Sniper", "Bulwark", "Interceptor", "Juggernaut")
PROPS = {"geometry": ("bodyHeight", "bodyWidth", "headScale", "armLength", "legLength", "weaponSeat"),
         "motion": ("walkCycle", "acceleration", "braking", "recoil", "idleBreath"),
         "skeleton": ("stride", "armSwing", "aimRaise")}
DEFAULTS = {"geometry": {k: (0.0 if k == "weaponSeat" else 1.0) for k in PROPS["geometry"]},
            "motion": {k: 1.0 for k in PROPS["motion"]}}
CHARACTER_FIELDS = ("health", "speed", "jump", "ammo", "rig", "color")
CHARACTER_METADATA = ("schema", "id", "name", "role", "art", "trait", "description")
ENEMY_METADATA = ("schema", "id", "name")
ENEMY_FIELDS = ("width", "height", "speed", "reach", "attackDelay", "health", "damage", "score")
PALETTE = {"ground":"#576579", "ledges":"#93a5b7", "cover":"#a97d59", "stairs":"#c0a1d7",
           "lifts":"#72add6", "vents":"#e5aa6f", "enemies":"#e9847a",
           "supplies":"#b9dc84", "crates":"#e4a77a", "beacons":"#84c9d8",
           "relays":"#edce89", "insertion":"#a6e9a4", "exit":"#6ee5c6"}


def prepare_motion(doc, catalog=None):
    """Import legacy enemy v1 and character rigs without destroying their statistics."""
    rig = doc.setdefault("skeleton", {})
    for pose in POSES:
        vals = rig.get(pose)
        if not isinstance(vals, list) or len(vals) != 14:
            rig[pose] = [[0.0, 0.0] for _ in NAMES]
    depths = rig.setdefault("depth", {})
    for pose in POSES:
        if not isinstance(depths.get(pose), list) or len(depths[pose]) != 14:
            depths[pose] = [0.0] * 14
    for k in PROPS["skeleton"]:
        rig.setdefault(k, 1.0)
    for group in ("geometry", "motion"):
        data = doc.setdefault(group, {})
        for k, value in DEFAULTS[group].items():
            data.setdefault(k, value)
    if doc.get("schema") == "voidline-enemy-v1":
        doc["schema"] = "voidline-enemy-v2"
    return doc


def pose_frame_index(frame):
    """Nearest keyframe on the editor timeline (five keys at 0,15,30,44,59)."""
    return max(0,min(4,round((int(frame)%60)/59*4)))


def pose_frame_offset(rig, pose, index, frame):
    """Interpolate exactly like game skeletonOffset using the authored XY keys."""
    frames=rig.get("poseFrames",{}).get(pose)
    if not isinstance(frames,list) or len(frames)!=5:return (0.0,0.0)
    value=max(0.,min(1.,(frame%60)/59.))*4
    first=min(4,int(value));last=min(4,first+1)
    alpha=value-first
    return tuple(frames[first][index][axis]*(1-alpha)+frames[last][index][axis]*alpha
                 for axis in range(2))


def active_layer(doc, pose, frame=None):
    rig=doc["skeleton"]
    if pose in POSES:return rig[pose],rig["depth"][pose]
    tracks=rig.get("poseFrames",{}).get(pose)
    if frame is not None and isinstance(tracks,list) and len(tracks)==5:
        # Manipulating an action edits its keyframe in the actual input JSON.
        return tracks[pose_frame_index(frame)],rig["sequenceDepth"][pose]
    return rig["sequences"][pose],rig["sequenceDepth"][pose]


def sample_joint(doc, idx, pose, frame, kind="character", speed_percent=100):
    d = prepare_motion(doc)
    rig = d["skeleton"]
    g = d["geometry"]
    x, y = BASE[idx]
    x *= float(g["bodyWidth"]); y *= float(g["bodyHeight"])
    if 6 <= idx <= 9:
        y = (-49 + (BASE[idx][1]+49)*float(g["armLength"])) * float(g["bodyHeight"])
    elif idx >= 10:
        y = (-33 + (BASE[idx][1]+33)*float(g["legLength"])) * float(g["bodyHeight"])
    elif idx == 3:
        y = (-49 + (BASE[idx][1]+49)*float(g["headScale"])) * float(g["bodyHeight"])
    x += rig["base"][idx][0]
    y += rig["base"][idx][1]
    z = DEPTH[idx] + rig["depth"]["base"][idx]
    if pose != "base":
        offsets,depth=active_layer(d,pose)
        x += offsets[idx][0];y += offsets[idx][1];z += depth[idx]
    tracks=d["skeleton"].get("poseFrames",{})
    authored_action=pose in tracks and len(tracks[pose])==5
    if authored_action:
        tx,ty=pose_frame_offset(rig,pose,idx,frame)
        x+=tx;y+=ty
    locomotion = pose in ("run","walk","sprint","patrol","chase","stairs")
    phase = run_phase(d, kind, frame, speed_percent) if locomotion else math.tau * (frame % 60) / 60
    if locomotion:
        # Full 12-stage cycle with alternating heel-strike, load, toe-off,
        # airborne recovery and forward knee drive. Source joint offsets are
        # applied above this block, never replaced by this procedural preview.
        phase01=phase/math.tau
        effort={"walk":.55,"patrol":.55,"stairs":.65,"run":1.,"sprint":1.12,"chase":1.12}.get(pose,1.)
        stance={"walk":.62,"patrol":.55,"stairs":.67,"run":.38,"sprint":.35,"chase":.38}.get(pose,.38)
        leg=0 if idx in (10,12) else 1
        foot_state=running_foot(phase01+(.5 if leg==0 else 0.),stance)
        planted,progress,clearance,_pitch=foot_state
        half_stride=15.*float(rig["stride"])*effort
        rel_x=(half_stride*(1.-2.*progress) if planted else
               half_stride*(-1.+2.*progress))
        lift=clearance*20.*effort
        # Articulated knees, solved from hip-to-foot geometry. The knee bends
        # toward travel; neither knee can snap backward through the thigh.
        if idx in (12,13):
            x+=rel_x-BASE[idx][0]
            y-=lift
        elif idx in (10,11):
            hip_x,hip_y=0.,-33.
            foot_x,foot_y=rel_x,-5.-lift
            dx,dy=foot_x-hip_x,foot_y-hip_y
            leg_distance=math.hypot(dx,dy)
            if leg_distance>29.9:
                ratio=29.9/leg_distance
                dx*=ratio;dy*=ratio
            d=max(.001,math.hypot(dx,dy))
            a=(16.*16.-15.*15.+d*d)/(2.*d)
            bend=math.sqrt(max(0.,16.*16.-a*a))
            knee_x=hip_x+dx/d*a+dy/d*bend
            knee_y=hip_y+dy/d*a-dx/d*bend
            x+=knee_x-BASE[idx][0]
            y+=knee_y-BASE[idx][1]
        elif idx in (6,8,7,9):
            # Arms counter-rotate with the opposite leg, with flexed elbows.
            counter=math.sin(phase01*math.tau)
            arm_sign=-1. if idx in (6,8) else 1.
            x+=9.*counter*arm_sign*float(rig["armSwing"])*effort
            if idx in (6,7):y-=max(0.,counter*arm_sign)*4.*effort
        elif idx in (0,1,2,3):
            y+=running_hip_offset(phase01)*effort
        if idx in (4,5): x+=.8*math.sin(phase)
    elif not authored_action and pose in ("aim","alert","fire","reload","interact","recoil"):
        # Visibly raise/lower both hands over the shot sequence and recoil in bursts.
        ready = min(1.0, frame / 12.0, max(0.0, (60-frame) / 12.0))
        recoil = max(0, math.sin(phase*4)) * float(d["motion"]["recoil"]) * (5.0 if pose in ("fire","recoil") else 2.6) * ready
        if idx in (8,9): x -= recoil; y -= ready*4.0*float(rig["aimRaise"])
        elif idx in (6,7): x -= recoil*.55; y -= ready*2.4
        elif idx in (4,5): x -= recoil*.3
        elif not authored_action and idx in (1,2,3): y += .65 * math.sin(phase)
    elif not authored_action and pose in ("jump","fall","land"):
        flex={"jump":-.9,"fall":.8,"land":1.2}[pose]
        if idx in (10,11):x += 4.5*flex
        if idx in (12,13):y -= 8*flex
        if idx in (0,1,2,3):y += 3*max(0,flex)
    elif not authored_action and pose in ("hit","stagger","death"):
        impact={"hit":2,"stagger":5,"death":10}[pose]
        if idx in (1,2,3): x -= impact * max(0,math.sin(phase/2))
        if idx in (8,9): y += impact*.6
    elif not authored_action and pose == "crouch":
        if idx in (0,1,2,3):y += 8
        if idx in (10,11):x += 4
    elif not authored_action and idx in (1,2,3):
        y += 2.2*math.sin(phase)*float(d["motion"]["idleBreath"])
    return x, y, z


class RigViewport(ttk.Frame):
    """The only individually drawn widget: model view plus mouse joint manipulators."""
    def __init__(self, master, app, axis):
        super().__init__(master)
        self.app, self.axis = app, axis
        self.yaw, self.pitch, self.scale = .72, .34, 2.6
        self.pan = [0.0,0.0]
        self.last = None
        self.drag_joint = None
        self.points = []
        ttk.Label(self, text={"front":"FRONT · XY", "side":"RIGHT · ZY", "top":"TOP · XZ", "orbit":"3D · Orbit"}[axis]).pack(anchor="w",padx=5,pady=(3,1))
        self.canvas = tk.Canvas(self, background="#111a29", highlightthickness=0, height=215, width=300)
        self.canvas.pack(fill="both",expand=True)
        self.canvas.bind("<Configure>", lambda _event:self.paint())
        self.canvas.bind("<ButtonPress-1>", self.start_drag)
        self.canvas.bind("<B1-Motion>", self.drag)
        self.canvas.bind("<ButtonRelease-1>",self.end_drag)
        self.canvas.bind("<ButtonPress-3>", lambda e: self.begin_camera(e,"orbit"))
        self.canvas.bind("<B3-Motion>", lambda e: self.camera_drag(e,"orbit"))
        self.canvas.bind("<ButtonPress-2>", lambda e: self.begin_camera(e,"pan"))
        self.canvas.bind("<B2-Motion>", lambda e: self.camera_drag(e,"pan"))
        self.canvas.bind("<MouseWheel>", self.zoom)
        self.canvas.bind("<Button-4>",lambda e:self.zoom_delta(1))
        self.canvas.bind("<Button-5>",lambda e:self.zoom_delta(-1))

    def axes(self):
        if self.axis == "front":return ((1,0,0),(0,1,0))
        if self.axis == "side":return ((0,0,1),(0,1,0))
        if self.axis == "top":return ((1,0,0),(0,0,1))
        cy, sy = math.cos(self.yaw), math.sin(self.yaw)
        cp, sp = math.cos(self.pitch), math.sin(self.pitch)
        return ((cy,0,sy),(-sy*sp,cp,cy*sp))

    def project(self, point):
        right,down=self.axes()
        w,h=self.canvas.winfo_width(),self.canvas.winfo_height()
        scale = min(w/108.,h/101.)*self.scale/2.6
        persp=1.0
        if self.axis=="orbit":
            forward=(right[1]*down[2]-right[2]*down[1],right[2]*down[0]-right[0]*down[2],right[0]*down[1]-right[1]*down[0])
            persp=max(.65,min(1.5,180/(180+sum(point[i]*forward[i] for i in range(3)))))
        return (w*.5+self.pan[0]+sum(point[i]*right[i] for i in range(3))*scale*persp,
                h*.69+self.pan[1]+sum(point[i]*down[i] for i in range(3))*scale*persp)

    def paint(self):
        canvas = self.canvas
        canvas.delete("all")
        if not self.app.model_document():
            canvas.create_text(18,22,anchor="nw",text="Select a character or enemy",fill="#b7c6d5")
            return
        w,h=canvas.winfo_width(),canvas.winfo_height()
        canvas.create_line(0,h*.69,w,h*.69,fill="#354559")
        canvas.create_line(w*.5,0,w*.5,h,fill="#28384c")
        pose=self.app.pose.get().lower()
        frame=self.app.preview_frame()
        if pose in ("run","walk","sprint","patrol","chase","stairs"):
            # Scroll ground markers at actual VOIDLINE running speed. The model
            # stays centered so joint handles remain usable while previewing.
            base_speed=float(self.app.doc.get("speed",0))*(1.28 if self.app.kind=="character" else 1.)
            effective_speed=base_speed*self.app.speed_percent.get()/100.
            markers=max(24.,min(160.,min(w/108.,h/101.)*self.scale/2.6*80.))
            distance=effective_speed*frame/30.0
            shift=(distance*markers/80.)%markers
            for j in range(-1,int(w/markers)+2):
                sx=j*markers-shift
                canvas.create_line(sx,h*.69-4,sx,h*.69+8,fill="#425a6f",width=2)
            cycles=run_cycles_per_second(self.app.doc,self.app.kind)*self.app.speed_percent.get()/100
            canvas.create_text(w-8,8,anchor="ne",fill="#b6e4bc",
                               text=f"{effective_speed:.0f} units/s · {cycles:.2f} gait cycles/s")
        if not can_edit_joint(self.app.kind,self.app.doc):
            self.paint_procedural(frame,pose)
            return
        joints=[sample_joint(self.app.doc,i,pose,frame,self.app.kind,self.app.speed_percent.get()) for i in range(14)]
        self.points=[self.project(point) for point in joints]
        accent="#e39e6e" if self.app.kind=="enemy" else "#70cfe4"
        if self.app.show_plates.get():
            for a,b in BONES:
                x0,y0=self.points[a];x1,y1=self.points[b]
                canvas.create_line(x0,y0,x1,y1,fill="#384d64",width=10,capstyle="round")
        for a,b in BONES:
            x0,y0=self.points[a];x1,y1=self.points[b]
            canvas.create_line(x0,y0,x1,y1,fill=accent,width=2)
        for i,(x,y) in enumerate(self.points):
            selected=i==self.app.joint_index.get()
            r=6 if selected else 4
            canvas.create_oval(x-r,y-r,x+r,y+r,outline="#edfaff" if selected else "#7ab1bf",
                               fill="#f1cc78" if selected else "#193447",width=2)
        canvas.create_text(8,h-10,anchor="sw",fill="#8ea6bc",text=f"{pose.upper()} · frame {int(frame)%60:02d}/59")

    def paint_procedural(self,frame,pose):
        """A schematic moving proxy, never falsely advertise editable joints.

        The game renders this archetype procedurally rather than from the
        humanoid skeleton. Body/properties still remain editable in inspector.
        """
        c=self.canvas
        w,h=c.winfo_width(),c.winfo_height()
        midx,midy=w*.5+self.pan[0],h*.60+self.pan[1]
        self.points=[]
        freq=run_cycles_per_second(self.app.doc,"enemy")
        modifier=self.app.speed_percent.get()/100 if pose in ("run","walk","sprint","patrol","chase","stairs") else 1.
        phase=math.tau*frame/30*freq*modifier
        move=math.sin(phase)*9 if pose in ("run","walk","sprint","patrol","chase","stairs") else math.sin(frame/30*2)*2
        role=int(self.app.doc.get("id",-1))
        scale=min(w/240.,h/200.)*self.scale/2.6
        color="#d8a878"
        if role in (0,8):
            # Rotor drones: central chassis, banking fins and visible rotor cycle.
            c.create_oval(midx-30*scale,midy-15*scale+move,midx+30*scale,midy+13*scale+move,
                          fill="#3b5366",outline="#7eb6db",width=2)
            for direction in (-1,1):
                a=midx+direction*47*scale
                c.create_line(midx+direction*25*scale,midy+move,a,midy-12*scale+move,fill=color,width=4)
                c.create_line(a-21*scale,midy-14*scale+move,a+21*scale,midy-14*scale+move,fill="#d1e7fc",width=2)
            c.create_oval(midx-6*scale,midy-5*scale+move,midx+6*scale,midy+7*scale+move,fill="#8af5dd")
        elif role in (3,5):
            # Stalker/scavenger use four limbs and a tail, not 14 human joints.
            c.create_oval(midx-42*scale,midy-23*scale,midx+42*scale,midy+6*scale,fill="#536273",outline=color,width=2)
            for k in (-30,-11,11,30):
                swing=math.sin(phase+(0 if k%2 else math.pi))*8*scale
                c.create_line(midx+k*scale,midy,midx+(k*1.15)*scale+swing,midy+35*scale,
                              fill="#829bb0",width=max(2,round(6*scale)))
            c.create_line(midx-39*scale,midy-10*scale,midx-71*scale,midy-22*scale+move,fill=color,width=3)
        else:
            # Turrets and guardians are non-humanoid mechanical rigs.
            c.create_rectangle(midx-40*scale,midy-36*scale,midx+40*scale,midy+26*scale,
                               fill="#46556b",outline="#b8c4d6",width=2)
            c.create_oval(midx-12*scale,midy-19*scale,midx+12*scale,midy+5*scale,fill="#8cb6d8")
            c.create_line(midx+26*scale,midy-8*scale+move,midx+65*scale,midy-8*scale+move,
                          fill=color,width=max(2,round(7*scale)))
        c.create_text(w/2,20,text="PROCEDURAL ENEMY · JOINT EDITING NOT SUPPORTED",fill="#e3b977")
        c.create_text(9,h-10,anchor="sw",fill="#aec3d9",text=f"PROCEDURAL · frame {int(frame)%60:02d}/59 · no editable action poses")

    def start_drag(self,event):
        if not self.app.model_document() or not can_edit_joint(self.app.kind,self.app.doc):return
        if not self.points:return
        distances=[(event.x-x)**2+(event.y-y)**2 for x,y in self.points]
        j=min(range(14),key=lambda i:distances[i])
        if distances[j]<=19**2:
            self.app.joint_index.set(j)
            self.drag_joint=j
            self.last=(event.x,event.y)
            self.app.update_joint_display()
            self.app.redraw_models()

    def drag(self,event):
        if self.drag_joint is None or self.last is None:return
        if not can_edit_joint(self.app.kind,self.app.doc):return
        dx,dy=event.x-self.last[0],event.y-self.last[1]
        right,down=self.axes()
        scale=min(self.canvas.winfo_width()/108.,self.canvas.winfo_height()/101.)*self.scale/2.6
        delta=[(right[i]*dx+down[i]*dy)/max(.2,scale) for i in range(3)]
        pose=self.app.pose.get().lower();i=self.drag_joint
        rig=self.app.doc["skeleton"]
        layer,depth=active_layer(self.app.doc,pose,self.app.preview_frame())
        # Mouse motion is intentionally unconstrained by the old +/-25 rig limit.
        # Preserve sensible numerical limits for binary float32 interchange only.
        for axis in range(2):
            layer[i][axis]=round(max(-65536.,min(65536.,layer[i][axis]+delta[axis])),3)
        depth[i]=round(max(-65536.,min(65536.,depth[i]+delta[2])),3)
        self.last=(event.x,event.y)
        self.app.dirty=True;self.app.update_joint_display();self.app.redraw_models()

    def end_drag(self,_event):self.drag_joint=None;self.last=None
    def begin_camera(self,event,mode):self.last=(event.x,event.y,mode)
    def camera_drag(self,event,mode):
        if not self.last or len(self.last)!=3 or self.last[2]!=mode:return
        dx,dy=event.x-self.last[0],event.y-self.last[1]
        self.last=(event.x,event.y,mode)
        if mode=="orbit" and self.axis=="orbit":
            self.yaw+=dx*.012;self.pitch=max(-1.3,min(1.3,self.pitch+dy*.008))
        elif mode=="pan":self.pan[0]+=dx;self.pan[1]+=dy
        self.paint()
    def zoom(self,event):self.zoom_delta(1 if event.delta>0 else -1)
    def zoom_delta(self,d):self.scale=max(.08,min(24,self.scale*(1+.1*d)));self.paint()


class Studio(tk.Tk):
    def __init__(self, options):
        super().__init__()
        self.title(f"VOIDLINE Model Studio {VERSION} · Linux")
        self._window_icon = None
        try:
            icon_file = ROOT.parent / "icons" / "model-studio.png"
            if icon_file.is_file():
                self._window_icon = tk.PhotoImage(file=str(icon_file))
                self.iconphoto(True, self._window_icon)
        except tk.TclError:
            pass
        style=ttk.Style(self)
        # Native ttk Treeview uses a 20px row on some distros, which overlaps
        # DejaVu Sans 11 on GNOME/KDE HiDPI configurations. Force usable rows.
        style.configure("Studio.Treeview",font=("DejaVu Sans",11),rowheight=30)
        style.configure("Studio.Treeview.Heading",font=("DejaVu Sans",10,"bold"))
        style.map("Studio.Treeview",background=[("selected","#305a75")],
                  foreground=[("selected","#ffffff")])
        style.configure("Heading.TLabel",font=("DejaVu Sans",10,"bold"))
        screen_w,screen_h=self.winfo_screenwidth(),self.winfo_screenheight()
        window_w=min(1440,max(960,screen_w-48))
        window_h=min(870,max(640,screen_h-92))
        self.geometry(f"{window_w}x{window_h}+{max(0,(screen_w-window_w)//2)}+{max(0,(screen_h-window_h)//2)}")
        self.minsize(960,640)
        self.after_idle(self.maximize_window)
        self.project=None;self.game=None;self.path=None;self.doc=None
        self.kind=None;self.model_id=0;self.dirty=False;self.selected_level=None;self.disk_signature=None
        self.joint_index=tk.IntVar(value=3)
        self.pose=tk.StringVar(value="Run")
        self.frame=tk.IntVar(value=0)
        self.playing=False;self._after=None;self._play_last=0.0;self.fps=30
        self._frame_accumulator=0.;self._absolute_frame=0
        self.speed_percent=tk.IntVar(value=100)
        self.show_plates=tk.BooleanVar(value=True)
        self.status=tk.StringVar(value="Open a VOIDLINE game workspace or project folder")
        self.image_reference=None
        self.world_origin=0.;self.world_y_origin=0.;self.world_zoom=.25
        self.node_items={}
        self.views=[]
        self.fields={}
        self.mode=tk.StringVar(value="Select")
        self.edit_vertices=tk.BooleanVar(value=False)
        self.selected_vertex=None
        self._build_ui()
        self.bind("<Control-s>", lambda e:self.save())
        self.bind("<F5>", lambda e:self.reload_file())
        self.bind("<Control-o>", lambda e:self.open_document())
        self.bind("<Control-Shift-O>", lambda e:self.open_workspace())
        self.bind("<Control-Shift-S>", lambda e:self.save_as())
        self.bind("<Delete>",self.on_delete_key)
        self.bind("<Control-c>",self.copy_model_key)
        self.bind("<Control-v>",self.paste_model_key)
        self.bind("<Control-Shift-C>",self.copy_movement_key)
        self.bind("<Control-Shift-V>",self.paste_movement_key)
        self.bind("<F11>", lambda e:self.toggle_maximized())
        if options:
            if options[0]=="--open-workspace" and len(options)>1:
                self.after(100,lambda:self.open_workspace(options[1]))
            elif options[0]=="--game-dir" and len(options)>1:
                self.after(100,lambda:self.open_workspace(options[1]))
        else:
            # The source checkout, not a private Model Studio project, is the
            # only persistent input. A CLI path always overrides autodetection.
            checkout=discover_game_workspace(ROOT.parent, Path.cwd())
            if checkout is not None:
                self.after(100,lambda path=checkout:self.open_workspace(str(path)))

    def maximize_window(self):
        """Use the window manager's maximize state; maintain a usable fallback."""
        try:self.attributes("-zoomed",True)
        except tk.TclError:
            try:self.state("zoomed")
            except tk.TclError:
                self.geometry(f"{self.winfo_screenwidth()}x{self.winfo_screenheight()}+0+0")

    def toggle_maximized(self):
        try:self.attributes("-zoomed",not bool(self.attributes("-zoomed")))
        except tk.TclError:self.maximize_window()

    def on_window_resize(self,event):
        if event.widget is self:
            self.status_label.configure(wraplength=max(300,event.width-30))

    def _dialog(self,title,message,buttons=("OK",),prompt=None,minvalue=None,maxvalue=None):
        return StudioDialog(self,title,message,buttons,prompt,minvalue,maxvalue).show()

    def show_info(self,title,message,**_kwargs):
        self._dialog(title,message)
    def show_warning(self,title,message,**_kwargs):
        self._dialog(title,message)
    def show_error(self,title,message,**_kwargs):
        self._dialog(title,message)
    def ask_yes_no(self,title,message,**_kwargs):
        return self._dialog(title,message,("Yes","No"))=="Yes"
    def ask_yes_no_cancel(self,title,message,**_kwargs):
        result=self._dialog(title,message,("Yes","No","Cancel"))
        return None if result=="Cancel" else result=="Yes"
    def ask_string(self,title,message,parent=None,**_kwargs):
        result=self._dialog(title,message,("OK","Cancel"),prompt="string")
        return None if result=="Cancel" else result
    def ask_integer(self,title,message,minvalue=None,maxvalue=None,parent=None,**_kwargs):
        result=self._dialog(title,message,("OK","Cancel"),prompt="integer",
                            minvalue=minvalue,maxvalue=maxvalue)
        return None if result=="Cancel" else result

    def _clipboard_text(self):
        try:return self.clipboard_get()
        except tk.TclError as exc:raise ValueError("System clipboard is empty or has no text") from exc

    def _write_clipboard(self,payload):
        self.clipboard_clear();self.clipboard_append(payload)
        self.update_idletasks()   # publish to desktop clipboard manager

    def copy_model(self):
        try:
            if self.kind not in ("character","enemy"):raise ValueError("Select a character/enemy model")
            self.apply_fields()
            self._write_clipboard(copy_model_payload(self.doc,self.kind))
            self.status.set(f"Copied {self.kind} model to system clipboard")
        except ValueError as exc:self.show_warning("Copy model",str(exc))

    def paste_model(self):
        try:
            if self.kind not in ("character","enemy"):raise ValueError("Select a character/enemy model")
            pasted=paste_model_payload(self._clipboard_text(),self.doc,self.kind)
            if pasted==self.doc:return
            self.doc=pasted;self.dirty=True
            self.show_properties();self.redraw_models()
            self.status.set(f"Pasted model into {self.path.name}; slot ID preserved. Ctrl+S to save to source.")
        except ValueError as exc:self.show_warning("Paste model",str(exc))

    def copy_movement(self):
        try:
            if not can_edit_joint(self.kind,self.doc):raise ValueError("Select a model with an editable skeleton")
            self._write_clipboard(copy_movement_payload(self.doc,self.pose.get().lower()))
            self.status.set(f"Copied {self.pose.get()} movement to system clipboard")
        except ValueError as exc:self.show_warning("Copy movement",str(exc))

    def paste_movement(self):
        try:
            if not can_edit_joint(self.kind,self.doc):raise ValueError("Select a model with an editable skeleton")
            pose=self.pose.get().lower()
            pasted=paste_movement_payload(self._clipboard_text(),self.doc,pose)
            if pasted==self.doc:return
            self.doc=pasted;self.dirty=True
            self.update_joint_display();self.redraw_models()
            self.status.set(f"Pasted movement into {pose} of {self.path.name}; Ctrl+S saves to source.")
        except ValueError as exc:self.show_warning("Paste movement",str(exc))

    @staticmethod
    def _text_editor_focused(event):
        return isinstance(event.widget,(tk.Entry,tk.Text,ttk.Entry,ttk.Combobox,tk.Spinbox))
    def copy_model_key(self,event):
        if self._text_editor_focused(event):return
        self.copy_model();return "break"
    def paste_model_key(self,event):
        if self._text_editor_focused(event):return
        self.paste_model();return "break"
    def copy_movement_key(self,event):
        if self._text_editor_focused(event):return
        self.copy_movement();return "break"
    def paste_movement_key(self,event):
        if self._text_editor_focused(event):return
        self.paste_movement();return "break"

    def _build_ui(self):
        menubar=tk.Menu(self)
        file=tk.Menu(menubar,tearoff=False)
        entries=[("Open Game Workspace…",self.open_workspace),("Open Source Project…",self.pick_project),
                 ("Open File…",self.open_document),("Save",self.save),("Save As…",self.save_as),
                 ("Export Single Model…",self.export_single),("Publish Model Sources + Levels",self.update_workspace),
                 ("Also Publish Runtime PAK…",lambda:self.update_workspace(update_pak=True)),
                 ("Export Project Sources",self.export_project)]
        for idx,(label,callback) in enumerate(entries):
            if idx in (3,5):file.add_separator()
            file.add_command(label=label,command=callback)
        file.add_separator();file.add_command(label="Quit",command=self.destroy)
        file.add_separator()
        file.add_command(label="New Character…",command=lambda:self.new_model("character"))
        file.add_command(label="New Enemy…",command=lambda:self.new_model("enemy"))
        file.add_command(label="Assign Model to Existing Game Slot…",command=self.assign_model)
        menubar.add_cascade(label="File",menu=file)
        edit=tk.Menu(menubar,tearoff=False)
        edit.add_command(label="Copy Model     Ctrl+C",command=self.copy_model)
        edit.add_command(label="Paste Model     Ctrl+V",command=self.paste_model)
        edit.add_separator()
        edit.add_command(label="Copy Movement     Ctrl+Shift+C",command=self.copy_movement)
        edit.add_command(label="Paste Movement     Ctrl+Shift+V",command=self.paste_movement)
        menubar.add_cascade(label="Edit",menu=edit)
        view=tk.Menu(menubar,tearoff=False)
        view.add_checkbutton(label="Show body volumes",variable=self.show_plates,command=self.redraw_models)
        view.add_command(label="Reset 3D orbit",command=self.reset_orbit)
        view.add_command(label="Reload Selected File from Game Workspace",command=self.reload_file)
        menubar.add_cascade(label="View",menu=view)
        help_menu=tk.Menu(menubar,tearoff=False)
        help_menu.add_command(label="About",command=lambda:self.show_info("Model Studio",f"VOIDLINE Model Studio {VERSION}\nLinux-only · Tk/ttk desktop interface\nC++ authoring/export backend"))
        menubar.add_cascade(label="Help",menu=help_menu)
        self.config(menu=menubar)
        toolbar=ttk.Frame(self,padding=5);toolbar.pack(fill="x")
        toolbar.columnconfigure(0,weight=1)
        files=ttk.LabelFrame(toolbar,text="Workspace & files",padding=5)
        files.grid(row=0,column=0,sticky="ew",padx=2,pady=2)
        for name,func in (("New character",lambda:self.new_model("character")),("New enemy",lambda:self.new_model("enemy")),("Open workspace",self.open_workspace),("Open file",self.open_document),
                          ("Save",self.save),("Export model",self.export_single),
                          ("Publish model sources + levels",self.update_workspace)):
            ttk.Button(files,text=name,command=func).pack(side="left",padx=2)
        animation=ttk.LabelFrame(toolbar,text="Animation preview · 30 FPS",padding=5)
        animation.grid(row=1,column=0,sticky="ew",padx=3,pady=2)
        ttk.Label(animation,text="Pose").pack(side="left",padx=3)
        self.pose_combo=ttk.Combobox(animation,textvariable=self.pose,values=("Base","Aim","Run"),state="readonly",width=12)
        self.pose_combo.pack(side="left")
        self.pose.trace_add("write",lambda *_:self.on_pose_change())
        ttk.Button(animation,text="↤",width=3,command=self.rewind).pack(side="left",padx=(6,1))
        ttk.Button(animation,text="Step",width=5,command=self.step).pack(side="left",padx=1)
        self.play_button=ttk.Button(animation,text="Play ▶",width=8,command=self.toggle_play)
        self.play_button.pack(side="left",padx=2)
        ttk.Label(animation,text="Run speed").pack(side="left",padx=(6,2))
        self.speed_slider=ttk.Scale(animation,from_=5,to=200,variable=self.speed_percent,
                                     command=self.on_speed_change,length=105)
        self.speed_slider.pack(side="left",padx=2)
        self.speed_label=ttk.Label(animation,text="100%",width=5)
        self.speed_label.pack(side="left",padx=(0,5))
        ttk.Button(animation,text="100%",width=4,command=self.reset_speed).pack(side="left",padx=2)
        ttk.Label(animation,text="Frame").pack(side="left",padx=(5,2))
        self.timeline=ttk.Scale(animation,from_=0,to=59,variable=self.frame,
                                command=self.on_timeline_scrub,length=95)
        self.timeline.pack(side="left",fill="x",expand=True,padx=2)
        self.frame_label=ttk.Label(animation,text="00/59",width=6)
        self.frame_label.pack(side="left",padx=3)
        self.play_status=ttk.Label(animation,text="Stopped",width=14)
        self.play_status.pack(side="left",padx=3)
        self.frame.trace_add("write",lambda *_:self.frame_label.config(text=f"{int(self.frame.get()):02d}/59"))
        whole=ttk.Panedwindow(self,orient="horizontal");whole.pack(fill="both",expand=True,padx=5,pady=3)
        left=ttk.Frame(whole,width=240);whole.add(left,weight=1)
        ttk.Label(left,text="PROJECT NAVIGATOR",style="Heading.TLabel").pack(anchor="w",padx=4,pady=(4,8))
        self.tree=ttk.Treeview(left,show="tree",selectmode="browse",style="Studio.Treeview")
        scroll=ttk.Scrollbar(left,orient="vertical",command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll.set)
        self.tree.pack(side="left",fill="both",expand=True);scroll.pack(side="right",fill="y")
        self.tree.bind("<<TreeviewSelect>>",self._select_tree)
        center=ttk.Frame(whole);whole.add(center,weight=5)
        self.tabs=ttk.Notebook(center);self.tabs.pack(fill="both",expand=True)
        self.level_page=ttk.Frame(self.tabs);self.tabs.add(self.level_page,text="Level Layout")
        self.model_page=ttk.Frame(self.tabs);self.tabs.add(self.model_page,text="Rig & Animation")
        self.asset_page=ttk.Frame(self.tabs);self.tabs.add(self.asset_page,text="Assets")
        level_head=ttk.LabelFrame(self.level_page,text="Level editing · click to select, drag to move",padding=5)
        level_head.pack(fill="x",padx=4,pady=3)
        ttk.Label(level_head,text="Tool").pack(side="left",padx=3)
        ttk.Combobox(level_head,textvariable=self.mode,state="readonly",width=16,
                     values=("Select",)+tuple(TOOL_KEYS)).pack(side="left",padx=2)
        ttk.Button(level_head,text="Delete selected (Del)",command=self.delete_level_selected).pack(side="left",padx=6)
        ttk.Checkbutton(level_head,text="Edit corners",variable=self.edit_vertices,command=self.draw_level).pack(side="left",padx=3)
        ttk.Button(level_head,text="+ Point",command=self.add_selected_vertex).pack(side="left",padx=2)
        ttk.Button(level_head,text="− Point",command=self.remove_selected_vertex).pack(side="left",padx=2)
        ttk.Button(level_head,text="Fit level",command=self.level_fit).pack(side="left",padx=2)
        ttk.Button(level_head,text="Frame selection",command=self.level_focus_selected).pack(side="left",padx=2)
        self.level_zoom_label=ttk.Label(level_head,text="25%",width=5)
        self.level_zoom_label.pack(side="right",padx=4)
        level_hint=ttk.Frame(self.level_page,padding=(8,2))
        level_hint.pack(fill="x")
        self.level_hint=ttk.Label(level_hint,text="Left drag: move/vertex  •  Double click: add corner  •  Del: delete selected object  •  Wheel: zoom  •  Right drag: pan",
                  foreground="#689cba",justify="left")
        self.level_hint.pack(fill="x")
        level_hint.bind("<Configure>",lambda e:self.level_hint.configure(wraplength=max(160,e.width-12)))
        self.level_canvas=tk.Canvas(self.level_page,background="#101927",highlightthickness=1,
                                    highlightbackground="#3b6d91")
        self.level_canvas.pack(fill="both",expand=True,padx=4,pady=4)
        self.level_canvas.bind("<Configure>",lambda _e:self.draw_level())
        self.level_canvas.bind("<ButtonPress-1>",self.level_start)
        self.level_canvas.bind("<B1-Motion>",self.level_drag)
        self.level_canvas.bind("<ButtonRelease-1>",self.level_end)
        self.level_canvas.bind("<Double-Button-1>",self.add_vertex_at_mouse)
        self.level_canvas.bind("<ButtonPress-2>",self.level_pan_start)
        self.level_canvas.bind("<B2-Motion>",self.level_pan_drag)
        self.level_canvas.bind("<ButtonPress-3>",self.level_pan_start)
        self.level_canvas.bind("<B3-Motion>",self.level_pan_drag)
        self.level_canvas.bind("<MouseWheel>",self.level_wheel)
        self.level_canvas.bind("<Button-4>",lambda e:self.level_zoom(1,e.x,e.y))
        self.level_canvas.bind("<Button-5>",lambda e:self.level_zoom(-1,e.x,e.y))
        grid=ttk.Frame(self.model_page,padding=4);grid.pack(fill="both",expand=True)
        for rr in range(2):grid.rowconfigure(rr,weight=1)
        for cc in range(2):grid.columnconfigure(cc,weight=1)
        for index,axis in enumerate(("front","side","top","orbit")):
            viewport=RigViewport(grid,self,axis)
            viewport.grid(row=index//2,column=index%2,sticky="nsew",padx=2,pady=2)
            self.views.append(viewport)
        ttk.Label(self.asset_page,text="Select an image asset from the workspace. Files are edited directly in game/assets/.").pack(anchor="w",padx=10,pady=10)
        self.image_label=ttk.Label(self.asset_page,anchor="center")
        self.image_label.pack(fill="both",expand=True,padx=14,pady=8)
        ttk.Button(self.asset_page,text="Replace Selected Artwork…",command=self.replace_asset).pack(pady=8)
        inspect=ttk.Frame(whole,width=280);whole.add(inspect,weight=2)
        ttk.Label(inspect,text="PROPERTIES / INSPECTOR",style="Heading.TLabel").pack(anchor="w",padx=4,pady=(4,8))
        self.info=ttk.Label(inspect,text="No document selected",wraplength=275,justify="left")
        self.info.pack(anchor="w",pady=5,fill="x")
        inspect.bind("<Configure>",lambda e:self.info.configure(wraplength=max(110,e.width-24)))
        # Scrollable native inspector ensures all rig parameters remain accessible
        # when a user tiles or shrinks the application window.
        scroll_host=ttk.Frame(inspect)
        scroll_host.pack(fill="both",expand=True)
        self.inspect_canvas=tk.Canvas(scroll_host,highlightthickness=0,width=280)
        vertical_scroll=ttk.Scrollbar(scroll_host,orient="vertical",command=self.inspect_canvas.yview)
        self.inspect_canvas.configure(yscrollcommand=vertical_scroll.set)
        self.inspect_canvas.pack(side="left",fill="both",expand=True)
        vertical_scroll.pack(side="right",fill="y")
        self.props=ttk.Frame(self.inspect_canvas)
        self.props_window=self.inspect_canvas.create_window((0,0),window=self.props,anchor="nw")
        self.props.bind("<Configure>",lambda _e:self.inspect_canvas.configure(scrollregion=self.inspect_canvas.bbox("all")))
        self.inspect_canvas.bind("<Configure>",lambda e:self.inspect_canvas.itemconfigure(self.props_window,width=e.width))
        self.inspect_canvas.bind("<MouseWheel>",lambda e:self.inspect_canvas.yview_scroll(-1 if e.delta>0 else 1,"units"))
        self.joint_info=ttk.Label(inspect,text="",wraplength=270,justify="left",font=("DejaVu Sans",10))
        self.joint_info.pack(anchor="w",pady=4,fill="x")
        inspect.bind("<Configure>",lambda e:(self.info.configure(wraplength=max(110,e.width-24)),
                                              self.joint_info.configure(wraplength=max(110,e.width-24))))
        self.status_label=ttk.Label(self,textvariable=self.status,anchor="w",relief="sunken",
                                    padding=(8,6),justify="left",font=("DejaVu Sans",10))
        self.status_label.pack(fill="x")
        self.bind("<Configure>",self.on_window_resize,add="+")

    def open_workspace(self,path=None):
        path=path or filedialog.askdirectory(
            title="Select VOIDLINE source root or built game folder")
        if not path:return
        game=Path(path).expanduser().resolve()
        # The backend resolves source/assets when no runtime PAK has been built.
        # Do not duplicate its path rules here: nested build/editor folders work.
        if self.dirty and self.path:
            answer=self.ask_yes_no_cancel("Unsaved changes", "Save changes to the current workspace before opening another one?")
            if answer is None:return
            if answer and not self.save():return
        self._job([str(CORE),"--prepare-workspace",str(game)],
                  "Loading game models and assets",self._workspace_imported)

    def _workspace_imported(self,output):
        values=dict(line.split("\t",1) for line in output.splitlines() if "\t" in line)
        if "WORKSPACE" not in values or "GAME" not in values:
            raise RuntimeError("Backend did not return a resolved authoring workspace")
        self.game=Path(values["GAME"])
        self.open_project(Path(values["WORKSPACE"]))
        if values.get("MODE") != "source" or Path(values["WORKSPACE"]).resolve()!=self.game.resolve():
            raise RuntimeError("Expected a live game source workspace, not a cached project")
        self.status.set(f"Using live game files: {self.game}. Save writes directly to source; Publish bakes assets.")

    def pick_project(self):
        # An "authoring project" is the game source checkout, never an editor cache.
        self.open_workspace()

    def open_project(self,path):
        project=Path(path).expanduser().resolve()
        if self.game is None or project!=self.game.resolve():
            self.show_error("Workspace", "Choose a VOIDLINE game source root using Open Game Workspace")
            return
        if not (project/"assets").is_dir() or not (project/"levels").is_dir() or not (project/"models").is_dir():
            self.show_error("Workspace", "Game source must contain assets/, levels/ and models/")
            return
        self.project=project;self.path=None;self.doc=None;self.kind=None;self.dirty=False
        self.disk_signature=None
        self.refresh_tree()
        self.status.set(f"Opened live game source: {project}")
        children=self.tree.get_children("characters")
        if children:self.tree.selection_set(children[0]);self.tree.see(children[0]);self._select_tree()

    def source_file(self,path):
        """Allow opening/saving only files belonging to the active game checkout."""
        if self.project is None:return False
        target=Path(path).expanduser().resolve()
        return any(target.is_relative_to(self.project/folder) for folder in ("models","levels","assets"))

    @staticmethod
    def file_signature(path):
        try:
            stat=Path(path).stat()
            return (stat.st_mtime_ns,stat.st_size)
        except FileNotFoundError:return None

    def reload_file(self):
        if self.path is None:return
        if self.dirty:
            if not self.ask_yes_no("Reload from source", "Discard unsaved changes and reload directly from the game file?"):
                return
        self.load_file(self.path,self.kind)

    def refresh_tree(self):
        self.tree.delete(*self.tree.get_children())
        self.node_items={}
        for key,title in (("levels","LEVELS"),("characters","CHARACTERS"),("enemies","ENEMIES"),("custom_characters","NEW CHARACTERS"),("custom_enemies","NEW ENEMIES"),("assets","MEDIA / ASSETS")):
            self.tree.insert("", "end",iid=key,text=title,open=key!="assets")
        for folder,key,pattern in (("levels","levels","*.vllevel.json"),("models/characters","characters","*.vlcharacter.json"),("models/enemies","enemies","*.vlenemy.json"),("models/custom/characters","custom_characters","*.vlcharacter.json"),("models/custom/enemies","custom_enemies","*.vlenemy.json"),("assets","assets","*")):
            files=sorted((self.project/folder).glob(pattern))
            if key=="assets":files=[p for p in (self.project/folder).rglob("*") if p.is_file() and p.suffix.lower() in (".png",".webp",".jpg")]
            for idx,path in enumerate(files):
                name=path.name if key!="assets" else str(path.relative_to(self.project/"assets"))
                iid=f"{key}_{idx}"
                self.tree.insert(key,"end",iid=iid,text=name)
                self.node_items[iid]=(key,path)

    def _select_tree(self,_event=None):
        selection=self.tree.selection()
        if not selection or selection[0] not in self.node_items:return
        key,path=self.node_items[selection[0]]
        if self.dirty and self.path and self.path != path:
            answer=self.ask_yes_no_cancel("Unsaved changes","Save changes before switching documents?")
            if answer is None:return
            if answer and not self.save():return
        self.load_file(path, "level" if key=="levels" else "character" if key in ("characters","custom_characters") else "enemy" if key in ("enemies","custom_enemies") else "asset")

    def load_file(self,path,kind=None):
        path=Path(path).resolve()
        if not self.source_file(path):
            self.show_error("Open file", "Only files inside this game's models/, levels/ or assets/ can be edited")
            return
        kind=kind or ("level" if path.name.endswith(".vllevel.json") else "character" if path.name.endswith(".vlcharacter.json") else "enemy" if path.name.endswith(".vlenemy.json") else "asset")
        try:
            doc=json.loads(path.read_text()) if kind!="asset" else None
            if kind in ("character","enemy"):prepare_motion(doc, self.project / "models/movement-descriptions.json" if self.project else None)
        except Exception as exc:
            self.show_error("Open file",str(exc));return
        self.stop_play()
        self.path=Path(path);self.doc=doc;self.kind=kind;self.dirty=False
        self.disk_signature=self.file_signature(path)
        self.selected_level=None
        self._absolute_frame=0
        self.frame.set(0)
        self.title(f"{self.path.name} — VOIDLINE Model Studio {VERSION}")
        self.info.config(text=f"{kind.title()}: {doc.get('name',self.path.name) if isinstance(doc,dict) else self.path.name}\n{self.path.name}")
        if kind in ("character","enemy"):
            choices=movement_sequences(kind,doc,self.project / "models/movement-descriptions.json" if self.project else None)
            self.pose_combo.configure(values=choices)
            self.pose_combo.state(["!disabled"] if choices else ["disabled"])
            if self.pose.get().lower() not in choices:
                self.pose.set("run" if "run" in choices else choices[0] if choices else "")
        self.tabs.select({"level":self.level_page,"character":self.model_page,"enemy":self.model_page,"asset":self.asset_page}[kind])
        self.show_properties();self.draw_level();self.redraw_models()
        if kind=="asset":self.show_asset()
        self.status.set(f"Loaded {self.path}")

    def new_model(self, kind):
        if not self.project:
            self.show_info("New model", "Open a game workspace first.")
            return
        name=self.ask_string("New "+kind, "Model display name:", parent=self)
        if not name:return
        folder=self.project/"models"/("characters" if kind=="character" else "enemies")
        files=list(folder.glob("*.vlcharacter.json" if kind=="character" else "*.vlenemy.json"))
        if not files:
            self.show_error("New model","No archetype is available as a template")
            return
        template=json.loads(files[0].read_text())
        chosen=self.doc if self.kind==kind else template
        custom=self.project/"models"/"custom"/("characters" if kind=="character" else "enemies")
        custom.mkdir(parents=True,exist_ok=True)
        slug=re.sub(r"[^a-z0-9-]+","-",name.lower()).strip("-") or "model"
        suffix=".vlcharacter.json" if kind=="character" else ".vlenemy.json"
        new_id=max([int(p.stem.split(".")[0]) for p in custom.glob("[0-9]*.*") if p.stem.split(".")[0].isdigit()] or [99])+1
        target=custom/f"{new_id:03d}-{slug}{suffix}"
        if target.exists():self.show_error("New model","Model already exists");return
        doc=create_model_from_template(chosen,kind,name,new_id)
        target.write_text(json.dumps(doc,indent=2)+"\n")
        self.refresh_tree();self.load_file(target,kind)
        self.status.set(f"New {kind} saved in the prototype library. Assign it to a game slot to publish.")

    def assign_model(self):
        if self.kind not in ("character","enemy") or not self.doc or not self.project:return
        n=6 if self.kind=="character" else 10
        slot=self.ask_integer("Assign to game slot",f"Choose a built-in {self.kind} slot to REPLACE (0–{n-1}):",
                                     minvalue=0,maxvalue=n-1,parent=self)
        if slot is None:return
        base=self.project/"models"/("characters" if self.kind=="character" else "enemies")
        matches=list(base.glob(f"{slot}.vl{self.kind}.json"))
        if not matches:
            self.show_error("Assign model",f"Runtime slot {slot} not found");return
        if not self.ask_yes_no("Replace game archetype",f"Replace {matches[0].name} with '{self.doc.get('name')}'?\nThe original will be backed up."):
            return
        if not self.save():return
        target=matches[0]
        shutil.copy2(target,target.with_name(target.name+".backup"))
        target.write_text(json.dumps(assign_model_to_slot(self.doc,slot),indent=2)+"\n")
        self.refresh_tree();self.load_file(target,self.kind)
        self.status.set(f"Assigned model to runtime slot {slot}; use Publish models + levels to publish")

    def open_document(self):
        if not self.project:
            self.show_info("Game workspace", "Open a game source workspace first")
            return
        file=filedialog.askopenfilename(title="Open file from game workspace",initialdir=str(self.project),
            filetypes=[("VOIDLINE models", "*.vllevel.json *.vlcharacter.json *.vlenemy.json"),
                       ("Images","*.png *.webp *.jpg"),("All files","*")])
        if file:self.load_file(Path(file))

    def save(self):
        if self.doc is None or self.kind=="asset" or self.path is None:return True
        if not self.source_file(self.path):
            self.show_error("Save", "Refusing to save outside the game workspace")
            return False
        if self.file_signature(self.path)!=self.disk_signature:
            self.show_warning("External change detected",
                "The game source file changed on disk since it was opened. "
                "Reload the file to avoid overwriting newer changes.")
            return False
        self.apply_fields()
        if not self.dirty:
            self.status.set(f"Unchanged: {self.path} (read directly from game source)")
            return True
        self.path.parent.mkdir(parents=True,exist_ok=True)
        temporary=self.path.with_name(self.path.name+".tmp")
        temporary.write_text(json.dumps(self.doc,indent=2)+"\n")
        temporary.replace(self.path)
        self.disk_signature=self.file_signature(self.path)
        self.dirty=False
        self.status.set(f"Saved directly to game source: {self.path}")
        return True

    def save_as(self):
        if self.doc is None or self.kind=="asset" or not self.project:return
        pattern={"level":".vllevel.json","character":".vlcharacter.json","enemy":".vlenemy.json"}[self.kind]
        directory={"level":"levels","character":"models/characters","enemy":"models/enemies"}[self.kind]
        file=filedialog.asksaveasfilename(title="Save As (inside game source)",initialdir=str(self.project/directory),
            initialfile=self.path.name if self.path else "",defaultextension=pattern,
            filetypes=[("VOIDLINE model", "*"+pattern)])
        if not file:return
        if not self.source_file(file):
            self.show_error("Save As", "Use Export Single Model for copies outside the game source")
            return
        if not str(file).endswith(pattern):
            self.show_error("Save As", "The chosen file must have extension "+pattern)
            return
        self.path=Path(file).resolve()
        self.disk_signature=self.file_signature(self.path)
        self.dirty=True
        self.save()

    def export_single(self):
        if self.doc is None or self.kind=="asset":return
        if not self.save():return
        pattern={"level":".vllevel.json","character":".vlcharacter.json","enemy":".vlenemy.json"}[self.kind]
        out=filedialog.asksaveasfilename(title="Export single model",defaultextension=pattern,initialfile=self.path.name,
                                         filetypes=[("Model", "*"+pattern)])
        if out:
            if Path(out).resolve()!=self.path.resolve():shutil.copy2(self.path,out)
            self.status.set(f"Exported {self.kind} to {out} (copy only; select Publish to publish)")

    def update_workspace(self, update_pak=False):
        if not self.project or not self.game:
            self.show_info("Game workspace","Open a VOIDLINE game workspace first")
            return
        if not self.ask_yes_no("Publish model sources", "Save editable model JSON and movement descriptions, "
                                   "publish modified levels and rebuild-input model assets "
                                   "to the game source?\n\n"
                                   + ("Also rebuild runtime PAKs. Stop the game first.\n\n" if update_pak
                                      else "The installed game PAK will NOT be changed.\n\n")
                                   + "Changed source files will be backed up."):
            return
        if not self.save():return
        probe=subprocess.run([str(CORE),"--detect-source",str(self.game)],
                             capture_output=True,text=True,timeout=15)
        source=next((line.split("\t",1)[1] for line in probe.stdout.splitlines()
                     if line.startswith("SOURCE\t")),None) if probe.returncode==0 else None
        if not source and self.ask_yes_no("Game source not found",
                 "This appears to be a standalone installed game. You can select its original "
                 "source tree to preserve edits across builds.\n\nSelect game source now?\n"
                 "(Choose No to update the installed game only.)"):
            source=filedialog.askdirectory(title="Choose VOIDLINE game source root")
            if not source:return
        command=[str(CORE),"--update-workspace" if update_pak else "--publish-sources",str(self.game)]
        if source:command.append(str(source))
        def published(output):
            lines=output.strip().splitlines()
            self.status.set(lines[-1] if lines else "Workspace updated")
            paths=[line.split("\t",1)[1] for line in lines if line.startswith("RUNTIME_PAK\t")]
            staged=[line.split("\t",1)[1] for line in lines if line.startswith("PACK_STAGED\t")]
            level_changes=[line.split("\t") for line in lines if line.startswith("LEVELS_UPDATED\t")]
            level_paths=[line.split("\t",1)[1] for line in lines if line.startswith("RUNTIME_LEVEL\t")]
            sources=[line.split("\t",1)[1] for line in lines if line.startswith("SOURCE_UPDATED\t")]
            inputs=[line.split("\t",1)[1] for line in lines if line.startswith("INPUT_MODELS_UPDATED\t")]
            report=[]
            if inputs:report.append("Editable input models and movement catalog published:\n"+"\n".join(inputs))
            if sources:report.append("Baked source assets updated (survive rebuild):\n"+"\n".join(sources))
            if level_changes:
                report.append("Level files are saved directly in the game source; "
                              "future Linux/Windows rebuilds stage levels/ automatically.")
            if level_paths:report.append("Updated runtime levels (restart game):\n"+"\n".join(level_paths[:12]))
            if paths:report.append("Updated built-game archives (restart game):\n"+"\n".join(paths))
            elif not update_pak:report.append("Runtime PAK not changed (source-only publish). Rebuild the game to use edited model assets.")
            else:report.append("No built game archive found. Build the game to deploy the models.")
            if staged:report.append("Exported complete asset pack:\n"+"\n".join(staged))
            if not sources:report.append("Workspace source assets already match the current game files.")
            self.show_info("Models published","\n\n".join(report))
        self._job(command,"Publishing models and levels to game and sources",published)

    def export_project(self):
        if not self.project:return
        if not self.save():return
        self._job([str(CORE),"--export-project",str(self.project)],"Generating C++ sources and baked models",lambda output:self.status.set(output.strip().splitlines()[-1]))

    def _job(self,cmd,title,on_success):
        win=tk.Toplevel(self);win.title(title);win.transient(self)
        win.geometry("620x170");win.minsize(360,150);win.resizable(True,True)
        ttk.Label(win,text=title,font=("DejaVu Sans",12,"bold")).pack(fill="x",padx=16,pady=(16,8))
        progress=ttk.Progressbar(win,mode="indeterminate");progress.pack(fill="x",padx=16,pady=5);progress.start(12)
        detail=ttk.Label(win,text="Reading and writing the selected game source workspace…",
                         wraplength=580,justify="left",font=DIALOG_FONT)
        detail.pack(fill="both",expand=True,padx=16,pady=3)
        win.bind("<Configure>",lambda e:detail.configure(wraplength=max(200,e.width-36)))
        win.grab_set()
        result={};event=threading.Event()
        def worker():
            try:
                job=subprocess.run(cmd,capture_output=True,text=True,timeout=180,check=True)
                result["stdout"]=job.stdout
            except Exception as exc:result["error"]=str(exc)+(f"\n{exc.stderr}" if hasattr(exc,"stderr") and exc.stderr else "")
            finally:event.set()
        threading.Thread(target=worker,daemon=True).start()
        def poll():
            if not event.is_set():win.after(90,poll);return
            progress.stop();win.grab_release();win.destroy()
            if "error" in result:self.show_error(title,result["error"])
            else:
                try:on_success(result.get("stdout",""))
                except Exception as exc:self.show_error(title,str(exc))
        win.after(90,poll)

    def show_properties(self):
        for widget in self.props.winfo_children():widget.destroy()
        self.fields={}
        if not self.doc:return
        self.inspect_canvas.yview_moveto(0)
        if self.kind in ("character","enemy"):
            metadata=tuple(k for k in (CHARACTER_METADATA if self.kind=="character" else ENEMY_METADATA) if k in self.doc)
            # Present every top-level scalar, including character name, role,
            # description and schema, not just movement statistics.
            extra=tuple(k for k,v in self.doc.items() if k not in metadata and
                        k not in CHARACTER_FIELDS+ENEMY_FIELDS and k not in ("geometry","motion","skeleton") and
                        not isinstance(v,(dict,list)))
            groups=[("Model identity",metadata+extra),
                    ("Statistics",CHARACTER_FIELDS if self.kind=="character" else ENEMY_FIELDS),
                    ("Dimensions",PROPS["geometry"]),("Movement & Reactions",PROPS["motion"]),
                    ("Skeleton Timing",PROPS["skeleton"])]
            for label,names in groups:
                if label=="Skeleton Timing" and not can_edit_joint(self.kind,self.doc):
                    continue
                frame=ttk.LabelFrame(self.props,text=label,padding=5)
                frame.pack(fill="x",padx=2,pady=3)
                for row,key in enumerate(names):
                    group=None if label in ("Statistics","Model identity") else "geometry" if label=="Dimensions" else "motion" if label=="Movement & Reactions" else "skeleton"
                    var=tk.StringVar(value=str((self.doc if group is None else self.doc[group]).get(key,"")))
                    ttk.Label(frame,text=key).grid(row=row,column=0,sticky="w",padx=3,pady=1)
                    if key in ("schema", "id") and label=="Model identity":
                        # File identity and schema are structural, not model properties.
                        ttk.Label(frame,text=var.get()).grid(row=row,column=1,sticky="w",padx=3,pady=3)
                    else:
                        entry=ttk.Entry(frame,textvariable=var,width=8)
                        entry.grid(row=row,column=1,sticky="ew",padx=3,pady=2)
                        entry.bind("<Return>",lambda _e:self.apply_fields())
                        entry.bind("<FocusOut>",lambda _e:self.apply_fields())
                        self.fields[(group,key)]=var
                frame.columnconfigure(1,weight=1)
            ttk.Button(self.props,text="Apply Properties",command=self.apply_fields).pack(fill="x",padx=3,pady=7)
            self.update_joint_display()
        elif self.kind=="level":
            ttk.Label(self.props,text="Select and drag a level object; choose a drawing tool to add geometry.",wraplength=250).pack(anchor="w",padx=7,pady=7)
            self.level_info=ttk.Label(self.props,text="No object selected")
            self.level_info.pack(anchor="w",padx=7,pady=7)
        else:
            ttk.Label(self.props,text="Use Replace Artwork to import a new PNG/WebP image.",wraplength=250).pack(pady=7)

    def apply_fields(self):
        if self.kind not in ("character","enemy") or not self.doc:return
        changed=False
        for (group,key),var in self.fields.items():
            try:
                changed=edit_scalar_property(self.doc,group,key,var.get()) or changed
            except (ValueError,TypeError,KeyError):
                # Keep editing responsive; invalid values are not committed.
                continue
        if changed:
            self.dirty=True
            if self.path and self.kind in ("character", "enemy"):
                self.info.config(text=f"{self.kind.title()}: {self.doc.get('name',self.path.name)}\n{self.path.name}")
            self.redraw_models()

    def update_joint_display(self):
        if not self.model_document():
            self.joint_info.config(text="");return
        self.speed_slider.state(["!disabled"] if self.pose.get().lower() in ("run","walk","sprint","patrol","stairs") else ["disabled"])
        if not can_edit_joint(self.kind,self.doc):
            self.joint_info.config(text=rig_status(self.kind,self.doc)+"\nUse supported movement settings in the inspector.")
            return
        j=self.joint_index.get();pose=self.pose.get().lower()
        rig=self.doc["skeleton"]
        layer,depth=active_layer(self.doc,pose,self.preview_frame())
        x,y=layer[j];z=depth[j]
        animated=pose in rig.get("poseFrames",{})
        key=f"  ·  Key {pose_frame_index(self.preview_frame())+1}/5" if animated else ""
        self.joint_info.config(text=f"{rig_status(self.kind,self.doc)}\nSelected joint: {NAMES[j]}\nPose: {pose}{key}  ·  X {x:+.2f}  Y {y:+.2f}  Z {z:+.2f}\nLeft drag edits the selected source keyframe · Right drag (3D): orbit")

    def redraw_models(self):
        for view in self.views:view.paint()
        self.update_joint_display()

    def model_document(self):return self.kind in ("character","enemy") and self.doc is not None

    def preview_frame(self):
        return self._absolute_frame

    def on_pose_change(self):
        self.stop_play()
        self._absolute_frame=0
        self.frame.set(0)
        self.redraw_models()

    def reset_speed(self):
        self.speed_percent.set(100)
        self.on_speed_change()

    def on_speed_change(self,_value=None):
        speed=max(5,min(200,round(self.speed_percent.get())))
        self.speed_label.config(text=f"{speed}%")
        if self.model_document():
            self.redraw_models()
            if self.pose.get().lower() in ("run","walk","sprint","chase","patrol","stairs"):
                hz=run_cycles_per_second(self.doc,self.kind)*speed/100
                self.status.set(f"Run preview {speed}% · {hz:.2f} gait cycles/s · 100% = game sprint speed; preview-only")

    def on_timeline_scrub(self,_value=None):
        self._absolute_frame=max(0,int(self.frame.get()))
        self.redraw_models()

    def stop_play(self):
        self.playing=False
        if self._after is not None:
            self.after_cancel(self._after)
            self._after=None
        self.play_button.config(text="Play ▶")
        self.play_status.config(text="Paused · 30 FPS")

    def rewind(self):
        self.stop_play()
        self._absolute_frame=0
        self.frame.set(0)
        self.redraw_models()

    def step(self):
        self.stop_play()
        if not self.model_document():return
        self._absolute_frame+=1
        self.frame.set(self._absolute_frame%60)
        self.play_status.config(text=f"Step {self._absolute_frame}")
        self.redraw_models()

    def toggle_play(self):
        if not self.model_document():
            self.status.set("Select a character/enemy model to preview its animation")
            return
        if self.playing:
            self.stop_play()
            return
        self.stop_play()
        self.playing=True
        self._frame_accumulator=0.
        self._play_last=time.monotonic()
        self.play_button.config(text="Pause ❚❚")
        self._after=self.after(round(1000/self.fps),self._play_tick)

    def _play_tick(self):
        self._after=None
        if not self.playing or not self.model_document():
            self.stop_play();return
        now=time.monotonic()
        elapsed=max(0.0,min(.25,now-self._play_last))
        self._play_last=now
        self._frame_accumulator+=elapsed*self.fps
        advance=int(self._frame_accumulator)
        if advance:
            self._frame_accumulator-=advance
            self._absolute_frame+=advance
            self.frame.set(self._absolute_frame%60)
            self.redraw_models()
        pct=self.speed_percent.get()
        self.play_status.config(text=f"Playing · {pct}%" if self.pose.get().lower()=="run" else "Playing · 30 FPS")
        self._after=self.after(round(1000/self.fps),self._play_tick)

    def reset_orbit(self):
        for view in self.views:view.yaw=.72;view.pitch=.34;view.scale=2.6;view.pan=[0,0]
        self.redraw_models()

    def level_screen(self,x,y):
        return (30+(x-self.world_origin)*self.world_zoom,
                self.level_canvas.winfo_height()*.1+(y-self.world_y_origin)*self.world_zoom)

    def level_world(self,x,y):
        return ((x-30)/self.world_zoom+self.world_origin,
                (y-self.level_canvas.winfo_height()*.1)/self.world_zoom+self.world_y_origin)

    def draw_level(self):
        c=self.level_canvas;c.delete("all")
        if self.kind!="level" or not self.doc:return
        width,height=c.winfo_width(),c.winfo_height()
        scale=self.world_zoom
        self.level_zoom_label.config(text=f"{scale*100:.0f}%")
        # World-aligned grid tracks panning and cursor-centric zooming.
        spacing=100. if scale>.23 else 250. if scale>.08 else 500.
        xa,ya=self.level_world(0,0);xb,yb=self.level_world(width,height)
        for k in range(int(xa//spacing),int(xb//spacing)+2):
            x,_=self.level_screen(k*spacing,0)
            c.create_line(x,0,x,height,fill="#203348")
        for k in range(int(ya//spacing),int(yb//spacing)+2):
            _,y=self.level_screen(0,k*spacing)
            c.create_line(0,y,width,y,fill="#203348")
        select=self.selected_level
        def highlight(key,i,part=None):
            return select is not None and select[0]==key and select[1]==i and (len(select)<3 or select[2]==part)
        def draw_rect(key,i,rect,h=None):
            x,y,w,h0=rect[:4] if h is None else (rect[0],rect[1],rect[2],h)
            a,b=self.level_screen(x,y)
            stroke="#ffe18a" if highlight(key,i) else PALETTE[key]
            c.create_rectangle(a,b,a+w*scale,b+h0*scale,fill=PALETTE[key],outline=stroke,width=3 if highlight(key,i) else 1)
            if highlight(key,i):
                c.create_text(a+4,b+4,anchor="nw",text=f"{key} #{i+1}",fill="#fff0b7")
        # Terrain first; place smaller objects on top for reliable selection.
        for key in ("ground","ledges","cover"):
            for i,rect in enumerate(self.doc.get(key,[])):draw_rect(key,i,rect)
        for i,item in enumerate(self.doc.get("stairs",[])):
            sx,sy=self.level_screen(item[0],item[1]);ex,ey=self.level_screen(item[2],item[3])
            active=highlight("stairs",i)
            c.create_line(sx,sy,ex,ey,fill="#fff0a6" if active else PALETTE["stairs"],width=5 if active else 3)
            c.create_oval(sx-4,sy-4,sx+4,sy+4,fill=PALETTE["stairs"])
            c.create_oval(ex-4,ey-4,ex+4,ey+4,fill=PALETTE["stairs"])
        for i,item in enumerate(self.doc.get("lifts",[])):
            draw_rect("lifts",i,item,15)
            sx,sy=self.level_screen(item[0]+item[2]/2,item[1])
            ex,ey=self.level_screen(item[0]+item[2]/2+item[3],item[1]+item[4])
            c.create_line(sx,sy,ex,ey,fill=PALETTE["lifts"],dash=(3,4),width=2)
        for i,item in enumerate(self.doc.get("relays",[])):
            draw_rect("relays",i,item)
            sx,sy=self.level_screen(item[4],item[5])
            active=highlight("relays",i,"switch")
            c.create_line(sx,sy,*self.level_screen(item[0]+item[2]/2,item[1]+item[3]/2),dash=(4,3),fill="#e8ca84")
            c.create_rectangle(sx-6,sy-6,sx+6,sy+6,fill=PALETTE["relays"],outline="white" if active else "#35291e",width=2)
        for key in ("vents","enemies","supplies","crates","beacons"):
            for i,item in enumerate(self.doc.get(key,[])):
                wx,wy=(item[1],item[2]) if key in ("enemies","supplies","crates") else (item[0],item[1])
                sx,sy=self.level_screen(wx,wy)
                if not(-15<=sx<=width+15 and -15<=sy<=height+15):continue
                active=highlight(key,i)
                if key in ("crates","supplies"):
                    c.create_rectangle(sx-6,sy-6,sx+6,sy+6,fill=PALETTE[key],outline="#fff0a6" if active else "#101927",width=2)
                else:
                    c.create_oval(sx-6,sy-6,sx+6,sy+6,fill=PALETTE[key],outline="#fff0a6" if active else "#122133",width=2)
                if active:c.create_text(sx+9,sy-10,anchor="w",text=f"{key} #{i+1}",fill="#fff0a6")
        for key,objects in (("ground",self.doc.get("ground",[])),("ledges",self.doc.get("ledges",[])),
                            ("cover",self.doc.get("cover",[])),("relays",self.doc.get("relays",[]))):
            for i,_item in enumerate(objects):
                vertices=self.doc.get("structureVertices",{}).get(f"{key}:{i}")
                if vertices is None or len(vertices)<3:continue
                screen=[self.level_screen(*pt) for pt in vertices]
                flattened=[co for pt in screen for co in pt]
                c.create_polygon(*flattened,fill="",outline="#e9b473" if highlight(key,i) else "#7191a2",width=2)
                if self.edit_vertices.get() and highlight(key,i):
                    for v,(sx,sy) in enumerate(screen):
                        active=self.selected_vertex==v
                        c.create_rectangle(sx-6,sy-6,sx+6,sy+6,fill="#fff0a6" if active else "#f4bb83",outline="#16202c")
        for key in ("insertion","exit"):
            if key not in self.doc:continue
            x,y=self.level_screen(*self.doc[key]);active=highlight(key,0)
            c.create_oval(x-9,y-9,x+9,y+9,outline="#ffe18a" if active else PALETTE[key],width=4 if active else 2)
            c.create_text(x,y-14,text=key,fill=PALETTE[key])
        c.create_text(10,8,anchor="nw",fill="#d1e6f5",text=f"{self.path.name}  ·  {int(self.doc.get('worldWidth',0)):,} units")
        if select:
            c.create_text(width-12,12,anchor="ne",fill="#ffe18a",text="SELECTED: "+select[0].upper()+f" #{select[1]+1}")

    def level_start(self,event):
        if self.kind!="level":return
        self.level_canvas.focus_set()
        if event.state & 0x0001 and self.mode.get()=="Select":
            self.level_pan_start(event)
            self._level_space_pan=True
            return
        self._level_space_pan=False
        x,y=self.level_world(event.x,event.y)
        mode=self.mode.get()
        if mode!="Select":
            key=TOOL_KEYS.get(mode)
            if key is None:return
            self.selected_level=create_level_object(self.doc,key,x,y)
            self.dirty=True
            self.mode.set("Select")
        else:
            # In polygon mode vertex handles take precedence over object hit tests.
            self.selected_vertex=None
            if self.edit_vertices.get():
                for key in ("ground","cover","ledges","relays"):
                    for i,_item in enumerate(self.doc.get(key,[])):
                        for v,pt in enumerate(polygon_vertices(self.doc,(key,i))):
                            sx,sy=self.level_screen(*pt)
                            if math.hypot(event.x-sx,event.y-sy)<12:
                                self.selected_level=(key,i)
                                self.selected_vertex=v
                                break
                        if self.selected_vertex is not None:break
                    if self.selected_vertex is not None:break
            if self.selected_vertex is None:
                self.selected_level=hit_level_object(self.doc,x,y,self.world_zoom)
        self._level_last=(event.x,event.y)
        self._update_level_info()
        self.draw_level()

    def level_drag(self,event):
        if self.kind!="level":return
        if getattr(self,"_level_space_pan",False):
            self.level_pan_drag(event)
            return
        if self.selected_level is None or not getattr(self,"_level_last",None):return
        dx=(event.x-self._level_last[0])/self.world_zoom
        dy=(event.y-self._level_last[1])/self.world_zoom
        if self.edit_vertices.get() and self.selected_vertex is not None:
            key,i,*_=self.selected_level
            pts=polygon_vertices(self.doc,self.selected_level)
            p=pts[self.selected_vertex]
            edit_structure_vertex(self.doc,self.selected_level,self.selected_vertex,(p[0]+dx,p[1]+dy))
        else:
            move_level_object(self.doc,self.selected_level,dx,dy)
        self._level_last=(event.x,event.y)
        self.dirty=True;self.draw_level();self._update_level_info()

    def add_selected_vertex(self):
        if not self.selected_level:return
        try:
            points=polygon_vertices(self.doc,self.selected_level)
            if points is None:return
            edge=self.selected_vertex if self.selected_vertex is not None else len(points)-1
            add_structure_vertex(self.doc,self.selected_level,edge)
            self.selected_vertex=(edge+1)% (len(points)+1)
            self.edit_vertices.set(True);self.dirty=True;self.draw_level()
        except (ValueError,IndexError) as exc:self.show_error("Edit corners",str(exc))

    def remove_selected_vertex(self):
        if not self.selected_level:return
        try:
            points=polygon_vertices(self.doc,self.selected_level)
            if points is None:return
            remove_structure_vertex(self.doc,self.selected_level,self.selected_vertex if self.selected_vertex is not None else len(points)-1)
            self.selected_vertex=None;self.dirty=True;self.draw_level()
        except (ValueError,IndexError) as exc:self.show_error("Edit corners",str(exc))

    def add_vertex_at_mouse(self,event):
        if not self.edit_vertices.get() or not self.selected_level:return
        points=polygon_vertices(self.doc,self.selected_level)
        if not points:return
        x,y=self.level_world(event.x,event.y)
        def segdist(a,b):
            dx=b[0]-a[0];dy=b[1]-a[1]
            t=max(0.,min(1.,((x-a[0])*dx+(y-a[1])*dy)/max(.0001,dx*dx+dy*dy)))
            return math.hypot(x-a[0]-t*dx,y-a[1]-t*dy)
        edge=min(range(len(points)),key=lambda i:segdist(points[i],points[(i+1)%len(points)]))
        add_structure_vertex(self.doc,self.selected_level,edge,(x,y))
        self.selected_vertex=edge+1;self.dirty=True;self.draw_level()

    def level_end(self,_event):
        self._level_last=None
        self._level_pan_last=None
        self._level_space_pan=False

    def level_pan_start(self,event):
        if self.kind!="level":return
        self._level_pan_last=(event.x,event.y)
        self._level_last=None

    def level_pan_drag(self,event):
        if self.kind!="level" or not getattr(self,"_level_pan_last",None):return
        dx=event.x-self._level_pan_last[0];dy=event.y-self._level_pan_last[1]
        self.world_origin-=dx/self.world_zoom
        self.world_y_origin-=dy/self.world_zoom
        self._level_pan_last=(event.x,event.y)
        self.draw_level()

    def on_delete_key(self,event):
        if isinstance(self.focus_get(),(tk.Entry,ttk.Entry,tk.Text)):
            return
        if self.tabs.select()!=str(self.level_page):return
        self.delete_level_selected()

    def delete_level_selected(self):
        if self.kind!="level" or self.selected_level is None:return
        if not delete_level_object(self.doc,self.selected_level):
            self.status.set("Spawn and extraction points are required; move them instead of deleting")
            return
        self.dirty=True;self.selected_level=None;self.selected_vertex=None
        self._update_level_info();self.draw_level()

    def _update_level_info(self):
        if not hasattr(self,"level_info") or self.kind!="level":return
        if not self.selected_level:
            self.level_info.config(text="No object selected")
            return
        k,i,*part=self.selected_level
        pt=level_selected_anchor(self.doc,self.selected_level)
        name=f"{k} #{i+1}" + (" · switch" if part else "")
        self.level_info.config(text=f"{name}\nX {pt[0]:.1f}  Y {pt[1]:.1f}\nDrag on the canvas to move; Del to remove")

    def level_wheel(self,event):
        if event.state & 0x0001:
            self.world_origin-=120/self.world_zoom*(1 if event.delta>0 else -1)
            self.draw_level()
        else:self.level_zoom(1 if event.delta>0 else -1,event.x,event.y)

    def level_zoom(self,d,x,y):
        wx,wy=self.level_world(x,y)
        self.world_zoom=max(.012,min(4.0,self.world_zoom*(1.14**d)))
        self.world_origin=wx-(x-30)/self.world_zoom
        self.world_y_origin=wy-(y-self.level_canvas.winfo_height()*.1)/self.world_zoom
        self.draw_level()

    def level_fit(self):
        if self.kind!="level" or not self.doc:return
        width=max(1,int(self.level_canvas.winfo_width())-75)
        height=max(1,int(self.level_canvas.winfo_height())-70)
        self.world_zoom=max(.012,min(4.,width/max(200.,float(self.doc.get("worldWidth",1000)))))
        self.world_origin=-15/self.world_zoom
        self.world_y_origin=0
        self.draw_level()

    def level_focus_selected(self):
        if self.kind!="level" or not self.selected_level:return
        point=level_selected_anchor(self.doc,self.selected_level)
        if point is None:return
        self.world_origin=point[0]-(self.level_canvas.winfo_width()*.5-30)/self.world_zoom
        self.world_y_origin=point[1]-(self.level_canvas.winfo_height()*.5-self.level_canvas.winfo_height()*.1)/self.world_zoom
        self.draw_level()

    def show_asset(self):
        if self.kind!="asset" or not self.path:return
        if Image is None:
            self.image_label.config(text="Install Pillow to preview PNG/WebP images (python3 -m pip install Pillow)")
            return
        try:
            img=Image.open(self.path).convert("RGBA")
            img.thumbnail((780,640))
            self.image_reference=ImageTk.PhotoImage(img)
            self.image_label.config(image=self.image_reference,text="")
        except Exception as exc:self.image_label.config(image="",text=str(exc))
    def replace_asset(self):
        if self.kind!="asset" or not self.path:return
        file=filedialog.askopenfilename(title="Replace art",filetypes=[("Images","*.png *.webp *.jpg")])
        if file:
            if not self.ask_yes_no("Replace image","Overwrite this image directly in game/assets/?"):return
            shutil.copy2(file,self.path);self.show_asset();self.status.set("Image replaced in game source; rebuild to update runtime assets")


def main():
    app=Studio(sys.argv[1:]);app.mainloop()
if __name__=="__main__":main()
