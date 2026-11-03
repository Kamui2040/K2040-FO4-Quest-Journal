#pragma once
// Independent quest-wide tracking research: PURE policy, no engine IO.
// Never authorizes a native write, game callback or reference mutation.
#include <cstddef>
#include <cstdint>
#include <span>
namespace K2040::QuestWideNativeResearch {
inline constexpr std::uint32_t kEnabled=0x1u;
inline constexpr std::uint32_t kCompleted=0x2u;
inline constexpr std::uint32_t kFailed=0x40u;
inline constexpr std::uint32_t kActive=0x800u;
inline constexpr std::size_t kMaxFeedRows=512u;
enum class Selection : std::uint8_t { kOrdinaryQuest, kMiscObjectiveRow };
enum class RowKind : std::uint8_t { kOrdinaryQuest, kMiscObjective };
enum class Outcome : std::uint8_t {
    kInvalidRequest, kStaleSession, kInvalidQuest, kUnusableQuest,
    kIncompleteFeed, kMissingRow, kAmbiguousRowKind, kInstanceMismatch,
    kFeedStateMismatch, kQuestWideIntentRequired, kMissingSideEffectContext,
    kAlreadyMatching, kCandidateNeedsVanillaEquivalenceProof
};
struct Request {
    std::uint32_t formID{}, selectedInstanceID{};
    std::uint64_t saveEpoch{}, viewEpoch{};
    bool desiredActive{};
    Selection selection{Selection::kOrdinaryQuest};
    bool explicitQuestWideIntent{};
};
struct Snapshot {
    std::uint32_t formID{}, selectedFormID{}, currentInstanceID{};
    std::uint64_t saveEpoch{}, viewEpoch{};
    std::uint32_t flags{};
    bool instanceIdentityTyped{}, runtimeOG163{}, gameLoaded{};
    bool gameThread{}, journalFocused{}, formIsQuest{}, questRunning{};
    bool completePipboyFeed{}, aliasTopologyAvailable{};
    bool questAndMapConsumersAvailable{};
};
struct FeedRow {
    std::uint32_t formID{}, instanceID{};
    RowKind kind{RowKind::kOrdinaryQuest};
    bool typedAndComplete{}, active{};
};
struct Result {
    Outcome outcome{Outcome::kInvalidRequest};
    std::size_t affectedVisibleRows{};
    bool writeAuthorized{false};
    bool vanillaEquivalenceProven{false};
};
// A valid dry-run conclusion is never a callable or writable plan.
[[nodiscard]] inline Result Review(
    const Request& req, const Snapshot& state,
    std::span<const FeedRow> rows) noexcept {
    if (!req.formID || !req.saveEpoch || !req.viewEpoch ||
        (req.selection!=Selection::kOrdinaryQuest &&
         req.selection!=Selection::kMiscObjectiveRow) ||
        req.formID!=state.formID || req.formID!=state.selectedFormID)
        return {Outcome::kInvalidRequest};
    if (req.saveEpoch!=state.saveEpoch ||
        req.viewEpoch!=state.viewEpoch ||
        !state.runtimeOG163 || !state.gameLoaded ||
        !state.gameThread || !state.journalFocused)
        return {Outcome::kStaleSession};
    if (!state.formIsQuest)
        return {Outcome::kInvalidQuest};
    if (!state.instanceIdentityTyped ||
        req.selectedInstanceID!=state.currentInstanceID)
        return {Outcome::kInstanceMismatch};
    if (!(state.flags&kEnabled) ||
        (state.flags&(kCompleted|kFailed)) ||
        !state.questRunning)
        return {Outcome::kUnusableQuest};
    if (!state.completePipboyFeed || rows.empty() ||
        rows.size()>kMaxFeedRows)
        return {Outcome::kIncompleteFeed};
    std::size_t matches=0;
    bool hasOrdinary=false,hasMisc=false;
    const bool currentlyActive=(state.flags&kActive)!=0u;
    for(const FeedRow& row : rows) {
        if(row.formID!=req.formID)
            continue;
        if(!row.typedAndComplete)
            return {Outcome::kIncompleteFeed};
        if(row.instanceID!=state.currentInstanceID)
            return {Outcome::kInstanceMismatch};
        if(row.active!=currentlyActive)
            return {Outcome::kFeedStateMismatch};
        if(row.kind==RowKind::kOrdinaryQuest)
            hasOrdinary=true;
        else if(row.kind==RowKind::kMiscObjective)
            hasMisc=true;
        else
            return {Outcome::kAmbiguousRowKind};
        ++matches;
    }
    if(!matches)
        return {Outcome::kMissingRow};
    if(hasOrdinary&&hasMisc)
        return {Outcome::kAmbiguousRowKind};
    if((req.selection==Selection::kOrdinaryQuest && !hasOrdinary) ||
       (req.selection==Selection::kMiscObjectiveRow && !hasMisc))
        return {Outcome::kAmbiguousRowKind};
    // A Misc-row click MUST NOT silently activate a whole TESQuest;
    // even one visible row may have additional hidden sibling objectives.
    if(!req.explicitQuestWideIntent)
        return {Outcome::kQuestWideIntentRequired,matches};
    if(currentlyActive==req.desiredActive)
        return {Outcome::kAlreadyMatching,matches};
    if(!state.aliasTopologyAvailable ||
       !state.questAndMapConsumersAvailable)
        return {Outcome::kMissingSideEffectContext,matches};
    return {Outcome::kCandidateNeedsVanillaEquivalenceProof,matches};
}
[[nodiscard]] constexpr const char* OutcomeName(Outcome result) noexcept {
    switch(result) {
    case Outcome::kInvalidRequest:return "invalid-request";
    case Outcome::kStaleSession:return "stale-session";
    case Outcome::kInvalidQuest:return "invalid-quest";
    case Outcome::kUnusableQuest:return "unusable-quest";
    case Outcome::kIncompleteFeed:return "incomplete-feed";
    case Outcome::kMissingRow:return "missing-row";
    case Outcome::kAmbiguousRowKind:return "ambiguous-row-kind";
    case Outcome::kInstanceMismatch:return "instance-mismatch";
    case Outcome::kFeedStateMismatch:return "feed-state-mismatch";
    case Outcome::kQuestWideIntentRequired:
        return "explicit-quest-wide-intent-required";
    case Outcome::kMissingSideEffectContext:return "side-effect-context-unavailable";
    case Outcome::kAlreadyMatching:return "already-matching";
    case Outcome::kCandidateNeedsVanillaEquivalenceProof:
        return "candidate-NOT-native-write-authorized";
    }
    return "unknown";
}
static_assert(kActive==0x800u);
static_assert(kMaxFeedRows==512u);
static_assert(!Result{}.writeAuthorized);
static_assert(!Result{}.vanillaEquivalenceProven);
} // namespace K2040::QuestWideNativeResearch
