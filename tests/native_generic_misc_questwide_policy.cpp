#include "../research/native_one_shot/GenericMiscQuestWidePolicy.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <optional>
using namespace K2040::OneShotResearch;
int main() {
    static_assert(!K2040::QuestTracking::CanWritePipboy());
    Intent request{3,0xFE001234u,true,18,9};
    Observation o{};
    o.resolvedFormId=request.formId;o.selectedFormId=request.formId;
    o.saveEpoch=request.saveEpoch;o.uiEpoch=request.uiEpoch;
    o.og163=true;o.onF4seTask=true;o.loaded=true;
    o.journalOpenFocused=true;o.previouslyObserved=true;
    o.questType=kMisc;o.questFlags=kEnabled;
    GenericMiscEvidence e{};
    e.questInstanceID=0; // instance ID zero is valid
    e.nativeDisplayedCount=2;e.miscFeedRows=2;
    e.nativeComplete=true;e.onlyCurrentInstance=true;
    e.pipboyFeedComplete=true;e.feedAgreesWithQuest=true;
    e.selectedIsMisc=true;
    auto expect=[&](Result result){assert(ReviewGenericMisc(request,o,0,e)==result);};
    expect(Result::kWouldWrite);
    request.desiredTracked=false;
    expect(Result::kAlreadyCurrent);
    request.desiredTracked=true;
    assert(ReviewGenericMisc(request,o,std::nullopt,e)==Result::kRejected);
    assert(ReviewGenericMisc(request,o,1,e)==Result::kRejected);
    e.onlyCurrentInstance=false;expect(Result::kRejected);e.onlyCurrentInstance=true;
    e.nativeComplete=false;expect(Result::kRejected);e.nativeComplete=true;
    e.feedAgreesWithQuest=false;expect(Result::kRejected);e.feedAgreesWithQuest=true;
    e.pipboyFeedComplete=false;expect(Result::kRejected);e.pipboyFeedComplete=true;
    e.miscFeedRows=0;expect(Result::kRejected);e.miscFeedRows=2;
    e.nativeDisplayedCount=0;expect(Result::kRejected);e.nativeDisplayedCount=2;
    e.nativeDisplayedCount=513;expect(Result::kRejected);e.nativeDisplayedCount=2;
    e.selectedIsMisc=false;expect(Result::kRejected);e.selectedIsMisc=true;
    o.questFlags=kEnabled|kCompleted;expect(Result::kRejected);o.questFlags=kEnabled;
    o.questFlags=kEnabled|kFailed;expect(Result::kRejected);o.questFlags=kEnabled;
    o.questType=1;expect(Result::kRejected);o.questType=kMisc;
    ++o.uiEpoch;expect(Result::kRejected);--o.uiEpoch;
    ++o.saveEpoch;expect(Result::kRejected);--o.saveEpoch;
    o.onF4seTask=false;expect(Result::kRejected);o.onF4seTask=true;
    o.journalOpenFocused=false;expect(Result::kRejected);o.journalOpenFocused=true;
    o.questFlags=kEnabled|kTracked;
    request.desiredTracked=false;
    expect(Result::kWouldWrite);
    request.desiredTracked=true;
    expect(Result::kAlreadyCurrent);
}
