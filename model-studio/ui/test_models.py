import copy
import unittest
from model_studio import prepare_motion, sample_joint, NAMES
from editor_logic import edit_scalar_property

class ModelTests(unittest.TestCase):
    def test_all_scalar_properties_can_be_edited(self):
        d={"id":2,"schema":"voidline-character-v4","name":"Kade Vale","role":"Engineer", "health":110,
           "speed":365.0, "motion":{"walkCycle":1.0}}
        self.assertTrue(edit_scalar_property(d,None,"name","Kade Updated"))
        self.assertTrue(edit_scalar_property(d,None,"role","Pilot"))
        self.assertTrue(edit_scalar_property(d,None,"health","140"))
        self.assertTrue(edit_scalar_property(d,"motion","walkCycle","1.2"))
        self.assertEqual(d["name"],"Kade Updated")
        self.assertEqual(d["health"],140)
        self.assertAlmostEqual(d["motion"]["walkCycle"],1.2)
        with self.assertRaises(ValueError):edit_scalar_property(d,None,"id","3")

    def test_legacy_enemy_upgrades_without_losing_stats(self):
        doc={"schema":"voidline-enemy-v1","id":7,"health":250,"width":49}
        prepare_motion(doc)
        self.assertEqual(doc["schema"],"voidline-enemy-v2")
        self.assertEqual(doc["health"],250)
        self.assertEqual(len(doc["skeleton"]["run"]),14)
        self.assertEqual(len(doc["skeleton"]["depth"]["aim"]),14)

    def test_enemy_pose_and_animation(self):
        doc=prepare_motion({"schema":"voidline-enemy-v2","id":2})
        before=sample_joint(doc,8,"aim",0)
        doc["skeleton"]["aim"][8]=[4.0,-3.0]
        doc["skeleton"]["depth"]["aim"][8]=6.0
        after=sample_joint(doc,8,"aim",0)
        self.assertAlmostEqual(after[0]-before[0],4.0)
        self.assertAlmostEqual(after[1]-before[1],-3.0)
        self.assertAlmostEqual(after[2]-before[2],6.0)
        p0=sample_joint(doc,12,"run",0)
        p15=sample_joint(doc,12,"run",15)
        self.assertNotEqual(p0[0],p15[0])

    def test_authored_nonlocomotion_sequences_animate_across_frames(self):
        from pathlib import Path
        import json
        root=Path(__file__).resolve().parents[2]
        doc=json.loads((root/'models/characters/0.vlcharacter.json').read_text())
        for pose,joint in (("jump",12),("fall",12),("land",0),("crouch",0)):
            a=sample_joint(doc,joint,pose,0)
            b=sample_joint(doc,joint,pose,24)
            self.assertNotEqual(a[:2],b[:2],pose)
        # The exact authored frame must be linearly interpolated, not replaced
        # with a hidden editor-only animation.
        rig=doc['skeleton']
        pose='jump';joint=12
        original=rig['poseFrames'][pose][0][joint][:]
        rig['poseFrames'][pose][0][joint][0]+=3.5
        changed=sample_joint(doc,joint,pose,0)
        rig['poseFrames'][pose][0][joint]=original
        baseline=sample_joint(doc,joint,pose,0)
        self.assertAlmostEqual(changed[0]-baseline[0],3.5)

    def test_pose_is_not_mutated_by_preview(self):
        doc=prepare_motion({"schema":"voidline-enemy-v2","id":0})
        original=copy.deepcopy(doc)
        for frame in range(60):
            for index in range(len(NAMES)):
                sample_joint(doc,index,"run",frame)
        self.assertEqual(doc,original)

if __name__=="__main__": unittest.main()
