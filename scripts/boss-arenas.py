#!/usr/bin/env python3
"""One-time, idempotent source-first migration of the 26 guardian arenas.

Usage: python3 scripts/boss-arenas.py GAME_ROOT --migrate [--backup PATH]
       python3 scripts/boss-arenas.py GAME_ROOT --verify
Does not touch models, assets, saves or game.vpak.
"""
import argparse
import json
from pathlib import Path
import shutil
import sys

REVISION=28

def modify(doc):
    if doc.get("bossArenaRevision",0)>=REVISION:
        return False
    bosses=[enemy for enemy in doc["enemies"] if int(enemy[0])==4]
    if not bosses: return False  # Respect levels where the editor intentionally removed the guardian.
    if len(bosses)!=1: raise ValueError("A level may only contain one guardian")
    boss=bosses[0]
    coop=bool(doc["cooperative"])
    deck=doc["ground"][-1]
    floor=boss[2]
    extra=320. if coop else max(0.,1080.-deck[2])
    deck[2]+=extra
    doc["worldWidth"]+=extra
    doc["exit"][0]+=extra
    arena_left=doc["worldWidth"]-1120. if coop else deck[0]
    arena_width=1120. if coop else deck[2]
    boss[1]=arena_left+(arena_width*.61 if not coop else 690.)
    boss[2]=floor
    boss[3]=arena_width*.42
    for lane in range(2):
        base=arena_left+(120. if coop else 125.)+lane*(710. if coop else arena_width-385.)
        rise=88. if lane==0 else 116.
        doc["ledges"].append([round(base+109.,3),round(floor-rise,3),148.,17.])
        doc["stairs"].append([round(base,3),round(floor,3),round(base+148.,3),round(floor-rise,3)])
    # Preserve the source file's original obstacles outside the final arena.
    # Remap optional editor polygon indices when an old floor-level wall goes.
    old_cover=doc["cover"]
    keep=[]; remap={}
    for old_idx,wall in enumerate(old_cover):
        in_arena=(arena_left+95.<=wall[0]<doc["worldWidth"]-90. and
                  floor-85.<=wall[1]<=floor+1.)
        if not in_arena:
            remap[old_idx]=len(keep)
            keep.append(wall)
    doc["cover"]=keep
    if "structureVertices" in doc:
        polygons={}
        for key,vertices in doc["structureVertices"].items():
            if key.startswith("cover:"):
                idx=int(key.partition(":")[2])
                if idx not in remap: continue
                key=f"cover:{remap[idx]}"
            polygons[key]=vertices
        doc["structureVertices"]=polygons
    doc["bossArenaRevision"]=REVISION
    return True

def verify(doc):
    if doc.get("bossArenaRevision")!=REVISION: raise ValueError("Not migrated")
    bosses=[enemy for enemy in doc["enemies"] if int(enemy[0])==4]
    if not bosses: return  # An editor-authored boss-free level is intentional.
    if len(bosses)!=1: raise ValueError("Multiple guardians in a single arena")
    boss=bosses[0]
    if not (doc["worldWidth"]-1100. < boss[1] < doc["exit"][0]):
        raise ValueError("Boss outside arena")
    floor=boss[2]
    if not any(abs(x[1]-(floor-88.))<.1 for x in doc["ledges"]):
        raise ValueError("Missing arena high route")
    for slope in doc["stairs"][-2:]:
        x0,y0,x1,y1=slope
        if x1-x0 < 48 or abs(y1-y0)>(x1-x0)*.95: raise ValueError("Invalid arena stair")
        for x,y in [(x0,y0),(x1,y1)]:
            if not any(abs(platform[1]-y)<.5 and
                       platform[0]+27 <= x <= platform[0]+platform[2]-27
                       for platform in doc["ground"]+doc["ledges"]):
                raise ValueError("Arena stair end has no supporting platform")

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("root",type=Path)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--migrate",action="store_true")
    mode.add_argument("--verify",action="store_true")
    parser.add_argument("--backup",type=Path)
    args=parser.parse_args()
    levels=sorted((args.root/"levels").glob("*.vllevel.json"))
    if len(levels)!=26: raise ValueError(f"Expected 26 levels, found {len(levels)}")
    count=0
    for path in levels:
        doc=json.loads(path.read_text())
        if args.migrate and modify(doc):
            verify(doc)
            if args.backup:
                target=args.backup/path.name
                target.parent.mkdir(parents=True,exist_ok=True)
                if not target.exists(): shutil.copy2(path,target)
            path.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+"\n")
            count+=1
        elif args.verify:
            verify(doc)
    print(f"Guardian arenas: {len(levels)} checked, {count} changed")

if __name__=="__main__":
    try: main()
    except (ValueError,OSError,KeyError) as exc:
        print(f"Guardian arena migration failed: {exc}",file=sys.stderr)
        sys.exit(1)
