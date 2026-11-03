#include "../research/native_questwide/QuestWideNativePreflight.h"
#include "../src/QuestTrackingPolicy.h"
#include <array>
#include <cassert>
#include <string_view>
#include <vector>
namespace Q=K2040::QuestWideNativeResearch;
static constexpr std::uint32_t id=0xFE003ABCu;
static constexpr std::uint64_t save=7,view=23;
int main() {
    static_assert(!K2040::QuestTracking::CanWritePipboy());
    Q::Request req{id,19,save,view,true,Q::Selection::kMiscObjectiveRow,true};
    Q::Snapshot s{id,id,19,save,view,Q::kEnabled,
                  true,true,true,true,true,true,true,true,true,true};
    std::vector<Q::FeedRow> rows{{id,19,Q::RowKind::kMiscObjective,true,false}};
    const auto expect=[&](Q::Outcome wanted) {
        auto result=Q::Review(req,s,rows);
        assert(result.outcome==wanted);
        assert(!result.writeAuthorized);
        assert(!result.vanillaEquivalenceProven);
    };
    expect(Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
    assert(Q::Review(req,s,rows).affectedVisibleRows==1);
    req.explicitQuestWideIntent=false;
    expect(Q::Outcome::kQuestWideIntentRequired);
    req.explicitQuestWideIntent=true;
    rows.push_back({id,19,Q::RowKind::kMiscObjective,true,false});
    assert(Q::Review(req,s,rows).affectedVisibleRows==2);
    expect(Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
    req.explicitQuestWideIntent=false;
    expect(Q::Outcome::kQuestWideIntentRequired);
    req.explicitQuestWideIntent=true;
    rows.back().instanceID++;
    expect(Q::Outcome::kInstanceMismatch);
    rows.back().instanceID--;
    rows.back().active=true;
    expect(Q::Outcome::kFeedStateMismatch);
    rows.back().active=false;
    rows.back().kind=Q::RowKind::kOrdinaryQuest;
    expect(Q::Outcome::kAmbiguousRowKind);
    rows.back().kind=Q::RowKind::kMiscObjective;
    rows.back().typedAndComplete=false;
    expect(Q::Outcome::kIncompleteFeed);
    rows.back().typedAndComplete=true;
    rows.pop_back();
    s.aliasTopologyAvailable=false;
    expect(Q::Outcome::kMissingSideEffectContext);
    s.aliasTopologyAvailable=true;
    s.questAndMapConsumersAvailable=false;
    expect(Q::Outcome::kMissingSideEffectContext);
    s.questAndMapConsumersAvailable=true;
    rows.front().active=true;
    expect(Q::Outcome::kFeedStateMismatch);
    rows.front().active=false;
    s.flags|=Q::kActive;
    expect(Q::Outcome::kFeedStateMismatch);
    rows.front().active=true;
    expect(Q::Outcome::kAlreadyMatching);
    req.desiredActive=false;
    expect(Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
    rows.front().active=false;
    s.flags=Q::kEnabled;
    req.desiredActive=true;
    expect(Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
    s.flags=0;
    expect(Q::Outcome::kUnusableQuest);
    s.flags=Q::kEnabled|Q::kCompleted;
    expect(Q::Outcome::kUnusableQuest);
    s.flags=Q::kEnabled|Q::kFailed;
    expect(Q::Outcome::kUnusableQuest);
    s.flags=Q::kEnabled;
    s.questRunning=false;
    expect(Q::Outcome::kUnusableQuest);
    s.questRunning=true;
    s.formIsQuest=false;
    expect(Q::Outcome::kInvalidQuest);
    s.formIsQuest=true;
    s.runtimeOG163=false;
    expect(Q::Outcome::kStaleSession);
    s.runtimeOG163=true;
    s.gameLoaded=false;
    expect(Q::Outcome::kStaleSession);
    s.gameLoaded=true;
    s.gameThread=false;
    expect(Q::Outcome::kStaleSession);
    s.gameThread=true;
    s.journalFocused=false;
    expect(Q::Outcome::kStaleSession);
    s.journalFocused=true;
    ++s.saveEpoch;
    expect(Q::Outcome::kStaleSession);
    --s.saveEpoch;
    ++s.viewEpoch;
    expect(Q::Outcome::kStaleSession);
    --s.viewEpoch;
    s.instanceIdentityTyped=false;
    expect(Q::Outcome::kInstanceMismatch);
    s.instanceIdentityTyped=true;
    ++s.currentInstanceID;
    expect(Q::Outcome::kInstanceMismatch);
    --s.currentInstanceID;
    s.selectedFormID=0;
    expect(Q::Outcome::kInvalidRequest);
    s.selectedFormID=id;
    s.completePipboyFeed=false;
    expect(Q::Outcome::kIncompleteFeed);
    s.completePipboyFeed=true;
    rows[0].formID=0;
    expect(Q::Outcome::kMissingRow);
    rows[0].formID=id;
    req.selection=Q::Selection::kOrdinaryQuest;
    expect(Q::Outcome::kAmbiguousRowKind);
    rows[0].kind=Q::RowKind::kOrdinaryQuest;
    expect(Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
    req.selection=Q::Selection::kMiscObjectiveRow;
    rows[0].kind=Q::RowKind::kMiscObjective;
    req.selectedInstanceID++;
    expect(Q::Outcome::kInstanceMismatch);
    req.selectedInstanceID--;
    req.formID=0;
    expect(Q::Outcome::kInvalidRequest);
    req.formID=id;
    req.saveEpoch=0;
    expect(Q::Outcome::kInvalidRequest);
    req.saveEpoch=save;
    req.viewEpoch=0;
    expect(Q::Outcome::kInvalidRequest);
    req.viewEpoch=view;
    req.selection=static_cast<Q::Selection>(255);
    expect(Q::Outcome::kInvalidRequest);
    req.selection=Q::Selection::kMiscObjectiveRow;
    rows[0].kind=static_cast<Q::RowKind>(255);
    expect(Q::Outcome::kAmbiguousRowKind);
    rows[0].kind=Q::RowKind::kMiscObjective;
    rows.clear();
    expect(Q::Outcome::kIncompleteFeed);
    rows.assign(Q::kMaxFeedRows+1,{0,0,Q::RowKind::kOrdinaryQuest,true,false});
    expect(Q::Outcome::kIncompleteFeed);
    rows.assign(Q::kMaxFeedRows,{0,0,Q::RowKind::kOrdinaryQuest,true,false});
    rows.front()={id,19,Q::RowKind::kMiscObjective,true,false};
    expect(Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
    rows.assign(1,{id,19,Q::RowKind::kMiscObjective,true,false});
    // Check all 65,536 low-bit combinations of native quest flags.
    for(std::uint32_t flags=0;flags<65536;flags++) {
        s.flags=flags;
        rows.front().active=(flags&Q::kActive)!=0;
        auto result=Q::Review(req,s,rows);
        if(!(flags&Q::kEnabled) || (flags&(Q::kCompleted|Q::kFailed)))
            assert(result.outcome==Q::Outcome::kUnusableQuest);
        else if(flags&Q::kActive)
            assert(result.outcome==Q::Outcome::kFeedStateMismatch ||
                   result.outcome==Q::Outcome::kAlreadyMatching);
        else assert(result.outcome==Q::Outcome::kCandidateNeedsVanillaEquivalenceProof);
        assert(!result.writeAuthorized && !result.vanillaEquivalenceProven);
    }
    assert(std::string_view(Q::OutcomeName(
        Q::Outcome::kCandidateNeedsVanillaEquivalenceProof))==
        "candidate-NOT-native-write-authorized");
}
