#pragma once
// Source-only read-only native observation reusable by isolated QA and
// the separate opt-in candidate; neither bypasses its native writer gate.
#include "OneShotIntent.h"
#if defined(K2040_GENERIC_MISC_QUESTWIDE_QA) || defined(K2040_REPEATABLE_MISC_QA) || defined(K2040_QUESTWIDE_PRODUCTION_CANDIDATE)
#include "GenericMiscQuestWidePolicy.h"
#include <optional>
#endif
#include <cstdint>
#include <functional>
#include <memory>
namespace K2040::OneShotResearch {
    struct LiveJournalContext {
        std::uint32_t selectedId{};
        std::uint64_t saveEpoch{};
        std::uint64_t uiEpoch{};
        bool gameLoaded{};
        bool journalFocused{};
        bool previouslyObserved{};
        bool runtimeOg163Verified{};
    };
    using ReadContext = std::function<LiveJournalContext()>;
    using Report = std::function<void(Result)>;
    // The observation must be performed inside a verified F4SE AddTask.
#if defined(K2040_GENERIC_MISC_QUESTWIDE_QA) || defined(K2040_REPEATABLE_MISC_QA) || defined(K2040_QUESTWIDE_PRODUCTION_CANDIDATE)
    [[nodiscard]] GenericMiscEvidence CaptureGenericMiscEvidence(const Intent&);
#endif
    [[nodiscard]] Observation ObserveOnTask(
        const Intent& intent, const LiveJournalContext& context);
    [[nodiscard]] bool QueueFromExplicitPrismaClick(
        const std::shared_ptr<Gate>& gate, const char* borrowedUiPayload,
        std::uint64_t saveEpoch, std::uint64_t uiEpoch,
        ReadContext readFresh, Report report);
}
