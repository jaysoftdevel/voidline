"""Headless editor geometry and animation rules shared by the Linux GUI and tests.

Stage objects retain VOIDLINE's source JSON layout. No transforms are applied to
model data on load; only explicit drag/edit commands change a model.
"""
from __future__ import annotations
from math import hypot, isfinite
import copy
import re
from pathlib import Path

# These four archetypes are rendered with the game Humanoid joint rig.
# Other enemies use distinct procedural renderers and do not consume humanoid
# per-joint offsets; keep their existing data intact but disable misleading edits.
# Explicit states used by the game renderer / simulation. The existing Base, Aim,
# Run layers remain compatible with VLRM/VLEM v2. Per-action authored offsets
# are an additional, optional baked payload, not animation keyframes.
ACTION_STATES = ("idle", "walk", "sprint", "crouch", "jump", "fall", "land",
                 "fire", "reload", "hit", "death", "patrol", "chase",
                 "alert", "recoil", "stagger", "stairs", "interact")
LEGACY_STATES = ("base", "aim", "run")
CHARACTER_STATES = ("base", "idle", "walk", "run", "sprint", "crouch", "jump",
                    "fall", "land", "stairs", "aim", "fire", "reload", "hit", "death", "interact")
SOLDIER_STATES = ("base", "idle", "patrol", "run", "chase", "stairs", "aim",
                  "alert", "fire", "recoil", "hit", "stagger", "death")
PROCEDURAL_STATES = {
    0:("idle", "patrol", "chase", "alert", "fire", "hit", "death"),
    2:("idle", "alert", "fire", "recoil", "hit", "death"),
    3:("idle", "patrol", "chase", "jump", "fall", "land", "hit", "death"),
    4:("idle", "patrol", "chase", "alert", "fire", "hit", "death"),
    5:("idle", "patrol", "chase", "jump", "hit", "death"),
    8:("idle", "patrol", "chase", "alert", "fire", "recoil", "hit", "death")
}

def movement_sequences(kind, doc):
    if kind == "character": return CHARACTER_STATES
    if kind == "enemy":
        try: ident = int(doc.get('id', -1))
        except (ValueError,TypeError): ident = -1
        return SOLDIER_STATES if ident in (1,6,7,9) else PROCEDURAL_STATES.get(ident, ("idle", "patrol", "hit", "death"))
    return ()

def ensure_sequences(doc):
    skeleton=doc.setdefault('skeleton',{})
    seq=skeleton.setdefault('sequences',{})
    depth=skeleton.setdefault('sequenceDepth',{})
    # Keep only real authoring records: don't overwrite custom vectors on open.
    for state in ACTION_STATES:
        if state not in seq: seq[state]=[[0.0,0.0] for _ in range(14)]
        if state not in depth: depth[state]=[0.0 for _ in range(14)]
    return doc

def create_model_from_template(template, kind, name, new_id):
    if kind not in ('character','enemy'): raise ValueError('Unknown model type')
    if not name.strip() or len(name)>96:raise ValueError('Name must be 1-96 characters')
    result=copy.deepcopy(template)
    result['name']=name.strip()
    result['id']=int(new_id)
    result['templateOf']=int(template.get('id',0))
    ensure_sequences(result)
    return result

def assign_model_to_slot(prototype, slot):
    result=copy.deepcopy(prototype)
    result['id']=int(slot)
    result.pop('templateOf',None)
    return result

def polygon_vertices(doc, selection):
    if not selection:return None
    key,index,*_=selection
    if key not in ('ground','ledges','cover','relays'): return None
    mesh=doc.get('structureVertices',{})
    k=f'{key}:{index}'
    if k in mesh:return mesh[k]
    a=doc[key][index]
    return [[a[0],a[1]],[a[0]+a[2],a[1]],[a[0]+a[2],a[1]+a[3]],[a[0],a[1]+a[3]]]

def edit_structure_vertex(doc, selection, vertex, xy):
    if selection is None or selection[0] not in ('ground','ledges','cover','relays'):
        raise ValueError('Select a rectangular structure')
    verts=polygon_vertices(doc,selection)
    if not 0 <= vertex < len(verts):raise IndexError(vertex)
    if not all(isfinite(float(c)) and -200000 < float(c) < 200000 for c in xy):raise ValueError('Invalid vertex')
    updated=[list(p) for p in verts]
    updated[vertex]=list(map(float,xy))
    doc.setdefault('structureVertices',{})[f'{selection[0]}:{selection[1]}']=updated
    return updated

def add_structure_vertex(doc, selection, edge, xy=None):
    verts=polygon_vertices(doc,selection)
    if verts is None:raise ValueError('Structure does not have editable vertices')
    if len(verts)>=32:raise ValueError('Maximum 32 vertices per structure')
    edge=int(edge)%len(verts)
    a,b=verts[edge],verts[(edge+1)%len(verts)]
    pos=xy if xy is not None else ((a[0]+b[0])/2,(a[1]+b[1])/2)
    updated=[list(p) for p in verts]
    updated.insert(edge+1,list(map(float,pos)))
    doc.setdefault('structureVertices',{})[f'{selection[0]}:{selection[1]}']=updated
    return updated

def remove_structure_vertex(doc, selection, vertex):
    verts=polygon_vertices(doc,selection)
    if verts is None or len(verts)<=3:raise ValueError('Structure must have at least 3 corners')
    updated=[list(p) for p in verts]
    updated.pop(vertex)
    doc.setdefault('structureVertices',{})[f'{selection[0]}:{selection[1]}']=updated
    return updated

HUMANOID_ENEMIES = frozenset((1, 6, 7, 9))
ENEMY_NAMES = ("Drone", "Ranger", "Turret", "Stalker", "Guardian", "Scavenger",
               "Sniper", "Bulwark", "Interceptor", "Juggernaut")
RECTS = ("ground", "ledges", "cover")
POINTS = ("beacons", "vents")
TYPED_POINTS = ("enemies", "supplies", "crates")
OBJECT_LAYERS = ("relays", "enemies", "supplies", "crates", "vents", "beacons",
                 "stairs", "lifts", "cover", "ledges", "insertion", "exit", "ground")
TOOL_KEYS = {"Ground":"ground", "Ledge":"ledges", "Cover":"cover", "Stair":"stairs",
             "Lift":"lifts", "Enemy":"enemies", "Checkpoint":"beacons",
             "Supply":"supplies", "Crate":"crates", "Vent":"vents",
             "Door + Switch":"relays", "Spawn":"insertion", "Exit":"exit"}


def can_edit_joint(kind, doc):
    if kind == "character":
        return True
    if kind != "enemy":
        return False
    try:
        return int(doc.get("id", -1)) in HUMANOID_ENEMIES
    except (ValueError, TypeError):
        return False


def rig_status(kind, doc):
    if kind == "character":
        return "Articulated character · all 14 skeleton joints editable"
    if kind == "enemy":
        if can_edit_joint(kind, doc):
            return "Humanoid enemy · all 14 skeleton joints editable"
        return "Procedural enemy · joint editing disabled (game renderer does not use humanoid joints)"
    return "No rig"


def run_cycles_per_second(doc, kind):
    """Match VOIDLINE's actual run gait calculation, in cycles per second.

    Characters: sprint speed 1.28 × character speed; the on-ground running
    stride is 118 logical units × model.stride / model.walkCycle.
    Soldiers: drawSoldier() uses distance / 56 × model.walkCycle.
    """
    skeleton = doc.get("skeleton", {})
    stride = max(.05, float(skeleton.get("stride", 1)))
    walk_cycle = max(.05, float(doc.get("motion", {}).get("walkCycle", 1)))
    speed = max(0.0, float(doc.get("speed", 350 if kind == "character" else 90)))
    if kind == "character":
        return (speed * 1.28 * walk_cycle) / (118.0 * stride)
    return speed * walk_cycle / 56.0


def run_phase(doc, kind, absolute_frame, speed_percent=100):
    from math import tau
    return tau * max(0.0, float(absolute_frame)) / 30.0 * run_cycles_per_second(doc, kind) * max(5, min(200, speed_percent))/100.0


def point_coordinates(key, obj):
    if key in TYPED_POINTS:
        return float(obj[1]),float(obj[2])
    return float(obj[0]),float(obj[1])


def level_selected_anchor(doc, selection):
    if not selection: return None
    key, idx, *part = selection
    if key in ("insertion","exit"):
        return tuple(doc[key][:2])
    item=doc[key][idx]
    if key=="relays" and part and part[0]=="switch": return (item[4],item[5])
    if key in RECTS or key=="lifts":return (item[0]+item[2]/2,item[1]+(item[3] if key in RECTS else 15)/2)
    if key=="relays":return (item[0]+item[2]/2,item[1]+item[3]/2)
    if key=="stairs":return ((item[0]+item[2])/2,(item[1]+item[3])/2)
    return point_coordinates(key,item)


def _segment_distance(x,y,x1,y1,x2,y2):
    dx=x2-x1;dy=y2-y1
    t=max(0,min(1,((x-x1)*dx+(y-y1)*dy)/max(.001,dx*dx+dy*dy)))
    return hypot(x-x1-t*dx,y-y1-t*dy)


def hit_level_object(doc, x, y, zoom=1):
    """Find the top-most physical object at world coordinates; selection can
    start from any visible part of a rectangle, step, or control handle.
    """
    pad=max(4.,9./max(.012,zoom))
    for key in OBJECT_LAYERS:
        if key not in doc: continue
        if key in ("insertion","exit"):
            if hypot(x-doc[key][0],y-doc[key][1])<=pad*1.3:return (key,0)
            continue
        group=doc[key]
        for i in range(len(group)-1,-1,-1):
            v=group[i]
            if key=="relays":
                if hypot(x-v[4],y-v[5])<=pad*1.4:return (key,i,"switch")
                if v[0]-pad*.3<=x<=v[0]+v[2]+pad*.3 and v[1]-pad*.3<=y<=v[1]+v[3]+pad*.3:return (key,i)
            elif key=="stairs":
                if _segment_distance(x,y,*v[:4])<=pad:return (key,i)
            elif key in RECTS or key=="lifts":
                h=v[3] if key in RECTS else 15
                if v[0]-pad*.25<=x<=v[0]+v[2]+pad*.25 and v[1]-pad*.25<=y<=v[1]+h+pad*.25:return (key,i)
            else:
                px,py=point_coordinates(key,v)
                if hypot(x-px,y-py)<=pad*1.4:return (key,i)
    return None


def move_level_object(doc, selection, dx, dy):
    if not selection:return
    key,i,*part=selection
    if key in ("insertion","exit"):
        doc[key][0]+=dx;doc[key][1]+=dy
        return
    item=doc[key][i]
    if key=="relays" and part and part[0]=="switch":
        item[4]+=dx;item[5]+=dy
    elif key=="relays":
        item[0]+=dx;item[1]+=dy;item[4]+=dx;item[5]+=dy
    elif key=="stairs":
        item[0]+=dx;item[1]+=dy;item[2]+=dx;item[3]+=dy
    elif key in TYPED_POINTS:
        item[1]+=dx;item[2]+=dy
    else:
        item[0]+=dx;item[1]+=dy
    if key in ("ground","ledges","cover","relays"):
        corners=doc.get("structureVertices",{}).get(f"{key}:{i}")
        if corners:
            for point in corners:
                point[0]+=dx;point[1]+=dy


def create_level_object(doc,key,x,y):
    if key in ("insertion","exit"):
        doc[key]=[x,y];return (key,0)
    defaults={
        "ground":[x,y,280.,110.], "ledges":[x,y,140.,17.],
        "cover":[x,y,44.,43.], "stairs":[x,y,x+155.,y-84.],
        "lifts":[x,y,108.,0.,-70.,3.2,0.], "vents":[x,y,0.],
        "enemies":[1,x,y,52.,0], "beacons":[x,y],
        "supplies":[0,x,y],"crates":[0,x,y],
        "relays":[x,y-140.,52.,140.,x-95.,y,0,0]
    }
    doc.setdefault(key,[]).append(defaults[key])
    return (key,len(doc[key])-1)


def delete_level_object(doc,selection):
    if not selection:return False
    key,i,*_=selection
    if key in ("insertion","exit"):return False
    doc[key].pop(i)
    # Keep polygons indexed with their structures when a structure is deleted.
    if key in ("ground","ledges","cover","relays"):
        original=doc.get("structureVertices",{})
        updated={}
        for label,verts in original.items():
            try: layer,number=label.rsplit(':',1);number=int(number)
            except (ValueError,TypeError):continue
            if layer==key and number==i:continue
            updated[f"{layer}:{number-1 if layer==key and number>i else number}"]=verts
        doc["structureVertices"]=updated
    return True


def edit_scalar_property(doc, group, key, text):
    """Update every scalar authoring property while protecting the file identity.

    The schema/id are structural references used by baked model slots; they
    cannot safely be changed from a property entry.
    """
    if group is None and key in ("id","schema"):
        raise ValueError("Model slot and schema cannot be renamed")
    target=doc if group is None else doc[group]
    old=target[key]
    if isinstance(old, bool):
        if text.strip().lower() not in ("true", "false", "1", "0"):
            raise ValueError("Expected a boolean")
        new=text.strip().lower() in ("true", "1")
    elif isinstance(old,str):
        new=text
        if key=="name" and not new.strip():
            raise ValueError("Model name must not be empty")
    elif isinstance(old,int):
        num=float(text)
        if not isfinite(num) or not num.is_integer():
            raise ValueError("Expected an integer")
        new=int(num)
    elif isinstance(old,float):
        new=float(text)
        if not isfinite(new):
            raise ValueError("Expected a finite numeric value")
    else:
        raise ValueError("Property is not scalar")
    if old==new:return False
    target[key]=new
    return True
