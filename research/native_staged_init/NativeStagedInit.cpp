// Experimental startup isolation for Fallout 4 OG 1.10.163.
// NEVER calls TESQuest::SetActive or touches quest state, saves or UI.
// Default INI stage: passive engine TESQuestEvent subscription only.
// Only single-run AddTask is supported on the installed F4SE 0.6.23.
// Official F4SE v0.6.23 F4SETaskInterface::kInterfaceVersion=2 has
// AddTask and AddUITask ONLY. Pinned CommonLibF4 exposes an unsupported
// third AddTaskPermanent pointer in the same version-2 layout.
// Never access that member; it points beyond the actual object.

#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <F4SE/Version.h>
#include <REL/Relocation.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace {
    using Event = RE::TESQuestEvent::Event;
    using EventSource = RE::BSTEventSource<Event>;
    using EventGetter = EventSource*(*)();
    constexpr std::uintptr_t kSetterRva = 0x005D6AC0u;
    constexpr std::uintptr_t kGetterRva = 0x005E2FD0u;
    constexpr std::uintptr_t kDispatcherRva = 0x005F5110u;
    constexpr std::uint64_t kGetterId = 476335u;
    constexpr std::uint32_t kMaxEventLog = 8u;

    static_assert(sizeof(Event) == 16);
    static_assert(offsetof(RE::TESQuest, data) == 0xF0);
    static_assert(offsetof(RE::QUEST_DATA, flags) == 0x4);

    struct Mode {
        bool eventSink{};
        bool taskOnce{};
        bool taskPermanent{};
    };

    Mode selected{};
    std::atomic_bool dataReady{false};
    std::atomic_bool taskQueued{false};
    std::atomic_uint32_t eventCount{0};

    [[nodiscard]] bool VerifyMappedOg() noexcept {
        const auto module = GetModuleHandleW(nullptr);
        if (!module) return false;
        const auto base = reinterpret_cast<std::uintptr_t>(module);
        const auto at = base + kSetterRva;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<const void*>(at), &mbi, sizeof(mbi)) ||
            mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
            return false;
        const auto prot = mbi.Protect & 0xFFu;
        if (prot != PAGE_EXECUTE && prot != PAGE_EXECUTE_READ &&
            prot != PAGE_EXECUTE_READWRITE && prot != PAGE_EXECUTE_WRITECOPY)
            return false;
        const auto end = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (at >= end || end - at < 0x45u) return false;
        const auto* code = reinterpret_cast<const unsigned char*>(at);
        constexpr std::array<unsigned char, 8> prefix{
            0x48, 0x83, 0xEC, 0x38, 0x84, 0xD2, 0x74, 0x0E
        };
        if (std::memcmp(code, prefix.data(), prefix.size()) != 0) return false;
        const auto decodeCall = [code](std::size_t offset) -> std::int64_t {
            if (code[offset] != 0xE8u) return -1;
            std::int32_t displacement{};
            std::memcpy(&displacement, code + offset + 1, sizeof(displacement));
            return static_cast<std::int64_t>(kSetterRva)
                 + static_cast<std::int64_t>(offset) + 5 + displacement;
        };
        return decodeCall(0x2Fu) == static_cast<std::int64_t>(kGetterRva) &&
               decodeCall(0x3Cu) == static_cast<std::int64_t>(kDispatcherRva);
    }

    [[nodiscard]] std::wstring SettingsPath() {
        wchar_t exe[MAX_PATH]{};
        const auto len = GetModuleFileNameW(nullptr, exe, MAX_PATH);
        if (!len || len >= MAX_PATH) return {};
        std::wstring path(exe, len);
        const auto last = path.find_last_of(L"\\/");
        if (last == std::wstring::npos) return {};
        path.resize(last + 1);
        path += L"Data\\F4SE\\Plugins\\K2040_Quest_Staged_Init.ini";
        return path;
    }

    [[nodiscard]] Mode ReadModes() {
        const auto file = SettingsPath();
        if (file.empty()) return {};
        const auto enabled = [&file](const wchar_t* key) {
            wchar_t value[12]{};
            const auto n = GetPrivateProfileStringW(
                L"Diagnostics", key, L"NO", value, 12, file.c_str());
            return n == 3 && std::wcscmp(value, L"YES") == 0;
        };
        return {
            enabled(L"EnableEventSink"),
            enabled(L"EnableOneShotTask"),
            enabled(L"EnablePermanentTask")
        };
    }

    class PassiveSink final : public RE::BSTEventSink<Event> {
    public:
        RE::BSEventNotifyControl ProcessEvent(const Event& event, EventSource*) override {
            // NO quest object dereference, retain, queues or writes.
            const auto n = eventCount.fetch_add(1, std::memory_order_relaxed);
            if (n < kMaxEventLog)
                REX::INFO("StagedInit: EV observed type={} count={}",
                    static_cast<std::int32_t>(event.changeType), n + 1);
            else if (n == kMaxEventLog)
                REX::INFO("StagedInit: event log capped, listener remains passive");
            return RE::BSEventNotifyControl::kContinue;
        }
    };
    PassiveSink passiveSink;

    void OnMessage(F4SE::MessagingInterface::Message* msg) {
        if (!msg) return;
        using M = F4SE::MessagingInterface;
        if (msg->type == M::kGameDataReady) {
            if (dataReady.exchange(true)) return;
            REX::INFO("StagedInit: GD1 game-data-ready");
            selected = ReadModes();
            REX::INFO("StagedInit: GD2 config event={} once={} permanent={}",
                selected.eventSink, selected.taskOnce, selected.taskPermanent);
            // This API is ABSENT from F4SE 0.6.23. Refuse the setting
            // even when only the permanent mode was requested.
            if (selected.taskPermanent) {
                REX::ERROR("StagedInit: GD3 UNSUPPORTED OG F4SE 0.6.23 permanent task; no actions");
                selected = {};
                return;
            }
            const auto modes = int(selected.eventSink) + int(selected.taskOnce);
            if (modes > 1) {
                REX::WARN("StagedInit: GD3 INVALID multiple modes, no actions");
                selected = {};
                return;
            }
            if (!modes) {
                REX::INFO("StagedInit: GD3 all stages disabled; no actions");
                return;
            }
            if (!VerifyMappedOg()) {
                REX::ERROR("StagedInit: GD3 loaded OG check FAILED; no actions");
                selected = {};
                return;
            }
            REX::INFO("StagedInit: GD3 OG instruction guard passed");
            if (selected.eventSink) {
                REX::INFO("StagedInit: EV1 resolving event getter; no write methods");
                REL::Relocation<EventGetter> getter{REL::ID(kGetterId)};
                auto* module = GetModuleHandleW(nullptr);
                if (getter.address() != reinterpret_cast<std::uintptr_t>(module) + kGetterRva) {
                    REX::ERROR("StagedInit: EV2 getter relocation mismatch; no listener");
                    return;
                }
                REX::INFO("StagedInit: EV2 invoking native event source GETTER (no quest call)");
                auto* source = getter();
                if (!source) {
                    REX::ERROR("StagedInit: EV3 no event source; no listener");
                    return;
                }
                REX::INFO("StagedInit: EV3 registering passive event sink");
                source->RegisterSink(&passiveSink);
                REX::INFO("StagedInit: EV4 passive event sink registered successfully");
            } else {
                REX::INFO("StagedInit: GD4 task test selected; deferring to post-load");
            }
        } else if (msg->type == M::kPostLoadGame) {
            REX::INFO("StagedInit: PL1 PostLoadGame received");
            if (!dataReady.load() || taskQueued.exchange(true)) return;
            if (!selected.taskOnce) return;
            REX::INFO("StagedInit: PL2 reading F4SE task interface");
            const auto* tasks = F4SE::GetTaskInterface();
            if (!tasks || tasks->Version() < F4SE::TaskInterface::kVersion) {
                REX::ERROR("StagedInit: PL3 task interface unavailable; no scheduling");
                return;
            }
            REX::INFO("StagedInit: OT1 submitting ONE read-free callback");
            tasks->AddTask([] {
                REX::INFO("StagedInit: OT2 one-shot callback executed, no game access");
            });
            REX::INFO("StagedInit: OT3 one-shot callback submitted");
        }
    }
}

F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(
    const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    if (!f4se || !info || f4se->IsEditor() ||
        f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "K2040_Quest_Staged_Init";
    info->version = 1;
    return true;
}
F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* load) {
    if (!load || load->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    F4SE::Init(load, F4SE::InitInfo{
        .log = true, .logName = "K2040_Quest_Staged_Init", .hook = false
    });
    REX::INFO("StagedInit: L0 plugin loaded, mode-specific operations NOT started");
    const auto* messages = F4SE::GetMessagingInterface();
    return messages && messages->RegisterListener(OnMessage);
}
