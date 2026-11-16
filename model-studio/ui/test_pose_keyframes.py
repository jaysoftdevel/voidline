"""Regression checks for source-first authored action frame synchronization."""
import copy
import importlib.util
import json
import tempfile
from pathlib import Path
import shutil
import unittest

ROOT=Path(__file__).resolve().parents[2]
SCRIPT=ROOT/'scripts/pose-keyframes.py'

class SourceKeyframeTests(unittest.TestCase):
    def setUp(self):
        spec=importlib.util.spec_from_file_location('pose_keyframes',SCRIPT)
        self.module=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)

    def test_migrate_preserves_existing_authoring_and_is_idempotent(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            shutil.copytree(ROOT/'models',root/'models')
            docpath=root/'models/characters/0.vlcharacter.json'
            doc=json.loads(docpath.read_text())
            doc['skeleton']['sequences']['jump'][4]=[7.0,-2.0]
            doc['skeleton']['poseFrames'].pop('jump')
            doc['skeleton']['poseFrames']['land'][1][0]=[5.0,5.0]
            docpath.write_text(json.dumps(doc))
            backup=root/'update-backups/v2.10.27'
            self.assertGreaterEqual(self.module.migrate(root,backup),1)
            migrated=json.loads(docpath.read_text())
            self.assertEqual(migrated['skeleton']['sequences']['jump'][4],[7.,-2.])
            self.assertEqual(migrated['skeleton']['poseFrames']['land'][1][0],[5.,5.])
            self.assertEqual(len(migrated['skeleton']['poseFrames']['jump']),5)
            self.assertTrue((backup/'models/characters/0.vlcharacter.json').exists())
            self.assertEqual(self.module.migrate(root,backup),0)
            self.assertEqual(json.loads(docpath.read_text()),migrated)

    def test_sync_preserves_authored_keyframes_not_generated_defaults(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            shutil.copytree(ROOT/'models',root/'models')
            p=root/'models/characters/0.vlcharacter.json'
            doc=json.loads(p.read_text())
            doc['skeleton']['poseFrames']['jump'][2][12]=[31.,-13.]
            p.write_text(json.dumps(doc))
            self.assertEqual(self.module.sync(root),2)
            catalog=json.loads((root/'assets/models/characters.actions.json').read_text())
            self.assertEqual(catalog['models'][0]['poseFrames']['jump'][2][12],[31.,-13.])
            self.assertEqual(self.module.sync(root),0)

if __name__=='__main__':unittest.main()
