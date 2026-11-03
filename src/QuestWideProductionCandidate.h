#pragma once
// Explicitly opted-in, standalone OG163 ordinary/Misc quest-wide candidate.
// Never linked into default read-only Journal; no QA consent or QA UI.
#include "../research/native_one_shot/RepeatableQuestWideController.h"
#include "../research/native_one_shot/OneShotOgDryRun.h"
#include <cstdint>
#include <functional>
#include <memory>
namespace K2040::QuestWideCandidate {
using Controller=K2040::RepeatableQuestWide::Controller;
using Phase=K2040::RepeatableQuestWide::Phase;
enum class Outcome { kRefused,kPending,kConfirmed,kBlocked };
using Report=std::function<void(std::uint32_t,Outcome)>;
// Game-thread-only native observations; no game pointers escape this call.
[[nodiscard]] K2040::OneShotResearch::GenericMiscEvidence
    CaptureOrdinaryQuestEvidence(const K2040::OneShotResearch::Intent&);
[[nodiscard]] bool Queue(const std::shared_ptr<Controller>& controller,
    const char* request,std::uint64_t saveEpoch,std::uint64_t uiEpoch,
    K2040::OneShotResearch::ReadContext readFresh,Report report);
[[nodiscard]] bool Reconcile(const std::shared_ptr<Controller>& controller,
    K2040::OneShotResearch::ReadContext readFresh,Report report);
}
