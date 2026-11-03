#pragma once
// Pure Misc TESQuest-wide preflight used by isolated QA and an
// explicitly opted-in candidate. No engine calls or implied authorization.
#include "OneShotIntent.h"
#include <cstdint>
#include <optional>
namespace K2040::OneShotResearch {
struct GenericMiscEvidence {
    std::uint32_t questInstanceID{};
    std::uint32_t nativeDisplayedCount{};
    std::uint32_t miscFeedRows{};
    // Separate ordinary-feed evidence: never substitute a synthetic
    // nested Misc objective row for a normal Pip-Boy quest entry.
    std::uint32_t ordinaryFeedRows{};
    bool selectedIsOrdinary{};
    bool nativeComplete{};
    bool onlyCurrentInstance{};
    bool pipboyFeedComplete{};
    bool feedAgreesWithQuest{};
    bool selectedIsMisc{};
};
[[nodiscard]] constexpr Result ReviewGenericMisc(
    const Intent& intent,const Observation& o,
    std::optional<std::uint32_t> expectedInstance,
    const GenericMiscEvidence& evidence) {
    if(!intent.ticket || !intent.formId || !intent.saveEpoch || !intent.uiEpoch ||
       intent.formId!=o.resolvedFormId || intent.formId!=o.selectedFormId ||
       intent.saveEpoch!=o.saveEpoch || intent.uiEpoch!=o.uiEpoch ||
       !o.og163 || !o.onF4seTask || !o.loaded || !o.journalOpenFocused ||
       !o.previouslyObserved || o.questType!=kMisc ||
       !(o.questFlags & kEnabled) || (o.questFlags & (kCompleted|kFailed)) ||
       !expectedInstance || !evidence.selectedIsMisc ||
       !evidence.nativeComplete || !evidence.onlyCurrentInstance ||
       evidence.questInstanceID!=*expectedInstance ||
       !evidence.nativeDisplayedCount || !evidence.miscFeedRows ||
       evidence.nativeDisplayedCount>512 || evidence.miscFeedRows>512 ||
       !evidence.pipboyFeedComplete || !evidence.feedAgreesWithQuest)
        return Result::kRejected;
    const bool native=bool(o.questFlags & kTracked);
    return native==intent.desiredTracked?Result::kAlreadyCurrent:Result::kWouldWrite;
}
}
