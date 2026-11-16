#!/usr/bin/env python3
"""Safely clear stairs beside each level's exit; preserve everything else.

Usage: clear-exit-stairs.py GAME_ROOT [BACKUP_DIR]
Only changed JSON files are backed up, and subsequent runs do nothing.
"""
import json
from pathlib import Path
import shutil
import sys


def close_to_gate(stair, exit_point):
    x1, y1, x2, y2 = stair
    ex, ey = exit_point
    return (max(x1,x2) >= ex-105 and min(x1,x2) <= ex+35
            and min(abs(y1-ey),abs(y2-ey)) < 160)


def clear(root, backup):
    changed=[]
    for path in sorted((root/'levels').glob('*.vllevel.json')):
        data=json.loads(path.read_text(encoding='utf-8'))
        original=data.get('stairs',[])
        if not isinstance(original,list):
            raise ValueError(f'Invalid stairs in {path}')
        stairs=[stair for stair in original if not close_to_gate(stair,data['exit'])]
        if len(stairs)==len(original):continue
        destination=backup/'levels'/path.name
        destination.parent.mkdir(parents=True,exist_ok=True)
        if not destination.exists():shutil.copy2(path,destination)
        data['stairs']=stairs
        path.write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
        changed.append((path.name,len(original)-len(stairs)))
    for filename,count in changed:
        print(f'EXIT CLEARANCE {filename}: removed {count} blocking stair(s)')
    return changed


if __name__=='__main__':
    if not 2<=len(sys.argv)<=3:
        raise SystemExit('Usage: clear-exit-stairs.py GAME_ROOT [BACKUP_DIR]')
    base=Path(sys.argv[1]).resolve()
    target=Path(sys.argv[2]).resolve() if len(sys.argv)==3 else base/'update-backups'/'v2.10.31'
    clear(base,target)
