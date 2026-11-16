"""Pose-list migration never reintroduces obsolete actions or erases retained edits."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'scripts/prune-unused-poses.py'

class PoseMigrationTests(unittest.TestCase):
    def test_existing_offsets_survive_and_repeated_migration_is_noop(self):
        spec = importlib.util.spec_from_file_location('prune_unused_poses', SCRIPT)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as tmp:
            checkout = Path(tmp) / 'source'
            for folder in ('models', 'model-studio/starter-project/models', 'assets/models'):
                shutil.copytree(ROOT / folder, checkout / folder)
            player = checkout / 'models/characters/2.vlcharacter.json'
            doc = json.loads(player.read_text())
            doc['skeleton']['sequences']['jump'][3] = [42.0, -21.0]
            doc['skeleton']['sequenceDepth']['jump'][3] = 12.0
            doc['skeleton']['sequences']['wall_jump'] = [[14., 2.]]*14
            doc['skeleton']['sequenceDepth']['wall_jump'] = [1.]*14
            player.write_text(json.dumps(doc))
            backup = checkout / 'update-backups/v2.10.24'
            self.assertGreaterEqual(module.migrate(checkout, backup), 1)
            saved = json.loads(player.read_text())
            self.assertNotIn('wall_jump', saved['skeleton']['sequences'])
            self.assertEqual(saved['skeleton']['sequences']['jump'][3], [42.,-21.])
            self.assertEqual(saved['skeleton']['sequenceDepth']['jump'][3], 12.)
            self.assertIn('wall_jump', json.loads((backup / player.relative_to(checkout)).read_text())['skeleton']['sequences'])
            self.assertEqual(module.migrate(checkout, backup), 0)
            for group in ('characters','enemies'):
                actions=json.loads((checkout/f'assets/models/{group}.actions.json').read_text())
                self.assertEqual(len(actions['definitions']), 16)
                self.assertNotIn('wall_jump',actions['models'][0]['sequences'])

if __name__ == '__main__':
    unittest.main()
