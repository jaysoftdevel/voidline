#!/usr/bin/env python3
"""Source-first model animation migration / action catalog synchronizer.

Models in models/{characters,enemies} are the only editable inputs.  This script
never imports exported C++ or the runtime PAK, and migration never overwrites an
existing authored frame track.  Five evenly-spaced 14-joint keyframes have the
same normalized 0..1 interpolation in Model Studio and in the game renderer.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import shutil

FRAME_COUNT=5
JOINT_COUNT=14
RUNTIME_ACTIONS=frozenset(('idle walk sprint crouch jump fall land fire reload hit death '
                           'patrol alert recoil stagger stairs').split())

def pose_frame(pose: str, t: float):
    """Conservative additive starting curves; player physics/IK stays authoritative."""
    points=[[0.0,0.0] for _ in range(JOINT_COUNT)]
    def shift(ids, dx=0.,dy=0.):
        for i in ids:
            points[i][0]+=dx
            points[i][1]+=dy
    s=t*t*(3-2*t)
    wave=math.sin(t*math.tau)
    pulse=max(0.,math.sin(t*math.pi))
    if pose=='idle':
        shift((1,2,3),dy=-0.65*wave)
        shift((8,9),dy=0.45*wave)
    elif pose in ('walk','sprint','patrol'):
        intensity={'walk':0.6,'sprint':1.1,'patrol':0.5}[pose]
        shift((10,12),dx=1.1*wave*intensity,dy=-0.5*pulse*intensity)
        shift((11,13),dx=-1.1*wave*intensity,dy=0.5*pulse*intensity)
        shift((6,8),dx=-1.1*wave*intensity)
        shift((7,9),dx=1.1*wave*intensity)
        shift((0,1),dy=0.7*math.sin(t*math.tau*2)*intensity)
    elif pose=='stairs':
        shift((10,12),dy=-2.0*max(0.,wave))
        shift((11,13),dy=-2.0*max(0.,-wave))
        shift((6,8),dx=-1.0*wave)
        shift((7,9),dx=1.0*wave)
    elif pose=='crouch':
        shift((0,1),dy=4.5*s)
        shift((2,3),dy=2.8*s,dx=0.7*s)
        shift((10,11),dx=2.3*s,dy=-3.5*s)
        shift((12,13),dx=1.4*s,dy=-1.5*s)
        shift((6,7,8,9),dy=1.0*s)
    elif pose=='jump':
        launch=max(0.,1.-abs(t-.25)/.25)
        tuck=max(0.,1.-abs(t-.65)/.65)
        shift((0,1),dy=2.2*launch-1.1*tuck)
        shift((10,11),dx=2.2*tuck,dy=-3.0*tuck)
        shift((12,13),dy=-6.5*tuck)
        shift((6,8),dy=-2.0*(.5+max(0.,wave)))
        shift((7,9),dy=-1.8*(.5+max(0.,wave)))
    elif pose=='fall':
        reach=math.sin((.25+.75*t)*math.pi)
        shift((10,11),dx=1.6*t,dy=-2.0*reach)
        shift((12,13),dy=-4.0*reach)
        shift((6,8),dx=-2.3*reach,dy=-1.6*reach)
        shift((7,9),dx=2.3*reach,dy=-1.6*reach)
        shift((0,1,2,3),dy=0.8*t)
    elif pose=='land':
        compression=math.sin(math.pi*min(t/.72,1.))**2 if t<.72 else 0.0
        shift((0,1),dy=5.2*compression)
        shift((2,3),dy=3.0*compression)
        shift((10,11),dy=-3.5*compression,dx=1.8*compression)
        shift((12,13),dy=-1.2*compression)
        shift((6,7,8,9),dy=2.0*compression)
    elif pose in ('alert','fire','recoil','reload'):
        if pose=='alert': amt=s
        elif pose in ('fire','recoil'): amt=max(0.,1.-abs(t-.25)/.25)
        else: amt=max(0.,math.sin(t*math.pi))
        shift((6,7),dx=-1.5*amt,dy=-1.2*amt)
        shift((8,9),dx=(-2.6 if pose in ('fire','recoil') else 1.)*amt,dy=-2.4*amt)
        if pose=='reload': shift((9,),dy=4.0*amt)
        if pose in ('fire','recoil'): shift((1,2),dx=-0.7*amt)
    elif pose in ('hit','stagger','death'):
        amp={'hit':2.5,'stagger':3.6,'death':5.5}[pose]
        amt=s if pose=='death' else max(0.,math.sin(t*math.pi))
        shift((1,2,3),dx=-amp*amt,dy=1.2*amt)
        shift((8,9),dy=amp*amt)
        if pose=='death':shift((0,),dy=2.5*s)
    return [[round(x,3),round(y,3)] for x,y in points]

def generate_frames(pose):
    return [pose_frame(pose,i/(FRAME_COUNT-1)) for i in range(FRAME_COUNT)]

def json_write_if_changed(path: Path, data) -> bool:
    output=json.dumps(data,indent=2,ensure_ascii=False)+'\n'
    if path.is_file() and path.read_text(encoding='utf-8')==output:
        return False
    path.parent.mkdir(parents=True,exist_ok=True)
    temporary=path.with_name(path.name+'.new')
    temporary.write_text(output,encoding='utf-8')
    temporary.replace(path)
    return True

def migrate(root: Path, backup: Path):
    count=0
    for category,suffix in (('characters','vlcharacter.json'),('enemies','vlenemy.json')):
        for path in sorted((root/'models'/category).glob('*.'+suffix)):
            doc=json.loads(path.read_text(encoding='utf-8'))
            rig=doc.get('skeleton',{})
            layers=rig.get('sequences',{})
            if not isinstance(layers,dict) or not layers:
                continue  # procedural enemies have no authored humanoid poses
            frames=rig.get('poseFrames')
            if frames is None:
                frames={}
                rig['poseFrames']=frames
            if not isinstance(frames,dict):
                raise ValueError(f'{path}: poseFrames must be an object')
            missing=[name for name in layers if name not in frames]
            if not missing:
                continue
            for name in missing:
                frames[name]=generate_frames(name)
            before=backup/path.relative_to(root)
            if not before.exists():
                before.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(path,before)
            json_write_if_changed(path,doc)
            count+=1
            print(f'ADDED_KEYFRAMES {path.relative_to(root)} ({len(missing)} motions)')
    # Describe actual game-selected actions instead of the obsolete
    # 'editor_only' flag; preserve all user-written labels and descriptions.
    descriptor=root/'models'/'movement-descriptions.json'
    if descriptor.is_file():
        definitions=json.loads(descriptor.read_text(encoding='utf-8'))
        changed=False
        for entry in definitions.get('movements',[]):
            if entry.get('id') in RUNTIME_ACTIONS and entry.get('runtime')!='game_and_editor':
                entry['runtime']='game_and_editor'
                changed=True
        if changed:
            before=backup/descriptor.relative_to(root)
            if not before.exists():
                before.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(descriptor,before)
            json_write_if_changed(descriptor,definitions)
            print('UPDATED_DESCRIPTORS',descriptor.relative_to(root))
    return count

def catalog(root: Path, category: str):
    is_enemy=category=='enemies'
    files=sorted((root/'models'/category).glob('*.'+('vlenemy' if is_enemy else 'vlcharacter')+'.json'),
                 key=lambda p:int(p.name.split('.')[0]))
    expected=10 if is_enemy else 6
    if len(files)!=expected or [int(f.name.split('.')[0]) for f in files]!=list(range(expected)):
        raise ValueError(f'Expected {expected} numbered source models in models/{category}')
    descriptor=json.loads((root/'models'/'movement-descriptions.json').read_text(encoding='utf-8'))
    models=[]
    for f in files:
        model=json.loads(f.read_text(encoding='utf-8'))
        rig=model['skeleton']
        tracks=rig.get('poseFrames',{})
        seq=rig.get('sequences',{})
        if not isinstance(tracks,dict):raise ValueError(f'{f}: invalid poseFrames')
        for pose,frames in tracks.items():
            if pose not in seq:raise ValueError(f'{f}: keyframe pose {pose} is not listed in sequences')
            if not isinstance(frames,list) or len(frames)!=FRAME_COUNT:
                raise ValueError(f'{f}: {pose} needs {FRAME_COUNT} keyframes')
            for frame in frames:
                if not isinstance(frame,list) or len(frame)!=JOINT_COUNT:
                    raise ValueError(f'{f}: {pose} needs {JOINT_COUNT} joints per keyframe')
                for pair in frame:
                    if (not isinstance(pair,list) or len(pair)!=2 or
                        any(type(v) not in (int,float) or not math.isfinite(v) or abs(v)>65536 for v in pair)):
                        raise ValueError(f'{f}: {pose} has invalid keyframe vector')
        models.append({'id':int(model['id']),'sequences':seq,
                       'sequenceDepth':rig['sequenceDepth'],'poseFrames':tracks})
    return {'schema':'voidline-actions-v1','definitions':descriptor['movements'],'models':models}

def sync(root: Path):
    count=0
    for category in ('characters','enemies'):
        path=root/'assets'/'models'/f'{category}.actions.json'
        if json_write_if_changed(path,catalog(root,category)):
            count+=1
            print('UPDATED_ACTION_CATALOG',path.relative_to(root))
    return count

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('root',type=Path)
    parser.add_argument('--migrate',action='store_true')
    parser.add_argument('--sync',action='store_true')
    args=parser.parse_args()
    root=args.root.resolve()
    if args.migrate:
        migrate(root,root/'update-backups'/'v2.10.27')
    if args.sync:
        sync(root)
    if not (args.migrate or args.sync):parser.error('Specify --migrate or --sync')
if __name__=='__main__':main()
