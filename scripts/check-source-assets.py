#!/usr/bin/env python3
"""Cross-platform, deterministic asset-pack digest for source parity checks."""
import hashlib
import pathlib
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'scripts'))
import importlib.util
spec = importlib.util.spec_from_file_location('voidline_pack', root / 'scripts' / 'pack-assets.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

assets = root / 'assets'
files = sorted(p for p in assets.rglob('*') if p.is_file()
               and 'licenses' not in p.relative_to(assets).parts and p.suffix != '.vpak')
if not files:
    raise SystemExit('No assets in source folder')
with tempfile.TemporaryDirectory() as scratch:
    pack = pathlib.Path(scratch) / 'game.vpak'
    mod.pack(assets, pack)
    content = pack.read_bytes()
    print('VOIDLINE asset compatibility report')
    print('Source asset files:', len(files))
    print('Packed size:', len(content), 'bytes')
    print('Packed SHA256:', hashlib.sha256(content).hexdigest())
    print('Includes:', ', '.join(sorted(f.name for f in (assets/'models').iterdir())))
