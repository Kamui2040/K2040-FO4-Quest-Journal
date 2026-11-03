"""Source-only regression gates for nonshipping SetActive QA."""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]

class SetActiveExperimentIsolation(unittest.TestCase):
    def test_consent_defaults_remain_disabled(self):
        template=(ROOT/"research/native_one_shot/K2040_Quest_Journal_Write_QA.ini").read_text()
        self.assertRegex(template,r"(?m)^IHaveBackedUpTestSave=NO$")
        self.assertRegex(template,r"(?m)^PermitOneNativeSetActiveCall=NO$")
        self.assertNotRegex(template,r"(?m)^\w+=YES$")
        writer=(ROOT/"src/QuestTrackingPolicy.h").read_text()
        self.assertIn("kVerifiedBackend = WriteBackend::kNone",writer)
    def test_isolated_build_and_one_shot_runtime_gates(self):
        config=(ROOT/"xmake.lua").read_text()
        self.assertRegex(config,r'option\("quest_native_write_qa"\)\s+set_default\(false\)')
        self.assertIn('add_files("research/native_one_shot/OneShotOgWriteQA.cpp")',config)
        native=(ROOT/"research/native_one_shot/OneShotOgWriteQA.cpp").read_text()
        for gate in ("ReadExplicitWriteTestConsent()",
                     "processAttempted.exchange(true",
                     "tasks->AddTask(", "ReviewOnGameTask(",
                     "native.address()==base+kSetterRva",
                     "kSetterId = 1567562u",
                     "native(quest,request.desiredTracked)"):
            self.assertIn(gate,native)
        self.assertNotRegex(native,r"\bAddTaskPermanent\s*\(")
        view=(ROOT/"view/app.js").read_text()
        self.assertIn("data-action=\"pipboy-write-arm\"",view)
        self.assertIn("data-action=\"pipboy-write-once\"",view)

if __name__=="__main__":
    unittest.main()
