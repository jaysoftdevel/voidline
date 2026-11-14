#!/usr/bin/env python3
"""Stage optional editable level/model JSON without overwriting user modifications."""
from pathlib import Path
import shutil
import sys

if len(sys.argv) != 3:
    sys.exit("usage: copy-editables.py SOURCE_GAME_ROOT TARGET_RUNTIME_ROOT")
source, target = (Path(arg).resolve() for arg in sys.argv[1:])
for category in ("levels", "models"):
    folder = source / category
    if not folder.is_dir():
        continue
    for file in folder.rglob("*.json"):
        relative = file.relative_to(source)
        destination = target / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists():
            shutil.copy2(file, destination)
