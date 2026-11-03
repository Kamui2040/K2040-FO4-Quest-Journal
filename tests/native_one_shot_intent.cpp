#include "../research/native_one_shot/OneShotIntent.h"
#include <cassert>
#include <cstdint>
using namespace K2040::OneShotResearch;

int main() {
    Gate gate;
    gate.BeginSession(7, 12);
    assert(!gate.Offer(0, true, 7, 12));
    assert(!gate.Offer(42, true, 7, 11));
    auto one=gate.Offer(42, true, 7, 12);
    assert(one && gate.Current(*one));
    assert(!gate.Offer(43, false, 7, 12));
    auto bad=*one;
    bad.formId++;
    assert(!gate.Current(bad));
    assert(!gate.Settle(bad));
    Observation valid{42,42,7,12,
        true,true,true,true,true,true,true,false,kEnabled,7};
    assert(ReviewOnGameTask(*one,valid)==Result::kWouldWrite);
    valid.questFlags|=kTracked;
    valid.feedTracked=true;
    assert(ReviewOnGameTask(*one,valid)==Result::kAlreadyCurrent);
    valid.feedTracked=false;
    assert(ReviewOnGameTask(*one,valid)==Result::kUnverified);
    valid.feedTracked=true;
    assert(ConfirmAfterRefresh(*one,valid)==Verification::kConfirmed);
    valid.questFlags=kEnabled;
    assert(ConfirmAfterRefresh(*one,valid)==Verification::kUnverified);
    valid.feedTracked=false;
    assert(ConfirmAfterRefresh(*one,valid)==Verification::kMismatch);
    valid.questFlags=kEnabled;
    valid.feedHasTrackedBit=false;
    assert(ReviewOnGameTask(*one,valid)==Result::kUnverified);

    valid.feedHasTrackedBit=true;
    valid=Observation{42,42,7,12,
        true,true,true,true,true,true,true,false,kEnabled,7};
    auto rejected=valid;
    rejected.selectedFormId++;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.saveEpoch++;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.uiEpoch++;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.journalOpenFocused=false;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.onF4seTask=false;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.previouslyObserved=false;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.inNormalPipboyFeed=false;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.questType=kMisc;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.loaded=false;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);
    rejected=valid;rejected.og163=false;
    assert(ReviewOnGameTask(*one,rejected)==Result::kRejected);

    for (std::uint32_t bits=0;bits<65536u;++bits) {
        valid.questFlags=std::uint16_t(bits);
        valid.feedTracked=(bits & kTracked)!=0;
        const auto result=ReviewOnGameTask(*one,valid);
        if (!(bits & kEnabled) || (bits & (kCompleted|kFailed)))
            assert(result==Result::kRejected);
        else if (bits & kTracked)
            assert(result==Result::kAlreadyCurrent);
        else
            assert(result==Result::kWouldWrite);
    }

    gate.BeginSession(8,13);
    assert(!gate.Current(*one));
    assert(!gate.Settle(*one));
    auto two=gate.Offer(43,false,8,13);
    assert(two && two->ticket!=one->ticket);
    assert(!gate.Settle(*one));
    assert(gate.Current(*two) && gate.Settle(*two));
    assert(!gate.Current(*two));
}
