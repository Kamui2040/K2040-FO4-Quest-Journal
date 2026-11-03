#pragma once
// Scope-specific fail-closed policy for the standalone OG163 normal-UI
// candidate; no native calls. The tested Misc policy remains unchanged.
#include "GenericMiscQuestWidePolicy.h"
namespace K2040::OneShotResearch {
[[nodiscard]] constexpr Result ReviewOrdinaryQuestWide(
    const Intent& intent,const Observation& o,
    std::optional<std::uint32_t> instance,
    const GenericMiscEvidence& e) noexcept {
    if(!intent.ticket || !intent.formId || !intent.saveEpoch || !intent.uiEpoch ||
       o.resolvedFormId!=intent.formId || o.selectedFormId!=intent.formId ||
       o.saveEpoch!=intent.saveEpoch || o.uiEpoch!=intent.uiEpoch ||
       !o.og163 || !o.onF4seTask || !o.loaded || !o.journalOpenFocused ||
       !o.previouslyObserved || o.questType==kMisc ||
       !(o.questFlags & kEnabled) || (o.questFlags & (kCompleted|kFailed)) ||
       !instance || !e.selectedIsOrdinary || e.selectedIsMisc ||
       !e.nativeComplete || !e.onlyCurrentInstance ||
       e.questInstanceID!=*instance ||
       !e.nativeDisplayedCount || e.nativeDisplayedCount>512u ||
       e.miscFeedRows || e.ordinaryFeedRows!=1 ||
       !e.pipboyFeedComplete || !e.feedAgreesWithQuest ||
       !o.inNormalPipboyFeed || !o.feedHasTrackedBit ||
       o.feedTracked!=bool(o.questFlags & kTracked))
        return Result::kRejected;
    return bool(o.questFlags & kTracked)==intent.desiredTracked
        ? Result::kAlreadyCurrent : Result::kWouldWrite;
}
[[nodiscard]] constexpr Result ReviewQuestWide(
    const Intent& intent,const Observation& o,
    std::optional<std::uint32_t> instance,
    const GenericMiscEvidence& e) noexcept {
    return o.questType==kMisc
        ? (e.selectedIsOrdinary || e.ordinaryFeedRows ? Result::kRejected :
            ReviewGenericMisc(intent,o,instance,e))
        : ReviewOrdinaryQuestWide(intent,o,instance,e);
}
}
