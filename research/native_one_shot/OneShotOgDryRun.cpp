// Source-only research bridge, optionally shared by separate write QA build.
// It itself never calls SetActive, even when linked to the write QA.
// requestPipboyTracking UI action. NO F4SE plugin entrypoints,
// no automatic startup, no menus, no hooks and NO quest writes.
// It deliberately uses only the official F4SE 0.6.23 AddTask slot.
// Native SetActive is NOT resolved or called by this experiment.

#include "OneShotOgDryRun.h"
#include "../f4se_og_api/TaskInterface0623.h"
#include "../../src/QuestTrackingPolicy.h"
#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
#include <F4SE/F4SE.h>
#include <F4SE/Version.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

namespace K2040::OneShotResearch {
    static_assert(K2040::OGF4SE::ABI0623::kSlotCount == 2);
    static_assert(F4SE::TaskInterface::kVersion == 2);
    static_assert(offsetof(RE::TESQuest, data) == 0xF0);
    static_assert(offsetof(RE::QUEST_DATA, flags) == 4);

    // Called by the existing Journal owner's *game task*, never by
    // a browser or DOM-ready callback on an arbitrary worker thread.
    // We do NOT infer the source's current flag by looking at a stale
    // snapshot, and we never enumerate unrevealed or Misc quest entries.
    [[nodiscard]] Observation ObserveOnTask(const Intent& intent,
                                            const LiveJournalContext& context) {
        Observation o{};
        o.selectedFormId = context.selectedId;
        o.saveEpoch = context.saveEpoch;
        o.uiEpoch = context.uiEpoch;
        o.loaded = context.gameLoaded;
        o.journalOpenFocused = context.journalFocused;
        o.previouslyObserved = context.previouslyObserved;
        o.onF4seTask = true; // this function is private to AddTask's callback
        // Caller’s F4SEPlugin_Query and Load must reject all but OG.
        // Additionally inspect the ACTUAL mapped code, like the successful
        // event observer, to deny any unknown binary patch layout.
        const auto* module = GetModuleHandleW(nullptr);
        if (!module || !context.runtimeOg163Verified) return o;
        const auto addr = reinterpret_cast<std::uintptr_t>(module) + 0x005D6AC0u;
        MEMORY_BASIC_INFORMATION page{};
        if (!VirtualQuery(reinterpret_cast<void*>(addr), &page, sizeof(page)) ||
            page.State != MEM_COMMIT ||
            (page.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return o;
        const auto protection = page.Protect & 0xFFu;
        if (protection != PAGE_EXECUTE && protection != PAGE_EXECUTE_READ &&
            protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY)
            return o;
        const auto end = reinterpret_cast<std::uintptr_t>(page.BaseAddress) + page.RegionSize;
        if (addr >= end || end - addr < 0x45u) return o;
        constexpr std::array<unsigned char, 8> prefix{
            0x48,0x83,0xEC,0x38,0x84,0xD2,0x74,0x0E
        };
        const auto* code = reinterpret_cast<const unsigned char*>(addr);
        if (std::memcmp(code, prefix.data(), prefix.size()) != 0) return o;
        const auto callDestination = [code](std::size_t offset) -> std::int64_t {
            if (code[offset] != 0xE8u) return -1;
            std::int32_t displacement{};
            std::memcpy(&displacement, code + offset + 1, sizeof(displacement));
            return static_cast<std::int64_t>(0x005D6AC0u) +
                   static_cast<std::int64_t>(offset) + 5 + displacement;
        };
        if (callDestination(0x2Fu) != 0x005E2FD0ll ||
            callDestination(0x3Cu) != 0x005F5110ll) return o;
        o.og163 = true;

        if (!o.loaded || !o.journalOpenFocused ||
            !o.previouslyObserved || !intent.formId || !o.selectedFormId ||
            intent.saveEpoch != o.saveEpoch || intent.uiEpoch != o.uiEpoch)
            return o;

        auto* quest = RE::TESForm::GetFormByID<RE::TESQuest>(intent.formId);
        if (!quest) return o;
        o.resolvedFormId = quest->GetFormID();
        o.questFlags = static_cast<std::uint16_t>(quest->data.flags);
        o.questType = static_cast<std::uint8_t>(quest->data.questType);

        auto* manager = RE::PipboyDataManager::GetSingleton();
        if (!manager || !manager->questData.questArray) return o;
        std::uint32_t traversed=0;
        for (auto* value : manager->questData.questArray->elements) {
            if (++traversed > 1024u) break;
            if (!value || value->GetType()!=RE::PipboyValue::kObject) continue;
            auto* entry = static_cast<RE::PipboyObject*>(value);
            auto* form = entry->GetMember<RE::PipboyValue*>(RE::BSFixedString("formID"));
            if (!form || form->GetType()!=RE::PipboyValue::kUint32) continue;
            const auto formId = static_cast<std::uint32_t>(
                *static_cast<RE::PipboyPrimitiveValue<std::uint32_t>*>(form));
            if (formId != intent.formId) continue;
            o.inNormalPipboyFeed=true;
            auto* tracked = entry->GetMember<RE::PipboyValue*>(RE::BSFixedString("active"));
            if (tracked && tracked->GetType()==RE::PipboyValue::kBool) {
                o.feedHasTrackedBit=true;
                o.feedTracked=static_cast<bool>(
                    *static_cast<RE::PipboyPrimitiveValue<bool>*>(tracked));
            }
            return o;
        }
        return o;
    }

    // RESEARCH-ONLY entrypoint intended for explicit user-initiated
    // Prisma click events. Strings never cross the callback boundary:
    // only by-value primitive IDs, generations and owned function objects.
    //
    // The owner MUST re-create its Gate on panel/session transition,
    // supply a fresh ReadContext on the task thread, and only surface
    // "would write" as a DRY-RUN result, not actual tracking success.
    [[nodiscard]] bool QueueDryRun(
        const std::shared_ptr<Gate>& gate,
        std::uint32_t selectedId,
        bool desired,
        std::uint64_t saveEpoch,
        std::uint64_t uiEpoch,
        ReadContext readFresh,
        Report report) {
        if (!gate || !readFresh || !report || !selectedId) return false;
        const auto* tasks = F4SE::GetTaskInterface();
        if (!tasks || tasks->Version() != K2040::OGF4SE::ABI0623::kTaskInterfaceVersion)
            return false;
        return QueueOneShot(gate, selectedId, desired, saveEpoch, uiEpoch,
            [tasks](std::function<void()> oneTask) -> bool {
                tasks->AddTask(std::move(oneTask));
                return true; // AddTask is void, ownership is transferred.
            },
            [readFresh=std::move(readFresh)](const Intent& request) {
                return ObserveOnTask(request,readFresh());
            },
            std::move(report));
    }

    // The Prisma callback's raw char* is a borrowed buffer. Validate its
    // bounded length and copy ONLY the parsed primitive values before
    // returning, just like the proven QueueUIString production bridge.
    [[nodiscard]] bool QueueFromExplicitPrismaClick(
        const std::shared_ptr<Gate>& gate, const char* borrowedUiPayload,
        std::uint64_t saveEpoch, std::uint64_t uiEpoch,
        ReadContext readFresh, Report report) {
        if (!borrowedUiPayload) return false;
        const auto length = strnlen_s(borrowedUiPayload, 11);
        if (length != 10) return false;
        const auto request=K2040::QuestTracking::ParseRequest(
            std::string_view(borrowedUiPayload, length));
        if (!request) return false;
        return QueueDryRun(gate, request->formId, request->desiredTracked,
                           saveEpoch, uiEpoch, std::move(readFresh),
                           std::move(report));
    }
}
