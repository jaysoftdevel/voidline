import copy
import json
import math
import unittest
from pathlib import Path
from editor_logic import (can_edit_joint, run_phase, run_cycles_per_second, rig_status,
                          hit_level_object, move_level_object, level_selected_anchor,
                          create_level_object, delete_level_object)
from model_studio import sample_joint, prepare_motion

ROOT=Path(__file__).resolve().parents[1]

class RuntimePreviewTests(unittest.TestCase):
    def test_game_character_sprint_gait_100_percent(self):
        p=ROOT/'starter-project/models/characters/2.vlcharacter.json'
        doc=json.loads(p.read_text())
        # Player speed × 1.28 sprint / (118 stride * stride / walkCycle)
        expected=float(doc['speed'])*1.28/(118*doc['skeleton']['stride']/doc['motion']['walkCycle'])
        self.assertAlmostEqual(run_cycles_per_second(doc,'character'),expected)
        self.assertAlmostEqual(run_phase(doc,'character',30,100),math.tau*expected)
        self.assertAlmostEqual(run_phase(doc,'character',30,50),math.tau*expected*.5)
        self.assertAlmostEqual(run_phase(doc,'character',30,200),math.tau*expected*2)
        a=sample_joint(doc,12,'run',12,'character',100)
        b=sample_joint(doc,12,'run',12,'character',25)
        self.assertNotEqual(a,b)

    def test_enemy_game_distance_cycle(self):
        p=ROOT/'starter-project/models/enemies/6.vlenemy.json'
        doc=json.loads(p.read_text())
        # Pre-v2 enemy files may have no motion object. The preview should
        # use the default cycle, not fail with KeyError.
        cycle=float(doc.get('motion',{}).get('walkCycle',1.0))
        self.assertAlmostEqual(run_cycles_per_second(doc,'enemy'),float(doc['speed'])*cycle/56)
        from model_studio import prepare_motion
        prepared=prepare_motion(doc)
        self.assertAlmostEqual(run_cycles_per_second(prepared,'enemy'),float(doc['speed'])*cycle/56)

    def test_supported_rig_only(self):
        for i in range(10):
            d={'id':i}
            self.assertEqual(can_edit_joint('enemy',d),i in (1,6,7,9))
            if i not in (1,6,7,9):
                self.assertIn('disabled',rig_status('enemy',d))
        self.assertTrue(can_edit_joint('character',{'id':0}))
        self.assertFalse(can_edit_joint('level',{}))

class LevelEditingTests(unittest.TestCase):
    def setUp(self):
        p=ROOT/'starter-project/levels/coop-02.vllevel.json'
        self.doc=json.loads(p.read_text())

    def test_select_and_move_stair_both_endpoints(self):
        a=copy.deepcopy(self.doc['stairs'][0]);
        s=hit_level_object(self.doc,a[0]+10,(a[1]+(a[3]-a[1])*10/(a[2]-a[0])),1.0)
        self.assertEqual(s,('stairs',0))
        move_level_object(self.doc,s,40,-15)
        self.assertEqual(self.doc['stairs'][0],[a[0]+40,a[1]-15,a[2]+40,a[3]-15])

    def test_select_interior_and_drag_ground(self):
        r=self.doc['ground'][0]
        sel=hit_level_object(self.doc,r[0]+min(200,r[2]/2),r[1]+r[3]*.85,1)
        self.assertEqual(sel,('ground',0))
        move_level_object(self.doc,sel,-7,8)
        self.assertEqual(self.doc['ground'][0][0],r[0]) # same shared reference updates

    def test_relay_door_and_switch_movable(self):
        r=self.doc['relays'][0]
        original=list(r)
        sx,sy=r[4:6]
        sel=hit_level_object(self.doc,sx,sy,1)
        self.assertEqual(sel,('relays',0,'switch'))
        move_level_object(self.doc,sel,35,16)
        self.assertEqual(self.doc['relays'][0][:4],original[:4])
        self.assertEqual(self.doc['relays'][0][4],original[4]+35)
        sel=('relays',0)
        move_level_object(self.doc,sel,10,5)
        self.assertEqual(self.doc['relays'][0][0],original[0]+10)
        self.assertEqual(self.doc['relays'][0][4],original[4]+45)

    def test_every_level_object_can_move(self):
        for key in ('ground','ledges','cover','stairs','lifts','vents','enemies','beacons',
                    'supplies','crates','relays','insertion','exit'):
            doc=copy.deepcopy(self.doc)
            if key not in doc:continue
            if key in ('insertion','exit') or doc.get(key):
                sel=(key,0)
                before=level_selected_anchor(doc,sel)
                move_level_object(doc,sel,13,-4)
                after=level_selected_anchor(doc,sel)
                self.assertAlmostEqual(after[0]-before[0],13, msg=key)
                self.assertAlmostEqual(after[1]-before[1],-4,msg=key)

    def test_create_move_delete_every_kind(self):
        for key in ('ground','ledges','cover','stairs','lifts','vents','enemies',
                    'beacons','supplies','crates','relays'):
            doc=copy.deepcopy(self.doc)
            sel=create_level_object(doc,key,180,280)
            before=level_selected_anchor(doc,sel)
            move_level_object(doc,sel,27,11)
            after=level_selected_anchor(doc,sel)
            self.assertAlmostEqual(after[0]-before[0],27,msg=key)
            self.assertAlmostEqual(after[1]-before[1],11,msg=key)
            self.assertTrue(delete_level_object(doc,sel))
        self.assertFalse(delete_level_object(self.doc,('insertion',0)))
        self.assertFalse(delete_level_object(self.doc,('exit',0)))

if __name__=='__main__':unittest.main()
