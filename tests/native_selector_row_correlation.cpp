#include "../src/SelectorRowCorrelation.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <string_view>
namespace S=K2040::SelectorRowCorrelation;
int main() {
    S::Observation o{};
    o.kind=S::Kind::kQuestRoot;
    o.rowFormID=0xFE000F11; // Synthetic ESL ID, never a quest whitelist
    o.rowInstanceID=7;
    o.rowCategory=10;
    o.nativeQuestFound=true;
    o.nativeFormID=*o.rowFormID;
    o.nativeCurrentInstanceID=7;
    o.nativeQuestType=10;
    assert(S::Compare(o)==S::Status::kCandidateNotVerified);
    o.kind=S::Kind::kMiscChild;
    o.nativeQuestType=*o.rowCategory=6;
    assert(S::Compare(o)==S::Status::kCandidateNotVerified);
    o.rowCategory.reset();
    assert(S::Compare(o)==S::Status::kMissingCategory);
    o.rowCategory=6;
    o.categoryAmbiguous=true;
    assert(S::Compare(o)==S::Status::kAmbiguousCategory);
    o.categoryAmbiguous=false;
    o.rowCategory=2;
    assert(S::Compare(o)==S::Status::kCategoryMismatch);
    o.rowCategory=6;
    o.rowInstanceID=8;
    assert(S::Compare(o)==S::Status::kInstanceMismatch);
    o.rowInstanceID.reset();
    assert(S::Compare(o)==S::Status::kMissingInstance);
    o.rowInstanceID=7;
    o.nativeQuestFound=false;
    assert(S::Compare(o)==S::Status::kUnresolvedQuest);
    o.nativeQuestFound=true;
    o.nativeFormID=0xFFABCD01;
    assert(S::Compare(o)==S::Status::kFormMismatch);
    o.kind=S::Kind::kSyntheticMiscRoot;
    o.rowFormID=0;
    o.rowCategory=6;
    assert(S::Compare(o)==S::Status::kSyntheticRootTypeSix);
    o.rowCategory=0;
    assert(S::Compare(o)==S::Status::kSyntheticRootTypeMismatch);
    o.rowCategory.reset();
    assert(S::Compare(o)==S::Status::kSyntheticRootTypeUnknown);
    o.rowFormID.reset();
    assert(S::Compare(o)==S::Status::kInvalidRow);
    o.rowFormID=0x100;
    assert(S::Compare(o)==S::Status::kInvalidRow);
    assert(std::string_view(S::Name(S::Status::kCandidateNotVerified))==
           "structural-candidate-NOT-command-ABI");
    // This research must never change the shipping native writer default.
    static_assert(!K2040::QuestTracking::CanWritePipboy());
}
