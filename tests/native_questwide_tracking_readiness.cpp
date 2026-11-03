#include "../src/QuestWideTrackingReadiness.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
using namespace K2040::QuestWideReadiness;
constexpr Snapshot valid{0x12345678u,true,false,true,true,true,true,2u,2u,true};
static_assert(Assess(valid)==Status::kCandidateReadOnly);
static_assert(K2040::QuestTracking::kVerifiedBackend==
              K2040::QuestTracking::WriteBackend::kNone);
static_assert(!K2040::QuestTracking::CanWritePipboy());
int main(){
 auto s=valid;
 assert(Assess(s)==Status::kCandidateReadOnly);
 s.instancePresent=false;
 assert(Assess(s)==Status::kUnverifiedInstance);
 s=valid;s.instanceConsistent=false;
 assert(Assess(s)==Status::kUnverifiedInstance);
 s=valid;s.pipboyFeedMatches=false;
 assert(Assess(s)==Status::kUnverifiedPipboyFeed);
 s=valid;s.miscRows=0;
 assert(Assess(s)==Status::kNoMiscRows);
 s=valid;s.miscRows=513;
 assert(Assess(s)==Status::kNoMiscRows);
 s=valid;s.displayedNativeObjectives=0;
 assert(Assess(s)==Status::kNoDisplayedNativeObjectives);
 s=valid;s.displayedNativeObjectives=513;
 assert(Assess(s)==Status::kNoDisplayedNativeObjectives);
 s=valid;s.questActive=false;
 assert(Assess(s)==Status::kUnavailable);
 s=valid;s.historyOnly=true;
 assert(Assess(s)==Status::kUnavailable);
 s=valid;s.formId=0;
 assert(Assess(s)==Status::kInvalidIdentity);
 s=valid;s.questWide=false;
 assert(Assess(s)==Status::kUnknownScope);
 // Ordinary quests do not require Misc rows, but still require independent
 // current-instance, native-objective and Pip-Boy-feed consistency.
 s=valid;s.isMisc=false;s.miscRows=0;
 assert(Assess(s)==Status::kCandidateReadOnly);
 s=valid;s.instancePresent=true;s.miscRows=2u; // instance 0 remains valid
 assert(Assess(s)==Status::kCandidateReadOnly);
 assert(Name(Status::kCandidateReadOnly)==Name(Assess(s)));
}
