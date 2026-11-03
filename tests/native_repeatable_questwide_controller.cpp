#include "../research/native_one_shot/RepeatableQuestWideController.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
using namespace K2040::RepeatableQuestWide;
using namespace K2040::OneShotResearch;
int repeatable_failure_tests();
int main(){
 static_assert(!K2040::QuestTracking::CanWritePipboy());
 Controller c;c.BeginSession(7,11);
 Intent request{1,0xFE003211u,true,7,11};
 Observation o{};o.resolvedFormId=request.formId;o.selectedFormId=request.formId;
 o.saveEpoch=7;o.uiEpoch=11;o.og163=true;o.onF4seTask=true;
 o.loaded=true;o.journalOpenFocused=true;o.previouslyObserved=true;
 o.questType=kMisc;o.questFlags=kEnabled;
 GenericMiscEvidence e{};e.questInstanceID=0;e.nativeDisplayedCount=2;
 e.miscFeedRows=2;e.nativeComplete=true;e.onlyCurrentInstance=true;
 e.pipboyFeedComplete=true;e.feedAgreesWithQuest=true;e.selectedIsMisc=true;
 auto first=c.Offer(request,o,0,e);assert(first);
 assert(c.Get().phase==Phase::kQueued);
 assert(!c.Offer(request,o,0,e)); // no concurrent operations
 assert(!c.Claim({999,request,0,2,2},o,e));
 assert(c.Claim(*first,o,e));
 assert(c.Get().phase==Phase::kExecuting);
 assert(!c.Offer(request,o,0,e));
 assert(c.MarkWriteReturned(*first));
 assert(!c.Offer(request,o,0,e)); // still requires refresh, not a return
 assert(!c.ConfirmRefreshed(*first,o,e));
 o.questFlags=kEnabled|kTracked;
 assert(c.ConfirmRefreshed(*first,o,e));
 assert(c.Get().phase==Phase::kIdle);
 request.ticket=2;request.desiredTracked=false;
 auto second=c.Offer(request,o,0,e);assert(second);
 assert(!c.Claim(*first,o,e)); // old ticket cannot act on newer request
 assert(c.Claim(*second,o,e));
 assert(c.MarkWriteReturned(*second));
 o.questFlags=kEnabled;
 assert(c.ConfirmRefreshed(*second,o,e));
 assert(c.Get().phase==Phase::kIdle);
 assert(repeatable_failure_tests()==0);
}

// Failure paths must never auto-arm, even on save/reload or panel close.
int repeatable_failure_tests() {
 Controller c;c.BeginSession(4,8);
 Intent request{1,0xFE001337u,true,4,8};
 Observation o{};o.resolvedFormId=request.formId;o.selectedFormId=request.formId;
 o.saveEpoch=4;o.uiEpoch=8;o.og163=o.onF4seTask=o.loaded=true;
 o.journalOpenFocused=o.previouslyObserved=true;
 o.questType=kMisc;o.questFlags=kEnabled;
 GenericMiscEvidence e{};e.questInstanceID=9;e.nativeDisplayedCount=2;
 e.miscFeedRows=1;e.nativeComplete=e.onlyCurrentInstance=true;
 e.pipboyFeedComplete=e.feedAgreesWithQuest=e.selectedIsMisc=true;
 assert(!c.Offer(request,o,8,e)); // wrong instance
 e.feedAgreesWithQuest=false;
 assert(!c.Offer(request,o,9,e)); // contradictory feed
 e.feedAgreesWithQuest=true;
 auto stale=c.Offer(request,o,9,e);assert(stale);
 c.BeginSession(4,9); // queued task must be cancelled
 assert(!c.Claim(*stale,o,e));
 o.uiEpoch=9;request.uiEpoch=9;
 auto queued=c.Offer(request,o,9,e);assert(queued);
 assert(c.Claim(*queued,o,e));
 c.BeginSession(5,9); // ambiguous if an engine call may be in flight
 assert(c.Get().phase==Phase::kBlocked);
 assert(!c.Offer(request,o,9,e));
 Controller uncertain;uncertain.BeginSession(3,2);
 request.ticket=2;request.saveEpoch=3;request.uiEpoch=2;
 o.saveEpoch=3;o.uiEpoch=2;
 auto t=uncertain.Offer(request,o,9,e);assert(t);
 assert(uncertain.Claim(*t,o,e));
 assert(uncertain.MarkWriteReturned(*t));
 e.questInstanceID=10;
 o.questFlags|=kTracked;
 assert(!uncertain.ConfirmRefreshed(*t,o,e)); // wrong radiant instance
 assert(!uncertain.Offer(request,o,9,e)); // still awaits readback
 uncertain.BlockUncertain(*t);
 assert(uncertain.Get().phase==Phase::kBlocked);
 uncertain.BeginSession(99,100); // never silently unlock a failed write
 assert(uncertain.Get().phase==Phase::kBlocked);
 Controller aborted;aborted.BeginSession(7,8);
 request.ticket=3;request.saveEpoch=7;request.uiEpoch=8;
 o.saveEpoch=7;o.uiEpoch=8;o.questFlags=kEnabled;
 e.questInstanceID=9;
 auto t2=aborted.Offer(request,o,9,e);assert(t2);
 assert(aborted.Claim(*t2,o,e));
 assert(aborted.RefuseBeforeWrite(*t2));
 assert(aborted.Get().phase==Phase::kIdle);
 assert(aborted.Offer(request,o,9,e)); // only proven no-call can retry
 // A delayed native/Pip-Boy event is allowed two further observations,
 // but NEVER another write while awaiting authoritative confirmation.
 Controller delayed;delayed.BeginSession(7,8);
 auto later=delayed.Offer(request,o,9,e);assert(later);
 assert(delayed.Claim(*later,o,e));assert(delayed.MarkWriteReturned(*later));
 GenericMiscEvidence lag=e;lag.feedAgreesWithQuest=false;
 assert(!delayed.ConfirmRefreshed(*later,o,lag));
 assert(delayed.Get().phase==Phase::kAwaitingRefresh);
 assert(!delayed.Offer(request,o,9,e));
 assert(!delayed.ConfirmRefreshed(*later,o,lag));
 assert(delayed.Get().phase==Phase::kAwaitingRefresh);
 assert(!delayed.ConfirmRefreshed(*later,o,lag));
 assert(delayed.Get().phase==Phase::kBlocked);
 delayed.BeginSession(11,12);
 assert(delayed.Get().phase==Phase::kBlocked);
 // Transient lag can recover without a second SetActive call.
 Controller recovered;recovered.BeginSession(7,8);
 auto fresh=recovered.Offer(request,o,9,e);assert(fresh);
 assert(recovered.Claim(*fresh,o,e));assert(recovered.MarkWriteReturned(*fresh));
 assert(!recovered.ConfirmRefreshed(*fresh,o,lag));
 o.questFlags|=kTracked;
 assert(recovered.ConfirmRefreshed(*fresh,o,e));
 assert(recovered.Get().phase==Phase::kIdle);
 return 0;
}
