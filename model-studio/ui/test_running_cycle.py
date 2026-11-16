"""Human locomotion cycle and source-only editor startup regression checks."""
import math
import tempfile
import unittest
from pathlib import Path

from editor_logic import discover_game_workspace, running_foot, running_hip_offset
from model_studio import sample_joint
import json

ROOT = Path(__file__).resolve().parents[2]

class HumanRunningTests(unittest.TestCase):
    def test_alternating_12_frame_cycle(self):
        samples = [running_foot(i/12) for i in range(12)]
        self.assertTrue(samples[0][0])
        self.assertFalse(samples[5][0])
        self.assertTrue(running_foot(.5)[0] is False)
        # Two staggered contacts with short double-flight intervals.
        for phase in (.40, .90):
            self.assertFalse(running_foot(phase)[0])
            self.assertFalse(running_foot(phase+.5)[0])
        self.assertGreater(running_foot(.69)[2], running_foot(.40)[2])
        self.assertAlmostEqual(running_foot(1.0)[2], 0)
        self.assertAlmostEqual(running_foot(.38)[2], 0)
        self.assertAlmostEqual(running_foot(0)[3], .28)
        self.assertAlmostEqual(running_foot(0)[0], running_foot(1.0)[0])

    def test_hip_bob_is_periodic(self):
        for p in (.04,.31,.51,.83):
            self.assertAlmostEqual(running_hip_offset(p), running_hip_offset(p+.5), places=4)

    def test_preview_legs_alternate(self):
        model = json.loads((ROOT/'models/characters/0.vlcharacter.json').read_text())
        # Opposite-foot phases cannot animate as mirror copies at the same time.
        points = [sample_joint(model,i,'sprint',7) for i in (12,13)]
        self.assertNotAlmostEqual(points[0][0],points[1][0])
        self.assertNotAlmostEqual(points[0][1],points[1][1])

class AutoWorkspaceTests(unittest.TestCase):
    def test_detect_checkout_not_editor_cache(self):
        self.assertEqual(discover_game_workspace(ROOT), ROOT)
        with tempfile.TemporaryDirectory() as tmp:
            stale=Path(tmp)
            (stale/'models').mkdir()
            (stale/'models/characters').mkdir()
            # A partial editor cache is never treated as a game checkout.
            self.assertEqual(discover_game_workspace(stale,ROOT),ROOT)
            self.assertIsNone(discover_game_workspace(stale))

if __name__ == '__main__':
    unittest.main()
