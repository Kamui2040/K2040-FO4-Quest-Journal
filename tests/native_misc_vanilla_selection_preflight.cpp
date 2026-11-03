#include "../research/native_one_shot/MiscVanillaSelectionPreflight.h"
#include <cassert>

using namespace K2040::MiscVanillaResearch;
static_assert(!kNativeMiscWritesAuthorized);

int main() {
    Snapshot s{
        .correctOgRuntime=true,
        .supportedF4seTask=true,
        .loadedAndFocused=true,
        .currentSaveAndView=true,
        .playerObservedAndSelected=true,
        .selectedFormId=0x12345678,
        .resolvedFormId=0x12345678,
        .questType=kMiscQuestType,
        .questFlags=kQuestRunning | kQuestTracked,
        .matchingRootRows=0,
        .matchingMiscRows=1,
        .displayedObjectiveCandidates=1,
        .truncated=false,
        .nestedActiveTyped=true,
        .nestedActive=true,
        .nestedEnabledTyped=true,
        .nestedEnabled=true,
        .instanceFieldTyped=true
    };
    assert(Review(s,false)==Decision::kCandidateOnly);
    assert(Review(s,true)==Decision::kAlreadyCurrent);

    s.matchingMiscRows=2;
    assert(Review(s,false)==Decision::kUnverified);
    s.matchingMiscRows=1;
    s.displayedObjectiveCandidates=2;
    assert(Review(s,false)==Decision::kUnverified);
    s.displayedObjectiveCandidates=1;
    s.nestedActive=false;
    assert(Review(s,false)==Decision::kUnverified);
    s.nestedActive=true;
    s.nestedEnabledTyped=false;
    assert(Review(s,false)==Decision::kUnverified);
    s.nestedEnabledTyped=true;
    s.instanceFieldTyped=false;
    assert(Review(s,false)==Decision::kUnverified);
    s.instanceFieldTyped=true;
    s.truncated=true;
    assert(Review(s,false)==Decision::kUnverified);
    s.truncated=false;
    s.matchingRootRows=1;
    assert(Review(s,false)==Decision::kUnverified);
    s.matchingRootRows=0;
    s.questFlags=kQuestRunning | kQuestCompleted | kQuestTracked;
    assert(Review(s,false)==Decision::kReject);
    s.questFlags=kQuestRunning | kQuestFailed | kQuestTracked;
    assert(Review(s,false)==Decision::kReject);
    s.questFlags=kQuestTracked;
    assert(Review(s,false)==Decision::kReject);
    s.questFlags=kQuestRunning|kQuestTracked;
    s.questType=2;
    assert(Review(s,false)==Decision::kReject);
    s.questType=kMiscQuestType;
    s.currentSaveAndView=false;
    assert(Review(s,false)==Decision::kReject);
    s.currentSaveAndView=true;
    s.resolvedFormId=0x98765432;
    assert(Review(s,false)==Decision::kReject);
}
