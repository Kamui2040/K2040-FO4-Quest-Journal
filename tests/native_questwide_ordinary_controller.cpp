#include "../research/native_one_shot/RepeatableQuestWideController.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
using namespace K2040::RepeatableQuestWide;
using namespace K2040::OneShotResearch;
int main() {
 static_assert(!K2040::QuestTracking::CanWritePipboy());
 Controller c;c.BeginSession(12,34);
 Intent i{1,0xFE001234u,true,12,34};
 Observation o{};o.resolvedFormId=i.formId;o.selectedFormId=i.formId;
 o.saveEpoch=12;o.uiEpoch=34;o.og163=o.onF4seTask=o.loaded=true;
 o.journalOpenFocused=o.previouslyObserved=true;
 o.questType=1;o.questFlags=kEnabled;
 o.inNormalPipboyFeed=o.feedHasTrackedBit=true;
 o.feedTracked=false;
 GenericMiscEvidence e{};e.questInstanceID=0;e.nativeDisplayedCount=2;
 e.nativeComplete=e.onlyCurrentInstance=true;e.ordinaryFeedRows=1;
 e.selectedIsOrdinary=e.pipboyFeedComplete=e.feedAgreesWithQuest=true;
 assert(ReviewQuestWide(i,o,0,e)==Result::kWouldWrite);
 auto ticket=c.Offer(i,o,0,e);assert(ticket);
 assert(c.Claim(*ticket,o,e));
 assert(c.MarkWriteReturned(*ticket));
 assert(!c.Offer(i,o,0,e));
 o.questFlags=kEnabled|kTracked;
 assert(!c.ConfirmRefreshed(*ticket,o,e)); // stale UI feed
 assert(c.Get().phase==Phase::kAwaitingRefresh);
 o.feedTracked=true;
 assert(c.ConfirmRefreshed(*ticket,o,e));
 assert(c.Get().phase==Phase::kIdle);
 i.ticket=2;i.desiredTracked=false;
 assert(ReviewQuestWide(i,o,0,e)==Result::kWouldWrite);
 auto second=c.Offer(i,o,0,e);assert(second);
 assert(c.Claim(*second,o,e));assert(c.MarkWriteReturned(*second));
 o.questFlags=kEnabled;o.feedTracked=false;
 assert(c.ConfirmRefreshed(*second,o,e));
 assert(c.Get().phase==Phase::kIdle);
 i.desiredTracked=true;
 assert(ReviewQuestWide(i,o,0,e)==Result::kWouldWrite);
 o.questType=kMisc;
 assert(!c.Offer(i,o,0,e));
 o.questType=1;e.ordinaryFeedRows=2;
 assert(!c.Offer(i,o,0,e));
 e.ordinaryFeedRows=1;e.selectedIsMisc=true;
 assert(!c.Offer(i,o,0,e));
 e.selectedIsMisc=false;e.miscFeedRows=1;
 assert(!c.Offer(i,o,0,e));
 e.miscFeedRows=0;e.onlyCurrentInstance=false;
 assert(!c.Offer(i,o,0,e));
 e.onlyCurrentInstance=true;e.nativeDisplayedCount=0;
 assert(!c.Offer(i,o,0,e));
 e.nativeDisplayedCount=2;o.feedHasTrackedBit=false;
 assert(!c.Offer(i,o,0,e));
 o.feedHasTrackedBit=true;o.inNormalPipboyFeed=false;
 assert(!c.Offer(i,o,0,e));
 o.inNormalPipboyFeed=true;o.feedTracked=true;
 assert(!c.Offer(i,o,0,e));
 o.feedTracked=false;e.questInstanceID=99;
 assert(!c.Offer(i,o,0,e));
 e.questInstanceID=0;o.questFlags=kEnabled|kCompleted;
 assert(!c.Offer(i,o,0,e));
 o.questFlags=kEnabled;e.pipboyFeedComplete=false;
 assert(!c.Offer(i,o,0,e));
 e.pipboyFeedComplete=true;
 Controller blocked;blocked.BeginSession(1,2);
 i.saveEpoch=1;i.uiEpoch=2;o.saveEpoch=1;o.uiEpoch=2;
 auto t=blocked.Offer(i,o,0,e);assert(t);
 assert(blocked.Claim(*t,o,e));assert(blocked.MarkWriteReturned(*t));
 blocked.BeginSession(3,4);
 assert(blocked.Get().phase==Phase::kBlocked);
 assert(!blocked.Offer(i,o,0,e));
 // A mismatched second prewrite observation never called the setter:
 // cancel the exact queued ticket instead of blocking every quest.
 Controller prewrite;prewrite.BeginSession(1,2);
 auto refused=prewrite.Offer(i,o,0,e);assert(refused);
 Observation inconsistent=o;inconsistent.questFlags|=kCompleted;
 assert(!prewrite.Claim(*refused,inconsistent,e));
 assert(prewrite.CancelQueuedBeforeWrite(*refused));
 assert(prewrite.Get().phase==Phase::kIdle);
 assert(!prewrite.CancelQueuedBeforeWrite(*refused));
 auto retried=prewrite.Offer(i,o,0,e);assert(retried);
 assert(prewrite.Claim(*retried,o,e));
 assert(!prewrite.CancelQueuedBeforeWrite(*retried));
 assert(prewrite.MarkWriteReturned(*retried));
 assert(!prewrite.CancelQueuedBeforeWrite(*retried));
 // A postwrite native scope change cannot confirm an ordinary quest.
 Controller cross;cross.BeginSession(1,2);
 auto switched=cross.Offer(i,o,0,e);assert(switched);
 assert(cross.Claim(*switched,o,e));assert(cross.MarkWriteReturned(*switched));
 o.questType=kMisc;o.questFlags=kEnabled|kTracked;o.feedTracked=true;
 assert(!cross.ConfirmRefreshed(*switched,o,e));
 assert(cross.Get().phase==Phase::kBlocked);
 // Completion during a write must also block rather than rearm.
 Controller completed;completed.BeginSession(1,2);
 o.questType=1;o.questFlags=kEnabled;o.feedTracked=false;
 auto finishing=completed.Offer(i,o,0,e);assert(finishing);
 assert(completed.Claim(*finishing,o,e));assert(completed.MarkWriteReturned(*finishing));
 o.questFlags=kEnabled|kTracked|kCompleted;o.feedTracked=true;
 assert(!completed.ConfirmRefreshed(*finishing,o,e));
 assert(completed.Get().phase==Phase::kBlocked);
}
