#include "../src/GenericQuestInstanceSummary.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
namespace I=K2040::GenericQuestInstance;
int main() {
    static_assert(!K2040::QuestTracking::CanWritePipboy());
    constexpr std::uint32_t q=0xFE002100u;
    I::Index index;
    assert(index.Resolve(q,0)==I::State::kNoDisplayedObjectives);
    index.Observe(q,0,0,true); // index 0 and instance 0 are valid
    index.Observe(q,8,0,true); // two objectives in same instance
    assert(index.Resolve(q,0)==I::State::kConsistent);
    assert(index.DisplayedCount(q)==2u);
    assert(index.Resolve(q,1)==I::State::kCurrentInstanceMismatch);
    index.Observe(q,8,7,false); // dormant objective is not a displayed candidate
    assert(index.DisplayedCount(q)==2u);
    index.Observe(q,9,2,true); // radiant repeat / different live instance
    assert(index.Resolve(q,0)==I::State::kMultipleInstances);
    assert(index.Resolve(q,2)==I::State::kMultipleInstances);
    I::Index truncated;
    for(std::size_t i=0;i<I::kMaxObservations;++i)
        truncated.Observe(q,static_cast<std::uint32_t>(i),1,true);
    assert(truncated.Complete());
    truncated.Observe(q,777,1,true);
    assert(!truncated.Complete());
    assert(truncated.Resolve(q,1)==I::State::kUnavailable);
    assert(truncated.DisplayedCount(q)==0);
    I::Index noPlayer;
    noPlayer.MarkUnavailable();
    assert(noPlayer.Resolve(q,0)==I::State::kUnavailable);
    I::Index other;
    other.Observe(q+1,2,5,true);
    assert(other.Resolve(q,5)==I::State::kNoDisplayedObjectives);
    assert(other.Resolve(q+1,5)==I::State::kConsistent);
    assert(other.Resolve(0,5)==I::State::kUnavailable);
}
