"""Regression checks for v0.0.15 model and polygon authoring."""
import copy
import json
import unittest
from pathlib import Path
from editor_logic import (ACTION_STATES, movement_sequences, ensure_sequences,
                          create_model_from_template, assign_model_to_slot,
                          polygon_vertices, edit_structure_vertex, add_structure_vertex,
                          remove_structure_vertex, move_level_object, delete_level_object)
from model_studio import prepare_motion, sample_joint, active_layer

ROOT=Path(__file__).resolve().parents[1]

class ActionTests(unittest.TestCase):
    def setUp(self):
        self.char=json.loads((ROOT/'starter-project/models/characters/2.vlcharacter.json').read_text())
        self.enemy=json.loads((ROOT/'starter-project/models/enemies/1.vlenemy.json').read_text())

    def test_all_supported_movement_states_exposed(self):
        names=movement_sequences('character',self.char)
        self.assertTrue(set(('base','run','sprint','crouch','jump','fall','land',
                             'aim','fire','reload','hit','death','interact')).issubset(names))
        self.assertIn('patrol',movement_sequences('enemy',self.enemy))
        self.assertNotIn('sprint',movement_sequences('enemy',{'id':0}))
        self.assertEqual(len(ACTION_STATES),18)

    def test_action_vectors_round_trip_without_changing_base(self):
        doc=prepare_motion(self.char)
        reference=copy.deepcopy(doc['skeleton']['base'])
        layer,depth=active_layer(doc,'crouch')
        layer[3]=[70.25,-22.5];depth[3]=5.0
        encoded=json.loads(json.dumps(doc))
        x,y,z=sample_joint(encoded,3,'crouch',0)
        self.assertAlmostEqual(x,4*encoded['geometry']['bodyWidth']+70.25)
        self.assertEqual(encoded['skeleton']['base'],reference)
        self.assertAlmostEqual(z,5.)
        self.assertEqual(encoded['skeleton']['sequences']['crouch'][3],layer[3])

    def test_new_character_enemy_prototypes_and_assignment(self):
        for kind,source in [('character',self.char),('enemy',self.enemy)]:
            made=create_model_from_template(source,kind,'Test prototype',103)
            self.assertEqual(made['id'],103)
            made['skeleton']['sequences']['hit'][0][0]=18.5
            self.assertEqual(source['skeleton'].get('sequences',{}).get('hit'),None)
            slotted=assign_model_to_slot(made,source['id'])
            self.assertEqual(slotted['id'],source['id'])
            self.assertEqual(slotted['skeleton']['sequences']['hit'][0][0],18.5)
            self.assertNotIn('templateOf',slotted)

class StructureVertexTests(unittest.TestCase):
    def setUp(self):
        self.doc=json.loads((ROOT/'starter-project/levels/coop-02.vllevel.json').read_text())

    def test_vertices_add_drag_remove_and_structure_move(self):
        selection=('cover',0)
        original=polygon_vertices(self.doc,selection)
        self.assertEqual(len(original),4)
        added=add_structure_vertex(self.doc,selection,0)
        self.assertEqual(len(added),5)
        edit_structure_vertex(self.doc,selection,1,(211.5,310.25))
        saved=json.loads(json.dumps(self.doc))
        self.assertEqual(saved['structureVertices']['cover:0'][1],[211.5,310.25])
        move_level_object(saved,selection,20,15)
        self.assertEqual(saved['structureVertices']['cover:0'][1],[231.5,325.25])
        self.assertEqual(len(remove_structure_vertex(saved,selection,1)),4)
        with self.assertRaises(ValueError):
            for _ in range(2):remove_structure_vertex(saved,selection,0)

    def test_deleted_structure_reindexes_polygons(self):
        self.doc['cover'].append(copy.deepcopy(self.doc['cover'][0]))
        add_structure_vertex(self.doc,('cover',1),1)
        self.assertTrue(delete_level_object(self.doc,('cover',0)))
        self.assertIn('cover:0',self.doc['structureVertices'])
        self.assertNotIn('cover:1',self.doc['structureVertices'])

if __name__=='__main__':unittest.main()
