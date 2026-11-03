// Synthetic, engine-independent read-only Misc row correlation tests.
#define K2040_TRACKING_INTEGRATED_QA 1
#include "../research/native_one_shot/MiscObjectiveIdentity.h"
#include <array>
#include <cassert>
#include <string_view>
int main() {
    using namespace K2040::MiscIdentity;
    constexpr std::array<NativeObjective,4> displayed{{
        {50,17},{51,17},{50,21},{0,0}
    }};
    auto unique=Compare({50,17},displayed,true);
    assert(unique.status==Status::kUniqueCandidate && unique.count==1);
    // Runtime-shaped read-only fixture: a Pip-Boy Misc row contains
    // instance=1 but NO objective index; one native index=50 is shown.
    constexpr std::array<NativeObjective,1> singleDisplayed{{{50,1}}};
    const auto runtimeShape=Compare({std::nullopt,1},singleDisplayed,true);
    assert(runtimeShape.status==Status::kUniqueCandidate);
    assert(runtimeShape.count==1);
    // Another displayed objective with the SAME instance immediately
    // makes the very same Pip-Boy row insufficient to identify either.
    constexpr std::array<NativeObjective,2> sharedInstance{{{50,1},{51,1}}};
    const auto multiRowRisk=Compare({std::nullopt,1},sharedInstance,true);
    assert(multiRowRisk.status==Status::kAmbiguous);
    assert(multiRowRisk.count==2);
    // A unique CANDIDATE cannot confer permission to write game state.
    auto ambiguous=Compare({std::nullopt,17},displayed,true);
    assert(ambiguous.status==Status::kAmbiguous && ambiguous.count==2);
    auto missing=Compare({50,std::nullopt},displayed,true);
    assert(missing.status==Status::kMissingInstance && missing.count==0);
    auto wrong=Compare({99,17},displayed,true);
    assert(wrong.status==Status::kNoCandidates && wrong.count==0);
    auto zero=Compare({0,0},displayed,true);
    assert(zero.status==Status::kUniqueCandidate && zero.count==1);
    auto untrusted=Compare({50,17},displayed,false);
    assert(untrusted.status==Status::kIncomplete && untrusted.count==0);
    assert(std::string_view(StatusName(Status::kUniqueCandidate))==
           "single-candidate-not-verified");
}
