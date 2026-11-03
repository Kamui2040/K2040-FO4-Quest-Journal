"""Safety gates for the isolated repeatable Misc controller."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class GateTests(unittest.TestCase):
 def test_default_off(self):
  x=(ROOT/'xmake.lua').read_text()
  self.assertIn('option("quest_repeatable_misc_qa")',x)
  self.assertIn('add_files("research/native_one_shot/RepeatableOgWriteQA.cpp")',x)
  self.assertIn('K2040_REPEATABLE_MISC_QA=1',x)
  self.assertIn('kVerifiedBackend = WriteBackend::kNone',(ROOT/'src/QuestTrackingPolicy.h').read_text())
  production=(ROOT/'src/QuestWideTrackingReadiness.h').read_text()
  self.assertIn('kCandidateReadOnly',production)
  self.assertNotIn('SetActive(',production)
  self.assertNotIn('F4SE::',production)
  ini=(ROOT/'research/native_one_shot/K2040_Quest_Journal_Write_QA.ini').read_text()
  for name in ('IHaveBackedUpTestSave','PermitOneNativeSetActiveCall','PermitRepeatableQuestTracking'):
   self.assertIn(name+'=NO',ini)
 def test_no_unverified_native_path(self):
  impl=(ROOT/'research/native_one_shot/RepeatableOgWriteQA.cpp').read_text()
  for token in ('tasks->AddTask(','controller->Offer(','controller->Claim(',
                'MarkWriteReturned','ConfirmRefreshed','ReadRepeatableConsent()'):
   self.assertIn(token,impl)
  for token in ('AddTaskPermanent(', 'ProcessEvent(', 'SetObjectiveDisplayed('):
   self.assertNotIn(token,impl)
  main=(ROOT/'src/main.cpp').read_text()
  self.assertIn('requestRepeatableMiscQuestTracking',main)
  self.assertIn('QueueUIString(QueueRepeatableMiscRequest,request)',main)
  self.assertIn('Phase::kAwaitingRefresh',main)
  self.assertIn('Outcome::kConfirmed',main)
  self.assertIn('Outcome::kBlocked',impl)
  self.assertIn('controller->BlockUncertain(ticket);',impl)
  self.assertIn('GrantSessionConsent()',impl)
  self.assertIn('ResetSessionConsent()',impl)
  self.assertIn('kMaxReadbackAttempts=3',(ROOT/'research/native_one_shot/RepeatableQuestWideController.h').read_text())
  self.assertIn('QueueUIAction(GrantRepeatableQaConsent)',main)
  self.assertIn('grantRepeatableMiscTrackingQA',main)
  self.assertIn('Outcome::kBlocked',main)
  self.assertIn('repeatableNow-lastRepeatableReadbackMillis>=1500u',main)
  self.assertIn('if(phase==Phase::kIdle)report(id,Outcome::kRefused)',impl)
  self.assertNotIn('if(result==K2040::RepeatableQuestWide::Outcome::kPending)\n                    SyncSnapshots();',main)
  ui=(ROOT/'view/app.js').read_text()
  self.assertIn('I have backed up my TEST SAVE',ui)
  self.assertIn('grantRepeatableMiscTrackingQA',ui)
  self.assertIn("repeatableStatus:'off'",ui)
  self.assertIn("nativeInstanceStatus==='consistent'",ui)
  self.assertIn("state.repeatableStatus='blocked'",ui)
if __name__=='__main__':unittest.main()
