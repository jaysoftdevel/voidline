"""Headless editor geometry and animation rules shared by the Linux GUI and tests.

Stage objects retain VOIDLINE's source JSON layout. No transforms are applied to
model data on load; only explicit drag/edit commands change a model.
"""
from __future__ import annotations
from math import hypot, isfinite
import copy
import re
from pathlib import Path

# The movement-description JSON is an editable INPUT, not a generated PAK file.
# Source workspaces import it to their models/ directory and may extend it.
_DEFAULT_MOVEMENT_CATALOG = Path(__file__).resolve().parents[1] / "starter-project/models/movement-descriptions.json"
LEGACY_STATES = ("base", "aim", "run")
HUMANOID_ENEMIES = frozenset((1, 6, 7, 9))

def movement_definitions(catalog=None):
    import json
    path = Path(catalog) if catalog else _DEFAULT_MOVEMENT_CATALOG
    if not path.is_file():
        path = _DEFAULT_MOVEMENT_CATALOG
    data = json.loads(path.read_text(encoding="utf-8"))
    moves = data.get("movements", [])
    ids = [entry["id"] for entry in moves]
    if len(ids) != len(set(ids)) or not all(re.fullmatch(r"[a-z][a-z0-9_]*", name) for name in ids):
        raise ValueError("Movement descriptor has duplicate or invalid state names")
    return moves

ACTION_STATES = tuple(item["id"] for item in movement_definitions())

def movement_sequences(kind, doc, catalog=None):
    if kind not in ("character", "enemy"):
        return ()
    # Expose every authored state for every model. Procedural enemies still
    # cannot have their unused humanoid joints dragged (see can_edit_joint).
    return LEGACY_STATES + tuple(entry["id"] for entry in movement_definitions(catalog)
                                 if entry["id"] not in LEGACY_STATES)

def ensure_sequences(doc, catalog=None):
    skeleton = doc.setdefault("skeleton", {})
    seq = skeleton.setdefault("sequences", {})
    depth = skeleton.setdefault("sequenceDepth", {})
    for state in (item["id"] for item in movement_definitions(catalog)):
        seq.setdefault(state, [[0.0, 0.0] for _ in range(14)])
        depth.setdefault(state, [0.0 for _ in range(14)])
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

# Clipboard data uses the OS clipboard only. No model snapshots are cached on disk.
CLIPBOARD_FORMAT = "voidline-model-studio-clipboard-v1"

def _validated_pose(data):
    if not isinstance(data, dict) or set(data) != {"xy", "depth"}:
        raise ValueError("Clipboard movement must contain XY and depth vectors")
    xy, depth = data["xy"], data["depth"]
    if not isinstance(xy, list) or len(xy) != 14 or not isinstance(depth, list) or len(depth) != 14:
        raise ValueError("Movement must have exactly 14 joints and 14 depth offsets")
    for vector in xy:
        if not isinstance(vector, list) or len(vector) != 2:
            raise ValueError("Every joint requires X and Y offsets")
        if any(isinstance(number, bool) or not isinstance(number, (int,float)) or
               not isfinite(number) or abs(number)>65536 for number in vector):
            raise ValueError("Movement contains invalid joint coordinates")
    if any(isinstance(number, bool) or not isinstance(number, (int,float)) or
           not isfinite(number) or abs(number)>65536 for number in depth):
        raise ValueError("Movement contains invalid depth coordinates")
    return copy.deepcopy(xy),copy.deepcopy(depth)

def copy_model_payload(doc, kind):
    """Encode the full editable model in a versioned, system-clipboard format."""
    import json
    if kind not in ("character", "enemy") or not isinstance(doc, dict) or not isinstance(doc.get("skeleton"), dict):
        raise ValueError("Select a character or enemy model to copy")
    return json.dumps({"format":CLIPBOARD_FORMAT,"operation":"model","modelType":kind,
                       "data":doc},ensure_ascii=False,allow_nan=False)

def paste_model_payload(payload, destination, kind):
    """Return a new model, preserving the destination's file identity/slot."""
    import json
    if kind not in ("character", "enemy") or not isinstance(destination,dict):
        raise ValueError("Select a destination model")
    if len(payload)>6_000_000:raise ValueError("Clipboard model is too large")
    try:clip=json.loads(payload)
    except (ValueError,TypeError) as exc:raise ValueError("Clipboard does not contain valid model JSON") from exc
    if not isinstance(clip,dict) or clip.get("format")!=CLIPBOARD_FORMAT or clip.get("operation")!="model":
        raise ValueError("Clipboard does not contain a VOIDLINE model; use Copy Model")
    if clip.get("modelType")!=kind:
        raise ValueError("Cannot paste a different model type into this file")
    incoming=clip.get("data")
    if (not isinstance(incoming,dict) or not isinstance(incoming.get("skeleton"),dict)
            or not isinstance(incoming.get("geometry"),dict)
            or not isinstance(incoming.get("motion"),dict)):
        raise ValueError("Clipboard model has no valid skeleton/geometry/motion")
    skeleton=incoming["skeleton"]
    for pose_name in LEGACY_STATES:
        _validated_pose({"xy":skeleton.get(pose_name),
                         "depth":skeleton.get("depth",{}).get(pose_name)})
    result=copy.deepcopy(incoming)
    # ID and schema belong to the selected source file, not the copied model.
    for key in ("id","schema"):
        if key in destination:result[key]=destination[key]
    result.pop("templateOf",None)
    return result

def copy_movement_payload(doc, pose):
    import json
    if not isinstance(doc,dict) or not isinstance(doc.get("skeleton"),dict):
        raise ValueError("Select a model before copying a movement")
    rig=doc["skeleton"]
    if pose in LEGACY_STATES:
        xy=rig.get(pose);depth=rig.get("depth",{}).get(pose)
    else:
        xy=rig.get("sequences",{}).get(pose);depth=rig.get("sequenceDepth",{}).get(pose)
    _validated_pose({"xy":xy,"depth":depth})
    return json.dumps({"format":CLIPBOARD_FORMAT,"operation":"movement", "sourcePose":pose,
                       "data":{"xy":xy,"depth":depth}},allow_nan=False)

def paste_movement_payload(payload, destination, pose):
    """Return an updated copy, mapping copied joint offsets into the selected pose."""
    import json
    if len(payload)>100_000:raise ValueError("Clipboard movement is too large")
    try:clip=json.loads(payload)
    except (ValueError,TypeError) as exc:raise ValueError("Clipboard does not contain valid movement JSON") from exc
    if not isinstance(clip,dict) or clip.get("format")!=CLIPBOARD_FORMAT or clip.get("operation")!="movement":
        raise ValueError("Clipboard does not contain a VOIDLINE movement; use Copy Movement")
    if not isinstance(destination,dict) or not isinstance(destination.get("skeleton"),dict):
        raise ValueError("Select a destination model")
    xy,depth=_validated_pose(clip.get("data"))
    result=copy.deepcopy(destination)
    rig=result["skeleton"]
    if pose in LEGACY_STATES:
        rig[pose]=xy;rig.setdefault("depth",{})[pose]=depth
    elif isinstance(pose,str) and re.fullmatch(r"[a-z][a-z0-9_]*",pose):
        rig.setdefault("sequences",{})[pose]=xy
        rig.setdefault("sequenceDepth",{})[pose]=depth
    else:raise ValueError("Choose a valid destination movement")
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
