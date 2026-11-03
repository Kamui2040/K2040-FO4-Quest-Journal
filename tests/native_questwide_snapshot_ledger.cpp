#include "../research/native_questwide/NativeQuestWideSnapshot.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <optional>
using namespace K2040::QuestWideNativeResearch;
int main() {
    static_assert(!K2040::QuestTracking::CanWritePipboy());
    FeedLedger l;
    constexpr std::uint32_t id=0xFE003ABC;
    l.Observe(0u,std::nullopt,std::nullopt,RowKind::kOrdinaryQuest,id);
    l.Observe(id,19u,false,RowKind::kMiscObjective,id);
    l.Observe(0x100u,19u,false,RowKind::kOrdinaryQuest,id);
    l.Observe(id,19u,false,RowKind::kMiscObjective,id);
    assert(l.scanned==4 && l.count==2 && l.complete && !l.truncated);
    assert(l.Rows()[0].typedAndComplete && l.Rows()[1].typedAndComplete);
    assert(l.Rows()[0].formID==id && l.Rows()[1].instanceID==19u);
    auto weak=FeedLedger{};
    weak.Observe(id,std::nullopt,false,RowKind::kMiscObjective,id);
    assert(weak.count==1 && !weak.Rows()[0].typedAndComplete);
    weak.Observe(id,19u,std::nullopt,RowKind::kMiscObjective,id);
    assert(weak.count==2 && !weak.Rows()[1].typedAndComplete);
    weak.Observe(std::nullopt,19u,false,RowKind::kMiscObjective,id);
    assert(!weak.complete);
    auto overflowing=FeedLedger{};
    for(std::size_t n=0;n<kMaxFeedRows;++n)
        overflowing.Observe(0xFFFFu,19u,true,RowKind::kOrdinaryQuest,id);
    assert(overflowing.complete && overflowing.scanned==kMaxFeedRows);
    overflowing.Observe(id,19u,false,RowKind::kMiscObjective,id);
    assert(overflowing.truncated && !overflowing.complete);
    assert(overflowing.count==0);
    NativeCapture cap{};
    cap.selectedInstanceIDFromContext=19;
    Request req{id,19,3,8,true,Selection::kMiscObjectiveRow,true};
    cap.state={id,id,19,3,8,kEnabled,
         true,true,true,true,true,true,true,true,false,true};
    cap.feed.Observe(id,19,false,RowKind::kMiscObjective,id);
    cap.state.completePipboyFeed=cap.feed.complete;
    assert(cap.Review(req).outcome==Outcome::kMissingSideEffectContext);
    assert(!cap.Review(req).writeAuthorized);
    ++req.selectedInstanceID;
    assert(cap.Review(req).outcome==Outcome::kInstanceMismatch);
    --req.selectedInstanceID;
    cap.state.instanceIdentityTyped=false;
    assert(cap.Review(req).outcome==Outcome::kInstanceMismatch);
}
