"""Standalone opt-in candidate must never become the normal writer."""
import unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class CandidateIsolation(unittest.TestCase):
 def test_opt_in_and_safe_staging(self):
  x=(R/'xmake.lua').read_text()
  sh=(R/'scripts/build-questwide-candidate.sh').read_text()
  default=(R/'scripts/build-linux.sh').read_text()
  for token in ('option("quest_questwide_production_candidate")',
                'set_default(false)',
                'K2040_QUESTWIDE_CANDIDATE_STAGE',
                'research/production_candidate/QuestWideProductionCandidate.cpp'):
   self.assertIn(token,x)
  self.assertIn('K2040_ENABLE_QUESTWIDE_CANDIDATE',sh)
  self.assertIn('--quest_questwide_production_candidate=y',sh)
  self.assertIn('--quest_dryrun_qa=n',sh)
  self.assertIn('--quest_native_write_qa=n',sh)
  self.assertIn('--quest_repeatable_misc_qa=n',sh)
  self.assertIn('--quest_questwide_production_candidate=n',default)
  self.assertNotIn('quest_questwide_production_candidate=y',default)
  self.assertFalse((R/'src/QuestWideProductionCandidate.cpp').exists())
  self.assertIn('kVerifiedBackend = WriteBackend::kNone',(R/'src/QuestTrackingPolicy.h').read_text())
 def test_native_candidate_reuses_strict_preflight(self):
  c=(R/'research/production_candidate/QuestWideProductionCandidate.cpp').read_text()
  main=(R/'src/main.cpp').read_text()
  for val in ('ReviewQuestWide(','CaptureGenericMiscEvidence(',
              'ObserveOnTask(','controller->Offer(','controller->Claim(',
              'MarkWriteReturned(','ConfirmRefreshed(','QuestWideCandidate: SetActive'):
   self.assertIn(val,c)
  for blocked in ('GrantSessionConsent','ReadRepeatableConsent','SetObjectiveDisplayed','ProcessEvent(',
                  'K2040_TRACKING_DRYRUN_QA=1'):
   self.assertNotIn(blocked,c)
  for val in ('K2040_QUESTWIDE_PRODUCTION_CANDIDATE',
              'QueueUIString(QueueCandidateQuestWide,raw)',
              'QueueUIString(SetCandidateSelection,id)',
              'candidateSaveEpoch.Invalidate()',
              'ResetCandidateGate()',
              'receiveQuestWideCandidateResult'):
   self.assertIn(val,main)
  self.assertIn('static_assert(!K2040::QuestTracking::CanWritePipboy()',c)
  ordinary=(R/'research/production_candidate/OrdinaryQuestEvidence.cpp').read_text()
  self.assertIn('questTargetLock',ordinary)
  self.assertIn('ordinaryFeedRows==1u',ordinary)
  self.assertIn('CaptureOrdinaryQuestEvidence(request)',c)
  self.assertIn('ReviewQuestWide(',c)
  self.assertIn('research/production_candidate/OrdinaryQuestEvidence.cpp',(R/'xmake.lua').read_text())
 def test_ui_selected_source_and_readback_are_candidate_only(self):
  app=(R/'view/app.js').read_text()
  for val in ('selectedQuestWideCandidate','requestQuestWideCandidateTracking',
              'receiveQuestWideCandidateResult',
              "state.candidateStatus==='ready'",
              "q.trackingReadiness!=='candidate-read-only'",
              'state.candidatePending={id,desired}',
              "state.candidateStatus='blocked'"):
   self.assertIn(val,app)
  self.assertIn('state.candidateAvailable',app)
  self.assertIn('questWideCandidateEligible',app)
  self.assertIn('(q.isMisc && q.miscRowCount<=0)',app)
  self.assertIn('data-action="pipboy-track" disabled',app)
if __name__=='__main__':unittest.main()
