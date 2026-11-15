"""No GUI display required: verify editor refuses non-workspace file paths."""
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from model_studio import Studio

class SourceWorkspacePaths(unittest.TestCase):
    def test_only_live_workspace_files(self):
        with tempfile.TemporaryDirectory() as temp:
            checkout=Path(temp)/"checkout"
            (checkout/"models").mkdir(parents=True)
            (checkout/"levels").mkdir()
            (checkout/"assets").mkdir()
            cache=Path(temp)/"cache";cache.mkdir()
            session=SimpleNamespace(project=checkout)
            self.assertTrue(Studio.source_file(session,checkout/"models"/"foo.vlcharacter.json"))
            self.assertTrue(Studio.source_file(session,checkout/"levels"/"sector-01.vllevel.json"))
            self.assertFalse(Studio.source_file(session,cache/"foo.vlcharacter.json"))
            self.assertFalse(Studio.source_file(session,checkout/"../cache"/"foo.vlcharacter.json"))

    def test_file_signature_sees_external_edits(self):
        with tempfile.TemporaryDirectory() as temp:
            source=Path(temp)/"model.json"
            self.assertIsNone(Studio.file_signature(source))
            source.write_text('{"version": 1}')
            previous=Studio.file_signature(source)
            source.write_text('{"version": 2, "new": true}')
            self.assertNotEqual(previous,Studio.file_signature(source))

if __name__=="__main__":unittest.main()
