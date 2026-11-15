"""Clipboard and dialog sizing tests: no X display needed."""
import copy
import json
import unittest
from editor_logic import (copy_model_payload,paste_model_payload,copy_movement_payload,
                          paste_movement_payload)
from model_studio import prepare_motion
from studio_dialogs import dialog_dimensions

class ClipboardTests(unittest.TestCase):
    def setUp(self):
        self.source=prepare_motion({"schema":"voidline-character-v4","id":1,"name":"Source"})
        self.target=prepare_motion({"schema":"voidline-character-v4","id":4,"name":"Target"})

    def test_model_copy_paste_preserves_destination_slot_and_source(self):
        original=copy.deepcopy(self.source)
        self.source["geometry"]["bodyHeight"]=2.5
        payload=copy_model_payload(self.source,"character")
        out=paste_model_payload(payload,self.target,"character")
        self.assertEqual(out["id"],4)
        self.assertEqual(out["name"],"Source")
        self.assertEqual(out["geometry"]["bodyHeight"],2.5)
        self.assertEqual(self.target["name"],"Target")
        self.assertEqual(original["id"],self.source["id"])

    def test_wrong_type_and_non_model_clipboard_rejected(self):
        payload=copy_model_payload(self.source,"character")
        with self.assertRaisesRegex(ValueError,"different model type"):
            paste_model_payload(payload,self.target,"enemy")
        with self.assertRaises(ValueError):paste_model_payload("hello",self.target,"character")
        with self.assertRaises(ValueError):paste_model_payload("{}",self.target,"character")

    def test_movement_across_named_poses_and_models(self):
        self.source["skeleton"]["sequences"]["wall_jump"][2]=[20.0,-9.5]
        self.source["skeleton"]["sequenceDepth"]["wall_jump"][2]=5.25
        payload=copy_movement_payload(self.source,"wall_jump")
        result=paste_movement_payload(payload,self.target,"dodge")
        self.assertEqual(result["skeleton"]["sequences"]["dodge"][2],[20.,-9.5])
        self.assertEqual(result["skeleton"]["sequenceDepth"]["dodge"][2],5.25)
        self.assertNotIn("dodge",self.target["skeleton"]["sequences"])
        legacy=paste_movement_payload(payload,self.target,"aim")
        self.assertEqual(legacy["skeleton"]["aim"][2],[20.,-9.5])

    def test_bad_movement_data_is_rejected(self):
        data=json.loads(copy_movement_payload(self.source,"run"))
        data["data"]["xy"][0]=[float("nan"),0]
        with self.assertRaisesRegex(ValueError,"invalid"):
            paste_movement_payload(json.dumps(data),self.target,"jump")
        data["data"]["xy"]=[[1,2]]
        with self.assertRaisesRegex(ValueError,"exactly 14"):
            paste_movement_payload(json.dumps(data),self.target,"jump")

    def test_dialog_sizes_readable_and_screen_bounded(self):
        w,h=dialog_dimensions("A concise status message",1920,1080)
        self.assertGreaterEqual(w,420)
        self.assertGreaterEqual(h,170)
        w,h=dialog_dimensions("Long status. "*500,1024,768)
        self.assertLessEqual(w,964)
        self.assertLessEqual(h,668)

if __name__=="__main__":unittest.main()
