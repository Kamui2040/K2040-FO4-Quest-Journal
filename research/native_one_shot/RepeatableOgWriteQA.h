#pragma once
// Nonshipping, consent-gated controller: shipping writer remains disabled.
#include "RepeatableQuestWideController.h"
#include "OneShotOgDryRun.h"
#include <functional>
#include <memory>
namespace K2040::RepeatableQuestWide {
enum class Outcome { kRefused,kPending,kConfirmed,kBlocked };
using Report=std::function<void(std::uint32_t,Outcome)>;
[[nodiscard]] bool ReadRepeatableConsent();
// A user-initiated acknowledgement inside the isolated QA view is an
// alternative to modifying INI consent. Never persisted across view/save.
void GrantSessionConsent();
void ResetSessionConsent();
[[nodiscard]] bool QueueRepeatable(
    const std::shared_ptr<Controller>& controller, const char* raw,
    std::uint64_t saveEpoch, std::uint64_t uiEpoch,
    K2040::OneShotResearch::ReadContext readFresh, Report report);
[[nodiscard]] bool Reconcile(
    const std::shared_ptr<Controller>& controller,
    K2040::OneShotResearch::ReadContext readFresh, Report report);
}
