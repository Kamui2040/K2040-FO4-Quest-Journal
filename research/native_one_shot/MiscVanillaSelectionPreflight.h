#pragma once
// Research-only, engine-independent decision record for OG Pip-Boy Misc
// selection. This header cannot perform a write. It is NOT an ABI binding.
#include <cstdint>

namespace K2040::MiscVanillaResearch {

    // Static analysis of vanilla Pipboy_DataPage.swf showed that a Misc
    // objective is passed as a selected nested Pip-Boy row, not a top-level
    // quest entry. The row's "instance" meaning is currently unverified.
    inline constexpr bool kNativeMiscWritesAuthorized = false;
    inline constexpr std::uint8_t kMiscQuestType = 6u;
    inline constexpr std::uint16_t kQuestRunning = 0x0001u;
    inline constexpr std::uint16_t kQuestCompleted = 0x0002u;
    inline constexpr std::uint16_t kQuestFailed = 0x0040u;
    inline constexpr std::uint16_t kQuestTracked = 0x0800u;

    struct Snapshot {
        bool correctOgRuntime{};
        bool supportedF4seTask{};
        bool loadedAndFocused{};
        bool currentSaveAndView{};
        bool playerObservedAndSelected{};
        std::uint32_t selectedFormId{};
        std::uint32_t resolvedFormId{};
        std::uint8_t questType{};
        std::uint16_t questFlags{};
        std::uint16_t matchingRootRows{};
        std::uint16_t matchingMiscRows{};
        std::uint16_t displayedObjectiveCandidates{};
        bool truncated{};
        bool nestedActiveTyped{};
        bool nestedActive{};
        bool nestedEnabledTyped{};
        bool nestedEnabled{};
        bool instanceFieldTyped{};
    };

    enum class Decision : std::uint8_t {
        kReject,            // missing game/lifecycle context or ineligible quest
        kUnverified,        // multiple rows, mismatched feed, or ambiguous schema
        kAlreadyCurrent,    // no state transition to test
        kCandidateOnly      // would require separate native ABI + user QA approval
    };

    [[nodiscard]] constexpr Decision Review(
        const Snapshot& o, bool desiredTracked) noexcept {
        if (!o.correctOgRuntime || !o.supportedF4seTask ||
            !o.loadedAndFocused || !o.currentSaveAndView ||
            !o.playerObservedAndSelected || !o.selectedFormId ||
            o.resolvedFormId != o.selectedFormId ||
            o.questType != kMiscQuestType ||
            (o.questFlags & kQuestRunning) == 0 ||
            (o.questFlags & (kQuestCompleted | kQuestFailed)) != 0)
            return Decision::kReject;

        // An objective list can have multiple independently exposed rows
        // for one TESQuest, and "instance" is not yet decoded. Never guess
        // a row from the first match or numeric objective index candidates.
        if (o.truncated || o.matchingRootRows != 0 ||
            o.matchingMiscRows != 1 ||
            o.displayedObjectiveCandidates != 1 ||
            !o.nestedActiveTyped || !o.nestedEnabledTyped ||
            !o.nestedEnabled || !o.instanceFieldTyped)
            return Decision::kUnverified;

        const bool nativeTracked=(o.questFlags & kQuestTracked)!=0;
        if (nativeTracked != o.nestedActive)
            return Decision::kUnverified;
        if (nativeTracked == desiredTracked)
            return Decision::kAlreadyCurrent;

        // No other code may interpret kCandidateOnly as permission to write.
        // Runtime interface/calling convention + effects have NOT been
        // independently confirmed; production and QA write paths stay OFF.
        return Decision::kCandidateOnly;
    }
}
