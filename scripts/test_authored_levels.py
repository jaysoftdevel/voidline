#!/usr/bin/env python3
"""Check authored terrain connectivity, extraction clearance, and optional high routes."""
import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent / "levels"


class AuthoredLevelTests(unittest.TestCase):
    def test_all_levels(self):
        paths = sorted(ROOT.glob("*.vllevel.json"))
        self.assertEqual(len(paths), 26)
        for path in paths:
            with self.subTest(level=path.name):
                level = json.loads(path.read_text())
                ground = level["ground"]
                ledges = level["ledges"]
                self.assertTrue(ground)
                self.assertGreater(len(ledges), 35)
                # A 39px crouched hitbox must fit, with 8px visual clearance.
                # Intersecting solid geometry is not an accessible corridor.
                for upper in ledges:
                    for lower in ground + ledges:
                        if upper is lower or upper[1] >= lower[1]:
                            continue
                        horizontal = min(upper[0] + upper[2], lower[0] + lower[2]) - max(upper[0], lower[0])
                        gap = lower[1] - upper[1] - upper[3]
                        if horizontal > 36 and gap >= 0:
                            self.assertGreaterEqual(gap, 46.9, "Crouched operative cannot fit beneath platform")
                # Climbable ladders have a supported entry/exit and descend
                # to a lower deck. The final scripted lift moves up and down.
                self.assertGreaterEqual(len(level.get('ladders', [])), 3)
                self.assertGreaterEqual(len(level.get('lifts', [])), 1)
                lift = level['lifts'][-1]
                self.assertGreaterEqual(abs(lift[4]), 35)
                self.assertGreater(lift[5], 0)
                for x, top, bottom in level['ladders']:
                    self.assertGreater(bottom - top, 50)
                    self.assertTrue(any(abs(deck[1]-top)<3 and deck[0]-8 <= x <= deck[0]+deck[2]+8
                                        for deck in ground+ledges), "Ladder top has no deck")
                    self.assertTrue(any(abs(deck[1]-bottom)<3 and deck[0]-8 <= x <= deck[0]+deck[2]+8
                                        for deck in ground+ledges), "Ladder bottom has no deck")
                if path.name.startswith('sector-'):
                    self.assertGreater(max(r[1] for r in ledges), min(r[1] for r in ground) + 30,
                                       "Campaign levels need a lower-baseline service route")
                self.assertGreaterEqual(ground[0][2], 200)
                self.assertAlmostEqual(level["insertion"][1], ground[0][1], delta=10)
                # No vertical cover blocks a first step or extraction interaction.
                for x in (level["insertion"][0], level["exit"][0]):
                    self.assertTrue(all(abs(c[0]+c[2]/2-x)>120 for c in level["cover"]))
                # A fixed portion of every level has climbable elevated routes.
                self.assertLess(min(l[1] for l in ledges), min(g[1] for g in ground)-95)
                if path.name.startswith("sector-"):
                    for a,b in zip(ground,ground[1:]):
                        self.assertLessEqual(abs(a[1]-b[1]), 83)
                        gap = b[0]-(a[0]+a[2])
                        if gap > 110:
                            self.assertTrue(any(a[0]+a[2]-60 <= l[0] <= b[0]+50 and
                                                min(a[1],b[1])-80<=l[1]<=max(a[1],b[1])+25
                                                for l in ledges),
                                            "Missing foothold between wide terrain gaps")
                else:
                    self.assertTrue(level["relays"], "Co-op puzzle gates must be retained")
                # Both endpoints of each slope must meet a solid deck exactly,
                # with enough overlap for the player's leading edge to enter
                # before its feet leave the slope. Prevent future corner traps.
                for stair in level["stairs"]:
                    x0, y0, x1, y1 = stair
                    self.assertGreaterEqual(x1 - x0, 48)
                    self.assertLessEqual(abs(y1 - y0), (x1 - x0) * .95)
                    for x, y in ((x0, y0), (x1, y1)):
                        self.assertTrue(any(
                            abs(s[1] - y) < .5 and
                            s[0] + 27 <= x <= s[0] + s[2] - 27
                            for s in ground + ledges
                        ), f"Stair at ({x}, {y}) lacks a flush, overlapping landing")

if __name__ == "__main__":
    unittest.main()
