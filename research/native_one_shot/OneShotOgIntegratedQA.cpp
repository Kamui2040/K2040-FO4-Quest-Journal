// Non-shipping, separately compiled integrated Journal native track/untrack QA.
// The user must explicitly ENABLE TEST TRACKING in the focused Journal.
// No INI file, no automatic startup write, never AddTaskPermanent.
// Per-process attempt cap deliberately small; no silent retry on failures.
#include "OneShotOgIntegratedQA.h"
#include "../f4se_og_api/TaskInterface0623.h"
#include "../../src/QuestTrackingPolicy.h"
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <REL/Relocation.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif
#include <cstdint>
#include <string_view>
#include <utility>

namespace K2040::OneShotResearch {
    namespace {
        using NativeSetter = void (*)(RE::TESQuest*, bool);
        constexpr auto kSetActiveId = 1567562u;
        constexpr auto kOGSetActiveRva = 0x005D6AC0u;
    }

    [[nodiscard]] bool QueueIntegratedTracking(
        const std::shared_ptr<Gate>& gate, const char* raw,
        std::uint64_t saveEpoch, std::uint64_t uiEpoch,
        std::atomic_bool& enabled,
        std::atomic_uint32_t& attempts,
        ReadContext fresh,
        std::function<void(IntegratedResult)> report) {
        if (!gate || !raw || !fresh || !report ||
            !enabled.load(std::memory_order_acquire)) {
            REX::WARN("IntegratedTrackingQA: REQUEST blocked reason=not-enabled-or-invalid");
            return false;
        }
        const auto n=strnlen_s(raw,11);
        if (n!=10) {
            REX::WARN("IntegratedTrackingQA: REQUEST blocked reason=payload-length");
            return false;
        }
        const auto request=K2040::QuestTracking::ParseRequest(
            std::string_view(raw,n));
        if (!request) {
            REX::WARN("IntegratedTrackingQA: REQUEST blocked reason=payload-invalid");
            return false;
        }
        const auto* task=F4SE::GetTaskInterface();
        if (!task || task->Version()!=OGF4SE::ABI0623::kTaskInterfaceVersion) {
            REX::WARN("IntegratedTrackingQA: REQUEST blocked reason=unsupported-task-abi");
            return false;
        }
        const auto intent=gate->Offer(
            request->formId,request->desiredTracked,saveEpoch,uiEpoch);
        if (!intent) {
            REX::WARN("IntegratedTrackingQA: REQUEST blocked reason=stale-or-pending");
            return false;
        }

        // Reserve an attempt BEFORE queuing any game-state write.
        // Even a stale/canceled/denied action spends the attempt.
        if (!ReserveIntegratedAttempt(attempts)) {
            REX::WARN("IntegratedTrackingQA: REQUEST seq={} blocked reason=budget-exhausted",
                intent->ticket);
            (void)gate->Settle(*intent);
            return false;
        }
        REX::WARN(
            "IntegratedTrackingQA: REQUEST seq={} desired={} reserved={} "
            "event=AddTask (FormID omitted)",
            intent->ticket,intent->desiredTracked,
            attempts.load(std::memory_order_acquire));

        task->AddTask([
            gate,requested=*intent,&enabled,
            fresh=std::move(fresh),report=std::move(report)]()mutable {
            REX::WARN("IntegratedTrackingQA: TASK seq={} started desired={}",
                requested.ticket,requested.desiredTracked);
            if (!enabled.load(std::memory_order_acquire) ||
                !gate->Current(requested)) {
                REX::WARN("IntegratedTrackingQA: TASK seq={} canceled before read",
                    requested.ticket);
                return;
            }
            const auto context=fresh();
            if (!enabled.load(std::memory_order_acquire) ||
                !gate->Current(requested)) {
                REX::WARN("IntegratedTrackingQA: TASK seq={} canceled after context",
                    requested.ticket);
                return;
            }
            const auto before=ObserveOnTask(requested,context);
            const auto decision=ReviewOnGameTask(requested,before);
            const char* preflight =
                decision==Result::kWouldWrite ? "would-write" :
                decision==Result::kAlreadyCurrent ? "already" :
                decision==Result::kUnverified ? "unverified" : "rejected";
            REX::WARN("IntegratedTrackingQA: TASK seq={} preflight={} "
                "nativeTracked={} feedValid={} feedTracked={}",
                requested.ticket,preflight,bool(before.questFlags & kTracked),
                before.inNormalPipboyFeed && before.feedHasTrackedBit,
                before.feedTracked);
            auto result=IntegratedResult::kRejected;

            if (decision==Result::kAlreadyCurrent)
                result=IntegratedResult::kAlready;
            else if (decision==Result::kUnverified)
                result=IntegratedResult::kUnverified;
            else if (decision==Result::kWouldWrite &&
                     enabled.load(std::memory_order_acquire) &&
                     gate->Current(requested)) {
                const auto base=reinterpret_cast<std::uintptr_t>(
                    GetModuleHandleW(nullptr));
                REL::Relocation<NativeSetter> setter{REL::ID(kSetActiveId)};
                if (base && setter.address()==base+kOGSetActiveRva) {
                    auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(
                        requested.formId);
                    const auto flags=quest ? static_cast<std::uint16_t>(
                        quest->data.flags):0u;
                    if (quest && quest->GetFormID()==before.resolvedFormId &&
                        flags==before.questFlags &&
                        context.saveEpoch==requested.saveEpoch &&
                        context.uiEpoch==requested.uiEpoch &&
                        enabled.load(std::memory_order_acquire) &&
                        gate->Current(requested)) {
                        REX::WARN(
                            "IntegratedTrackingQA: TASK seq={} SetActive ENTER "
                            "desired={} (FormID omitted)",
                            requested.ticket,requested.desiredTracked);
                        setter(quest,requested.desiredTracked);
                        const auto after=ObserveOnTask(requested,context);
                        const auto actual=bool(after.questFlags & kTracked);
                        const auto feedOk=after.inNormalPipboyFeed &&
                            after.feedHasTrackedBit && after.feedTracked==actual;
                        result=(actual==requested.desiredTracked && feedOk)
                            ? IntegratedResult::kConfirmed
                            : IntegratedResult::kProvisional;
                        REX::WARN(
                            "IntegratedTrackingQA: TASK seq={} SetActive RETURNED "
                            "nativeTracked={} feedValid={} feedTracked={} result={} "
                            "(gameplay confirmation still needed)",
                            requested.ticket,actual,feedOk,after.feedTracked,
                            result==IntegratedResult::kConfirmed ?
                                "confirmed" : "provisional");
                    }
                }
            }
            // An ambiguous readback requires the player to check the
            // vanilla Pip-Boy/compass before explicitly re-enabling QA.
            if (result==IntegratedResult::kProvisional ||
                result==IntegratedResult::kUnverified)
                enabled.store(false,std::memory_order_release);
            const char* resultLabel =
                result==IntegratedResult::kConfirmed ? "confirmed" :
                result==IntegratedResult::kProvisional ? "provisional" :
                result==IntegratedResult::kAlready ? "already" :
                result==IntegratedResult::kUnverified ? "unverified" : "rejected";
            const auto settled=gate->Settle(requested);
            REX::WARN("IntegratedTrackingQA: TASK seq={} END result={} "
                      "delivered={} (FormID omitted)",
                      requested.ticket,resultLabel,settled);
            if (settled)report(result);
        });
        return true;
    }
}
