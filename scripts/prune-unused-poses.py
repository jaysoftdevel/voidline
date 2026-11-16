#!/usr/bin/env python3
"""v2.10.24 one-time migration: prune unused model poses without losing edits.

Run from the update installer. All touched files are backed up in the checkout.
The game source is authoritative; runtime PAK updates are unnecessary.
"""
import argparse
import json
from pathlib import Path
import shutil
import sys

PLAYER = ('idle', 'walk', 'sprint', 'crouch', 'jump', 'fall', 'land',
          'fire', 'reload', 'hit', 'death', 'alert', 'stairs')
SOLDIER = ('idle', 'patrol', 'stairs', 'alert', 'fire', 'recoil', 'hit', 'stagger', 'death')
SOLDIER_IDS = frozenset((1, 6, 7, 9))


def migrate(root, backup):
    count = 0

    def store(file, new):
        nonlocal count
        if not file.is_file():
            return
        updated = json.dumps(new, ensure_ascii=False, indent=2) + '\n'
        if json.loads(file.read_text(encoding='utf-8')) == new:
            return
        relative = file.relative_to(root)
        destination = backup / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists():
            shutil.copy2(file, destination)
        temporary = file.with_name(file.name + '.vlpose-tmp')
        temporary.write_text(updated, encoding='utf-8')
        temporary.replace(file)
        count += 1
        print(f'POSES_UPDATED\t{relative}')

    for base in (root / 'models', root / 'model-studio/starter-project/models'):
        if not base.is_dir():
            continue
        for group, extension, total in (('characters', 'vlcharacter', 6), ('enemies', 'vlenemy', 10)):
            for i in range(total):
                file = base / group / f'{i}.{extension}.json'
                if not file.is_file():
                    raise FileNotFoundError(f'Missing model source {file}')
                doc = json.loads(file.read_text(encoding='utf-8'))
                if doc.get('id') != i or not isinstance(doc.get('skeleton'), dict):
                    raise ValueError(f'Invalid model identity/skeleton in {file}')
                skeleton = doc['skeleton']
                seq = skeleton.get('sequences', {})
                depth = skeleton.get('sequenceDepth', {})
                if not isinstance(seq, dict) or not isinstance(depth, dict):
                    raise ValueError(f'Invalid movement sequences in {file}')
                keep = PLAYER if group == 'characters' else SOLDIER if i in SOLDIER_IDS else ()
                # Do not synthesize any missing poses; preserve every authored
                # offset for retained poses and all unrelated model properties.
                skeleton['sequences'] = {name: seq[name] for name in keep if name in seq and name in depth}
                skeleton['sequenceDepth'] = {name: depth[name] for name in keep if name in seq and name in depth}
                store(file, doc)
    # Generate the published action catalogs from the actual source models.
    # No action states are invented for a model lacking that pose.
    definition_file = root / 'models/movement-descriptions.json'
    definitions = json.loads(definition_file.read_text(encoding='utf-8')).get('movements', [])
    for group, extension, total, filename in (
            ('characters', 'vlcharacter', 6, 'characters.actions.json'),
            ('enemies', 'vlenemy', 10, 'enemies.actions.json')):
        models = []
        for i in range(total):
            doc = json.loads((root / 'models' / group / f'{i}.{extension}.json').read_text(encoding='utf-8'))
            sk = doc['skeleton']
            models.append({'id': i, 'sequences': sk.get('sequences', {}),
                           'sequenceDepth': sk.get('sequenceDepth', {})})
        catalog = {'schema': 'voidline-actions-v1', 'models': models, 'definitions': definitions}
        output = root / 'assets/models' / filename
        if output.is_file():
            store(output, catalog)
    return count


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_root', type=Path)
    args = parser.parse_args()
    root = args.game_root.resolve()
    try:
        result = migrate(root, root / 'update-backups/v2.10.24')
    except (ValueError, OSError, KeyError, TypeError) as exc:
        sys.exit(f'Model pose migration failed: {exc}')
    print(f'Pruned obsolete action data: {result} changed files; backups under update-backups/v2.10.24')
