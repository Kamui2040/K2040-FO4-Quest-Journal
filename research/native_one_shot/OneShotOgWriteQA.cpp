// NON-SHIPPING experiment. Enabled ONLY with explicit opt-in xmake build
// flavor, two exact YES values in the separate QA INI, a deliberate
// in-Journal click, fresh authoritative quest state, and one-call limit.
// The previously crashing AddTaskPermanent API is NEVER referenced.
// Warning: unlike dry-run, this can change one quest's native tracking
// status and potentially affect the saved game. Use a backup/test save.
#include "OneShotOgWriteQA.h"
#include "../f4se_og_api/TaskInterface0623.h"
#include "../../src/QuestTrackingPolicy.h"
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <REL/Relocation.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif
#include <string>
#include <string_view>
#include <charconv>
#include <optional>
#include <utility>

namespace K2040::OneShotResearch {
    namespace {
        using NativeSetter = void(*)(RE::TESQuest*, bool);
        constexpr std::uintptr_t kSetterRva = 0x005D6AC0u;
        constexpr std::uint64_t kSetterId = 1567562u;

        [[nodiscard]] std::wstring ConsentIni() {
            wchar_t module[MAX_PATH]{};
            const auto count=GetModuleFileNameW(nullptr,module,MAX_PATH);
            if (!count || count>=MAX_PATH) return {};
            std::wstring path(module,count);
            auto slash=path.find_last_of(L"\\/");
            if (slash==std::wstring::npos)return {};
            path.resize(slash+1);
            return path+L"Data\\F4SE\\Plugins\\K2040_Quest_Journal_Write_QA.ini";
        }
    }

    [[nodiscard]] bool ReadExplicitWriteTestConsent() {
        const auto path=ConsentIni();
        if (path.empty())return false;
        const auto permitted=[&path](const wchar_t* key) {
            wchar_t value[12]{};
            const auto count=GetPrivateProfileStringW(
                L"Safety",key,L"NO",value,12,path.c_str());
            return count==3 && std::wcscmp(value,L"YES")==0;
        };
        return permitted(L"IHaveBackedUpTestSave") &&
               permitted(L"PermitOneNativeSetActiveCall");
    }

    [[nodiscard]] bool QueueWriteTestOnce(
        const std::shared_ptr<Gate>& gate, const char* raw,
        std::uint64_t saveEpoch, std::uint64_t uiEpoch,
        std::atomic_bool& processAttempted,
        ReadContext readFresh, std::function<void(WriteTestResult)> report) {
        // Re-read the managed INI before accepting any click; changing an
        // INI after launch never silently arms the feature (UI is also gated).
        if (!gate || !raw || !readFresh || !report ||
            processAttempted.load(std::memory_order_acquire) ||
            !ReadExplicitWriteTestConsent())return false;
        std::optional<std::uint32_t> expectedInstance;
#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA
        const auto n=strnlen_s(raw,20);
        if(n!=10 && n!=19)return false;
        if(n==19) {
            if(raw[10]!=':')return false;
            std::uint32_t instance{};
            const auto [end,ec]=std::from_chars(raw+11,raw+19,instance,16);
            if(ec!=std::errc{} || end!=raw+19)return false;
            expectedInstance=instance; // zero is a valid vanilla instance
        }
#else
        const auto n=strnlen_s(raw,11);
        if(n!=10)return false;
#endif
        const auto parsed=K2040::QuestTracking::ParseRequest(
            std::string_view(raw,10));
        if(!parsed)return false;
        const auto* tasks=F4SE::GetTaskInterface();
        if (!tasks || tasks->Version()!=OGF4SE::ABI0623::kTaskInterfaceVersion)
            return false;
        const auto ticket=gate->Offer(parsed->formId,parsed->desiredTracked,
                                      saveEpoch,uiEpoch);
        if (!ticket)return false;
        // The process-wide one-attempt token is consumed BEFORE the
        // asynchronous operation. Even denial/crash cannot auto-retry.
        if (processAttempted.exchange(true,std::memory_order_acq_rel)) {
            (void)gate->Settle(*ticket);
            return false;
        }
        tasks->AddTask(
            [gate,request=*ticket,readFresh=std::move(readFresh),
             report=std::move(report),expectedInstance]() mutable {
                if (!gate->Current(request))return;
                const auto context=readFresh();
                if (!gate->Current(request))return;
                const auto before=ObserveOnTask(request,context);
                auto decision=ReviewOnGameTask(request,before);
#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA
                GenericMiscEvidence miscBefore{};
                if(before.questType==kMisc) {
                    miscBefore=CaptureGenericMiscEvidence(request);
                    decision=ReviewGenericMisc(request,before,expectedInstance,miscBefore);
                } else if(expectedInstance) {
                    decision=Result::kRejected; // old ordinary input cannot masquerade as Misc
                }
#endif
                WriteTestResult outcome=WriteTestResult::kDenied;
                if(decision==Result::kAlreadyCurrent)
                    outcome=WriteTestResult::kAlready;
                else if(decision==Result::kUnverified)
                    outcome=WriteTestResult::kUnverified;
                else if(decision==Result::kWouldWrite) {
                    // Recheck the INI and engine relocation. Never use a
                    // direct flag patch or construct fake quest events.
                    if (ReadExplicitWriteTestConsent() &&
                        gate->Current(request)) {
                        const auto base=reinterpret_cast<std::uintptr_t>(
                            GetModuleHandleW(nullptr));
                        REL::Relocation<NativeSetter> native{REL::ID(kSetterId)};
                        if (base && native.address()==base+kSetterRva) {
                            auto* quest=RE::TESForm::GetFormByID<RE::TESQuest>(
                                request.formId);
                            const auto flags=quest ? static_cast<std::uint16_t>(
                                quest->data.flags) : 0u;
                            bool scopeStillValid=true;
#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA
                            if(before.questType==kMisc){
                                const auto fresh=CaptureGenericMiscEvidence(request);
                                scopeStillValid=quest && expectedInstance &&
                                    quest->currentInstanceID==*expectedInstance &&
                                    fresh.nativeDisplayedCount==miscBefore.nativeDisplayedCount &&
                                    fresh.miscFeedRows==miscBefore.miscFeedRows &&
                                    ReviewGenericMisc(request,before,expectedInstance,fresh)==
                                        Result::kWouldWrite;
                            }
#endif
                            if (quest && scopeStillValid &&
                                quest->GetFormID()==before.resolvedFormId &&
                                flags==before.questFlags &&
                                context.saveEpoch==request.saveEpoch &&
                                context.uiEpoch==request.uiEpoch &&
                                gate->Current(request)) {
                                REX::WARN(
                                    "QuestNativeWriteQA: invoking ONE verified native SetActive; "
                                    "desired={} (no FormID logged)",
                                    request.desiredTracked);
                                native(quest,request.desiredTracked);
                                // Provisional: the authoritatively refreshed
                                // Pip-Boy/compass and save/reload are
                                // game-test responsibilities.
                                outcome=WriteTestResult::kCalledProvisional;
                                const auto after=ObserveOnTask(request,context);
#ifdef K2040_GENERIC_MISC_QUESTWIDE_QA
                                if(before.questType==kMisc){
                                    const auto refresh=CaptureGenericMiscEvidence(request);
                                    REX::WARN("QuestGenericMiscQA: returned; nativeBit={} "
                                              "instanceStable={} feedConsistent={} (PROVISIONAL)",
                                              bool(after.questFlags & kTracked),
                                              expectedInstance && refresh.questInstanceID==*expectedInstance,
                                              refresh.pipboyFeedComplete && refresh.feedAgreesWithQuest);
                                }
#endif
                                REX::WARN(
                                    "QuestNativeWriteQA: RETURNED; bit={} feedValid={} "
                                    "feedTracked={} (NOT gameplay confirmation)",
                                    bool(after.questFlags & kTracked),
                                    after.inNormalPipboyFeed && after.feedHasTrackedBit,
                                    after.feedTracked);
                            }
                        }
                    }
                }
                if(gate->Settle(request))report(outcome);
            });
        return true;
    }
}
