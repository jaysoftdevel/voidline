#!/usr/bin/env python3
"""Sync published source level overrides to an executable folder.

Only known sector/coop level filenames are managed; absent source overrides are
removed, and unchanged files retain mtimes for reproducible incremental builds.
"""
from pathlib import Path
import argparse
import os
import re
import shutil
import tempfile

PATTERN = re.compile(r"(?:sector|coop)-[0-9]{2}\.vllevel\.json\Z")

def sync(source: Path, dest: Path) -> tuple[int, int]:
    source = source.resolve()
    dest = dest.resolve()
    expected = {}
    if source.is_dir():
        for file in source.iterdir():
            if file.is_symlink():
                if PATTERN.fullmatch(file.name):
                    raise ValueError(f"Linked source level rejected: {file}")
                continue
            if file.is_file() and PATTERN.fullmatch(file.name):
                expected[file.name] = file
    dest.mkdir(parents=True, exist_ok=True)
    copied = removed = 0
    for file in dest.iterdir():
        if PATTERN.fullmatch(file.name) and file.name not in expected:
            if file.is_symlink() or not file.is_file():
                raise ValueError(f"Invalid staged level: {file}")
            file.unlink()
            removed += 1
    for name, file in expected.items():
        target = dest / name
        if target.is_symlink():
            raise ValueError(f"Linked staged level rejected: {target}")
        if target.is_file() and target.read_bytes() == file.read_bytes():
            continue
        with tempfile.NamedTemporaryFile(dir=dest, delete=False) as tmp:
            staged = Path(tmp.name)
        try:
            shutil.copyfile(file, staged)
            os.replace(staged, target)
        finally:
            staged.unlink(missing_ok=True)
        copied += 1
    return copied, removed

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    copied, removed = sync(args.source, args.destination)
    print(f"Authored levels synchronized: {copied} updated, {removed} removed")
