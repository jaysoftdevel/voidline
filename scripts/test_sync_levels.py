#!/usr/bin/env python3
"""Regression checks for cross-platform published-level staging."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

MODULE = Path(__file__).with_name("sync-levels.py")
spec = importlib.util.spec_from_file_location("voidline_sync_levels", MODULE)
levels = importlib.util.module_from_spec(spec)
spec.loader.exec_module(levels)


class StageLevels(unittest.TestCase):
    def test_changed_only_and_removed_overrides(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            src, dest = root / "source", root / "runtime"
            src.mkdir()
            scene = src / "sector-01.vllevel.json"
            scene.write_text('{"worldWidth": 1000}\n')
            copied, removed = levels.sync(src, dest)
            self.assertEqual((copied, removed), (1, 0))
            self.assertEqual((dest / scene.name).read_bytes(), scene.read_bytes())
            first_mtime = (dest / scene.name).stat().st_mtime_ns
            self.assertEqual(levels.sync(src, dest), (0, 0))
            self.assertEqual(first_mtime, (dest / scene.name).stat().st_mtime_ns)
            scene.write_text('{"worldWidth": 1100}\n')
            self.assertEqual(levels.sync(src, dest), (1, 0))
            (dest / "personal-notes.json").write_text('{}')
            scene.unlink()
            self.assertEqual(levels.sync(src, dest), (0, 1))
            self.assertFalse((dest / scene.name).exists())
            self.assertTrue((dest / "personal-notes.json").exists())

    def test_coop_and_symlink_guard(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            src, dest = root / "source", root / "runtime"
            src.mkdir()
            (src / "coop-06.vllevel.json").write_text('{}')
            self.assertEqual(levels.sync(src, dest), (1, 0))
            (src / "sector-01.vllevel.json").symlink_to("coop-06.vllevel.json")
            with self.assertRaises(ValueError):
                levels.sync(src, dest)


if __name__ == "__main__":
    unittest.main()
