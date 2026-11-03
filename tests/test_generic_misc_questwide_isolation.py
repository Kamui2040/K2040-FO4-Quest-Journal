"""Ensure one integrated native Misc QA is separately gated from shipping."""
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[1]

class GenericMiscIsolation(unittest.TestCase):
    def test_default_off_and_build_isolation(self):
        x=(ROOT/'xmake.lua').read_text()
        self.assertRegex(x,r'option\("quest_generic_misc_questwide_qa"\)\s+set_default\(false\)')
        self.assertIn('not has_config("quest_native_write_qa")',x)
        self.assertIn('add_files("research/native_one_shot/GenericMiscEvidence.cpp")',x)
        self.assertIn('K2040_GENERIC_MISC_QUESTWIDE_QA=1',x)
        self.assertIn('kVerifiedBackend = WriteBackend::kNone',(ROOT/'src/QuestTrackingPolicy.h').read_text())
        s=(ROOT/'scripts/build-native-write-once-qa.sh').read_text()
        self.assertIn("K2040_GENERIC_MISC_QA:-NO",s)
    def test_runtime_consent_and_instance_do_not_auto_arm(self):
        src=(ROOT/'research/native_one_shot/OneShotOgWriteQA.cpp').read_text()
        for gate in ('ReadExplicitWriteTestConsent()','processAttempted.exchange(true',
                     'expectedInstance','ReviewGenericMisc(','CaptureGenericMiscEvidence(',
                     'quest->currentInstanceID==*expectedInstance',
                     'tasks->AddTask(', 'native(quest,request.desiredTracked)'):
            self.assertIn(gate,src)
        ui=(ROOT/'view/app.js').read_text()
        self.assertIn('qaWriteArmedId!==id',ui)
        self.assertIn("nativeInstanceStatus==='consistent'",ui)
        self.assertIn("pipboyFeedAgreement==='match'",ui)
        self.assertIn('nativeInstanceID.toString(16)',ui)
        main=(ROOT/'src/main.cpp').read_text()
        self.assertIn('enableGenericMiscQuestWideQA',main)
        self.assertIn('#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA',main)
    def test_no_direct_pipboy_or_engine_mutation_in_reader(self):
        src=(ROOT/'research/native_one_shot/GenericMiscEvidence.cpp').read_text()
        self.assertIn('RE::BSAutoLock guard{player->questTargetLock}',src)
        self.assertIn('kMaxObservations',src)
        for forbidden in ('SetActive(', 'ProcessEvent(', 'PromoteReference(',
                          'DemoteReference(', 'SetMember(', 'SetStage(',
                          'SetObjectiveDisplayed('):
            self.assertNotIn(forbidden,src)
        template=(ROOT/'research/native_one_shot/K2040_Quest_Journal_Write_QA.ini').read_text()
        self.assertIn('PermitOneNativeSetActiveCall=NO',template)
if __name__=='__main__':
    unittest.main()
