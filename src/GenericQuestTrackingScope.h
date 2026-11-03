#pragma once
// READ-ONLY review of a proposed vanilla tracking toggle.
// Never grants native write authorization, and never calls an engine method.
// The previously captured companion-app-compatible command handler
// resolves a quest instance; its ABI does NOT prove the in-game UI handler.
// Keep this policy distinct from the existing default-deny writer gate.
#include <cstdint>

namespace K2040::GenericQuestTrackingScope {
    enum class Selection : std::uint8_t {
        kQuest, kMiscObjective, kUnknown
    };
    // Which game-owned selectedEntry object produced the would-be command?
    // A PipboyQuestData feed row is NOT proof of this provenance.
    enum class CommandOrigin : std::uint8_t {
        kUnknown,
        kQuestDataFeed,
        kObjectiveDisplayLeaf,
        kQuestListSelectedEntry,
        kNativePopulatedMiscSelectedEntry
    };
    // Distinct engine pathways. The documented Companion App RPC
    // command type 5 accepts [formID, instance, type] but does NOT
    // establish the in-game BGSCodeObj.SetQuestActive callback ABI.
    enum class CommandChannel : std::uint8_t {
        kUnknown,
        kCompanionAppRpcType5,
        kInGameScaleformCodeObject
    };
    enum class InGameCallbackEvidence : std::uint8_t {
        kUnknown,
        kSelectedEntrySchemaOnly,
        kBGSCodeObjCallbackAbiObserved
    };
    enum class Outcome : std::uint8_t {
        kInvalidIdentity, kStaleOrIncomplete, kWrongInstance,
        kQuestNotRunning, kAlreadyInRequestedState,
        kRequiresExplicitQuestWideSelection, kCommandInputUnverified,
        kQuestInstanceCandidateReadOnly
    };
    struct Observation {
        std::uint32_t selectedFormID{};
        std::uint32_t observedFormID{};
        std::uint32_t currentInstanceID{};
        std::uint32_t selectedInstanceID{};
        Selection selection{Selection::kUnknown};
        CommandOrigin commandOrigin{CommandOrigin::kUnknown};
        // Independently validated in-game BGSCodeObj.SetQuestActive
        // callback signature and its actual selectedEntry arguments.
        // The companion-app JSON RPC tuple never sets these values.
        CommandChannel commandChannel{CommandChannel::kUnknown};
        bool inGameCallbackArgumentsVerified{false};
        InGameCallbackEvidence inGameCallbackEvidence{
            InGameCallbackEvidence::kUnknown};
        bool selectedInstanceTyped{false};
        bool runtimeOG163Verified{false};
        bool completePipboySnapshot{false};
        bool freshGameAndView{false};
        bool questRunning{false};
        bool questCompleted{false};
        bool questFailed{false};
        bool currentTrackingKnown{false};
        bool currentlyTracked{false};
        bool desiredTracked{false};
        // Number of distinct selected-quest Misc rows currently displayed.
        // Never infer "independent objective tracking" from this count.
        std::uint32_t displayedMiscRowsForQuest{};
        // A user action labeled "Track entire quest", not a click on an
        // individual objective. Important when several rows share a quest.
        bool explicitQuestWideIntent{false};
    };
    [[nodiscard]] constexpr Outcome Review(const Observation& o) noexcept {
        if (!o.selectedFormID || o.selectedFormID!=o.observedFormID ||
            o.selection==Selection::kUnknown)
            return Outcome::kInvalidIdentity;
        if (!o.runtimeOG163Verified || !o.completePipboySnapshot ||
            !o.freshGameAndView || !o.currentTrackingKnown)
            return Outcome::kStaleOrIncomplete;
        // The Companion App handler checks a quest instance; this
        // remains a conservative identity requirement, not a proven
        // ABI for the in-game BGSCodeObj callback.
        if (!o.selectedInstanceTyped ||
            o.selectedInstanceID!=o.currentInstanceID)
            return Outcome::kWrongInstance;
        if (!o.questRunning || o.questCompleted || o.questFailed)
            return Outcome::kQuestNotRunning;
        if (o.currentlyTracked==o.desiredTracked)
            return Outcome::kAlreadyInRequestedState;
        if (o.selection==Selection::kMiscObjective) {
            if (!o.displayedMiscRowsForQuest)
                return Outcome::kStaleOrIncomplete;
            // A quest-level toggle would also affect sibling Misc rows.
            // It cannot honestly satisfy an objective-specific request.
            if (o.displayedMiscRowsForQuest>1 &&
                !o.explicitQuestWideIntent)
                return Outcome::kRequiresExplicitQuestWideSelection;
        }
        // Ordinary list selects the quest list's selectedEntry. Misc
        // selects a *native-populated* objectives-list selectedEntry.
        // Neither is interchangeable with generic PipboyQuestData
        // "objectives" display leaves or structural feed rows.
        const auto requiredOrigin=o.selection==Selection::kMiscObjective
            ? CommandOrigin::kNativePopulatedMiscSelectedEntry
            : CommandOrigin::kQuestListSelectedEntry;
        if (o.commandOrigin!=requiredOrigin ||
            o.commandChannel!=CommandChannel::kInGameScaleformCodeObject ||
            !o.inGameCallbackArgumentsVerified ||
            o.inGameCallbackEvidence!=
                InGameCallbackEvidence::kBGSCodeObjCallbackAbiObserved)
            return Outcome::kCommandInputUnverified;
        // This is STILL only a read-only research candidate. Never
        // authorize or invoke a native engine function from this value.
        return Outcome::kQuestInstanceCandidateReadOnly;
    }

    [[nodiscard]] constexpr const char* Name(Outcome o) noexcept {
        switch(o) {
        case Outcome::kInvalidIdentity: return "invalid-quest-identity";
        case Outcome::kStaleOrIncomplete: return "stale-or-incomplete";
        case Outcome::kWrongInstance: return "quest-instance-mismatch";
        case Outcome::kQuestNotRunning: return "quest-not-running";
        case Outcome::kAlreadyInRequestedState: return "already-in-requested-state";
        case Outcome::kRequiresExplicitQuestWideSelection:
            return "whole-quest-confirmation-required";
        case Outcome::kCommandInputUnverified:
            return "in-game-code-object-origin-or-abi-unverified";
        case Outcome::kQuestInstanceCandidateReadOnly:
            return "quest-instance-candidate-NOT-write-authorized";
        }
        return "unknown";
    }
}
