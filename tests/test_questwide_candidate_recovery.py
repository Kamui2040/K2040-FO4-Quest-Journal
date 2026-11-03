"""Static contract for game-thread-only candidate recovery paths."""
from pathlib import Path
import unittest
R=Path(__file__).resolve().parents[1]
class CandidateRecovery(unittest.TestCase):
 def test_separate_originating_write_view_and_readback_session(self):
  s=(R/'src/main.cpp').read_text()
  self.assertIn('const auto originatingView=uiEpoch.Capture()',s)
  self.assertIn('uiEpoch.IsCurrent(originatingView)',s)
  self.assertIn('candidateSessionEpoch.Capture()',s)
  self.assertIn('candidateSessionEpoch.Invalidate()',s)
  self.assertIn('status.pending ? status.pending->intent.formId : 0u',s)
  self.assertIn('journalOpen && journalFocused.load(std::memory_order_acquire) &&',s)
  self.assertIn('status.pending->intent.desiredTracked?1u:0u',s)
  self.assertIn('char pending[48]',s)
  self.assertIn('respondNoWrite',s)
  # Close/Open must not mutate game or reset the process gate; save must.
  close=s[s.index('    void CloseJournal() {'):s.index('    void HandleJournalEscape(')]
  open_view=s[s.index('    void OpenJournal() {'):s.index('    void ToggleTracker()')]
  destroy=s[s.index('    void DestroyViews() {'):s.index('    void CreateJournalView() {')]
  self.assertNotIn('ResetCandidateGate()',close)
  self.assertNotIn('ResetCandidateGate()',open_view)
  self.assertIn('candidateSelectedFormID.store(0',close)
  self.assertIn('candidateSelectedFormID.store(0',open_view)
  self.assertIn('ResetCandidateGate()',destroy)
  self.assertEqual(s.count('candidateSessionEpoch.Invalidate()'),2)
 def test_no_write_cancel_and_authoritative_status(self):
  controller=(R/'research/native_one_shot/RepeatableQuestWideController.h').read_text()
  executor=(R/'research/production_candidate/QuestWideProductionCandidate.cpp').read_text()
  ui=(R/'view/app.js').read_text()
  self.assertIn('CancelQueuedBeforeWrite',controller)
  self.assertIn('if(controller->CancelQueuedBeforeWrite(*ticket))report(id,Outcome::kRefused)',executor)
  self.assertIn('if(controller->RefuseBeforeWrite(*ticket))report(id,Outcome::kRefused)',executor)
  self.assertIn("result==='refused'",ui)
  self.assertIn("state.candidatePending=null;state.candidateStatus='ready'",ui)
  self.assertIn('receiveQuestWideCandidateResult',ui)
  self.assertIn('enableQuestWideCandidate',ui)
  self.assertIn('kVerifiedBackend = WriteBackend::kNone',(R/'src/QuestTrackingPolicy.h').read_text())
if __name__=='__main__':unittest.main()
