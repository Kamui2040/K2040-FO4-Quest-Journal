#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>

#if defined(K2040_SINGLE_MISC_WRITE_QA) || \
    defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE)
#include "../research/native_one_shot/SingleMiscWriteGate.h"
#endif
namespace K2040::Quests {
#ifdef K2040_SELECTOR_READONLY_QA
    // Anonymous, bounded row schema/type/instance counts. Does not expose
    // actual selectedEntry ABI, quest IDs or native tracking permission.
    [[nodiscard]] std::string CaptureSelectorRowCorrelation();
#endif
#ifdef K2040_TRACKING_INTEGRATED_QA
    // Read-only research: only the isolated QA flavor exposes diagnostics.
    [[nodiscard]] std::string CaptureMiscTrackingProbe(std::uint32_t selectedFormId);
#endif
#if defined(K2040_SINGLE_MISC_WRITE_QA) || \
    defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE)
    // Read game-owned data only on F4SE game thread, after session checks.
    [[nodiscard]] SingleMiscQA::Observation InspectSingleMiscWriteCandidate(
        std::uint32_t selectedFormId);
#endif
    // Builds a standalone, read-only snapshot from the user's current game.
    // On unsupported/early engine states this returns ready=false and no quests.
    [[nodiscard]] std::string CaptureSnapshot(const std::unordered_set<std::uint32_t>& pinnedQuestIds,
                                              const std::unordered_set<std::uint32_t>& favoriteQuestIds,
                                              const std::unordered_set<std::uint32_t>& hiddenQuestIds,
                                              bool autoPinNew=false);
}
