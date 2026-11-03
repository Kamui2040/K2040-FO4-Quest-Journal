"""Fail-closed build isolation for the unwired quest-wide research reader."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]

class QuestWideIsolation(unittest.TestCase):
    def test_compile_only_in_isolated_readonly_qa(self):
        config=(ROOT/"xmake.lua").read_text()
        name="research/native_questwide/NativeQuestWideSnapshot.cpp"
        self.assertEqual(config.count(name),1)
        start=config.index(name)
        scope=config.rfind('if has_config("quest_pipboy_selection_readonly_qa")',0,start)
        schema=config.rfind('if has_config("quest_pipboy_selection_schema_readonly_qa")',scope,start)
        self.assertGreaterEqual(scope,0)
        self.assertGreater(schema,scope)
        self.assertLess(schema,start)
        self.assertNotIn("CaptureNativeQuestWideReadOnly",
                         (ROOT/"src/main.cpp").read_text())
    def test_research_does_not_call_mutation(self):
        source=(ROOT/"research/native_questwide/NativeQuestWideSnapshot.cpp").read_text()
        self.assertIn("#error Native quest-wide snapshot adapter is isolated read-only QA only",source)
        for forbidden in ("SetActive(","SetStage(","PromoteReference(",
                          "DemoteReference(","ProcessEvent(","SetMember(",
                          "Invoke(","WriteProcessMemory(","VirtualProtect("):
            self.assertNotIn(forbidden,source)
        policy=(ROOT/"src/QuestTrackingPolicy.h").read_text()
        self.assertIn("kVerifiedBackend = WriteBackend::kNone",policy)

if __name__=="__main__":
    unittest.main()
