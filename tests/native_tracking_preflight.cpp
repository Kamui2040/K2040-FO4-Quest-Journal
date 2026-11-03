#include "../research/native_writer/TrackingPreflight.h"
#include <cassert>
#include <array>
#include <cstdint>

using namespace K2040::NativeTrackingResearch;

int main() {
    Request request{0x00123456u, true, 37u, 99u};
    Observation good{request.formId,37u,99u,
                     true,true,true,true,true,true,
                     kQuestEnabled,1};
    assert(Evaluate(request,good)==Decision::kReadyForUnwiredReview);
    auto bad=good;
    bad.flags|=kQuestTracked;
    assert(Evaluate(request,bad)==Decision::kAlreadyMatching);
    bad=good;
    bad.flags=0;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad.flags=kQuestCompleted|kQuestEnabled;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad.flags=kQuestFailed|kQuestEnabled;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.questType=kMiscQuestType;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.runtimeIsOG163=false;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.gameThread=false;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.gameLoaded=false;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.journalOpenAndFocused=false;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.formIsQuest=false;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.inCurrentPipboyFeed=false;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.formId=0x00abcdef;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.currentSaveEpoch++;
    assert(Evaluate(request,bad)==Decision::kDenied);
    bad=good;
    bad.currentUiEpoch++;
    assert(Evaluate(request,bad)==Decision::kDenied);
    auto noSession=request;
    noSession.saveEpoch=0;
    assert(Evaluate(noSession,good)==Decision::kDenied);
    auto noView=request;
    noView.uiEpoch=0;
    assert(Evaluate(noView,good)==Decision::kDenied);

    request.desiredTracked=false;
    assert(Evaluate(request,good)==Decision::kAlreadyMatching);
    good.flags|=kQuestTracked;
    assert(Evaluate(request,good)==Decision::kReadyForUnwiredReview);

    Request positive{0x00123456u,true,37u,99u};
    Postcondition observed{positive.formId,positive.saveEpoch,positive.uiEpoch,
                           true,true,true,true};
    assert(Confirm(positive,observed)==Confirmation::kConfirmed);
    observed.pipboyFeedTracked=false;
    assert(Confirm(positive,observed)==Confirmation::kMismatch);
    observed=Postcondition{positive.formId,positive.saveEpoch,positive.uiEpoch,
                           true,true,true,true};
    observed.engineFlagTracked=false;
    assert(Confirm(positive,observed)==Confirmation::kMismatch);
    observed.engineFlagTracked=true;
    observed.pipboyFeedAvailable=false;
    assert(Confirm(positive,observed)==Confirmation::kUnverified);
    observed.pipboyFeedAvailable=true;
    observed.visibleInPipboyFeed=false;
    assert(Confirm(positive,observed)==Confirmation::kUnverified);
    observed.visibleInPipboyFeed=true;
    observed.saveEpoch++;
    assert(Confirm(positive,observed)==Confirmation::kUnverified);
    observed.saveEpoch--;
    observed.uiEpoch++;
    assert(Confirm(positive,observed)==Confirmation::kUnverified);
    observed.uiEpoch--;
    observed.formId++;
    assert(Confirm(positive,observed)==Confirmation::kUnverified);

    // All 65,536 combinations of quest flags must honor the minimum
    // engine eligibility guard and reject completed / failed quests.
    auto exhaustive = good;
    exhaustive.flags = 0;
    request.desiredTracked = true;
    for (std::uint32_t bits=0;bits<65536u;++bits) {
        exhaustive.flags=static_cast<std::uint16_t>(bits);
        const auto result=Evaluate(request,exhaustive);
        if ((bits & kQuestEnabled)==0 ||
            (bits & (kQuestCompleted|kQuestFailed))!=0)
            assert(result==Decision::kDenied);
        else if (bits & kQuestTracked)
            assert(result==Decision::kAlreadyMatching);
        else
            assert(result==Decision::kReadyForUnwiredReview);
    }
}
