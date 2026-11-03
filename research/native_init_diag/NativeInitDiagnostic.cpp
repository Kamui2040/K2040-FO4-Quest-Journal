// Fallout 4 OG 1.10.163 native writer STARTUP TRIAGE ONLY.
// No quest calls, no event subscription, no F4SE scheduled tasks.
// Verifies initialization steps and logs BEFORE every potentially
// fallible engine/Windows read operation. NOT a tracking writer.
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <F4SE/Version.h>
#include <REL/Relocation.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR
#endif
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

namespace {
    constexpr std::uintptr_t kSetRva = 0x005D6AC0u;
    constexpr std::uintptr_t kEventSourceRva = 0x005E2FD0u;
    constexpr std::uintptr_t kEventDispatchRva = 0x005F5110u;
    using Getter = RE::BSTEventSource<RE::TESQuestEvent::Event>* (*)();
    using Setter = void (*)(RE::TESQuest*, bool);
    static_assert(sizeof(RE::TESQuestEvent::Event) == 0x10);
    static_assert(offsetof(RE::TESQuest, data) == 0xF0);
    static_assert(offsetof(RE::QUEST_DATA, flags) == 0x4);

    [[nodiscard]] bool MatchesOgLoadedCode() noexcept {
        const auto module = GetModuleHandleW(nullptr);
        if (!module) return false;
        const auto at = reinterpret_cast<std::uintptr_t>(module) + kSetRva;
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(reinterpret_cast<const void*>(at), &region, sizeof(region)) ||
            region.State != MEM_COMMIT ||
            (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const auto prot = region.Protect & 0xFFu;
        if (prot != PAGE_EXECUTE && prot != PAGE_EXECUTE_READ &&
            prot != PAGE_EXECUTE_READWRITE && prot != PAGE_EXECUTE_WRITECOPY) return false;
        const auto end = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
        if (end <= at || end - at < 0x45u) return false;
        const auto* code = reinterpret_cast<const unsigned char*>(at);
        constexpr std::array<unsigned char, 8> prefix{
            0x48, 0x83, 0xEC, 0x38, 0x84, 0xD2, 0x74, 0x0E
        };
        if (std::memcmp(code, prefix.data(), prefix.size()) != 0) return false;
        const auto callTarget = [code](std::size_t offset) -> std::int64_t {
            if (code[offset] != 0xE8u) return -1;
            std::int32_t relative{};
            std::memcpy(&relative, code + offset + 1, sizeof(relative));
            return static_cast<std::int64_t>(kSetRva) +
                static_cast<std::int64_t>(offset) + 5 + relative;
        };
        return callTarget(0x2Fu) == static_cast<std::int64_t>(kEventSourceRva) &&
               callTarget(0x3Cu) == static_cast<std::int64_t>(kEventDispatchRva);
    }

    void F4SEAPI OnMessage(F4SE::MessagingInterface::Message* msg) {
        if (!msg) return;
        using M = F4SE::MessagingInterface;
        if (msg->type == M::kGameDataReady) {
            REX::INFO("NativeInitDiag: checkpoint=01 GameDataReady reached");
            REX::INFO("NativeInitDiag: checkpoint=02 reading module pathname");
            wchar_t exe[MAX_PATH]{};
            const auto count = GetModuleFileNameW(nullptr, exe, MAX_PATH);
            if (!count || count >= MAX_PATH) {
                REX::WARN("NativeInitDiag: pathname unavailable; abort");
                return;
            }
            std::wstring path(exe, count);
            const auto index = path.find_last_of(L"\\/");
            if (index == std::wstring::npos) {
                REX::WARN("NativeInitDiag: invalid pathname; abort");
                return;
            }
            path.resize(index + 1);
            path += L"Data\\F4SE\\Plugins\\K2040_Quest_Native_Write_Test.ini";
            REX::INFO("NativeInitDiag: checkpoint=03 reading INI values (read-only)");
            wchar_t value[32]{};
            const auto a = GetPrivateProfileStringW(L"Safety", L"IHaveBackedUpSave",
                L"NO", value, 32, path.c_str());
            const bool first = a == 3 && std::wcscmp(value, L"YES") == 0;
            const auto b = GetPrivateProfileStringW(L"Safety", L"PermitOneNativeTrackingWrite",
                L"NO", value, 32, path.c_str());
            const bool second = b == 3 && std::wcscmp(value, L"YES") == 0;
            REX::INFO("NativeInitDiag: checkpoint=04 INI parsed; consent1={} consent2={}", first, second);
            REX::INFO("NativeInitDiag: checkpoint=05 checking loaded code");
            const bool valid = MatchesOgLoadedCode();
            REX::INFO("NativeInitDiag: checkpoint=06 loaded-code match={}", valid);
            if (!valid) return;
            REX::INFO("NativeInitDiag: checkpoint=07 reading task interface pointer");
            const auto* tasks = F4SE::GetTaskInterface();
            if (!tasks) {
                REX::WARN("NativeInitDiag: no task interface (no tasks queued)");
                return;
            }
            REX::INFO("NativeInitDiag: checkpoint=08 task interface version={}", tasks->Version());
            REX::INFO("NativeInitDiag: checkpoint=09 resolving relocations without invoking");
            REL::Relocation<Getter> sourceGetter{REL::ID(476335)};
            REL::Relocation<Setter> setActive{REL::ID(1567562)};
            const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
            REX::INFO("NativeInitDiag: checkpoint=10 getter={} setter={} (NEITHER CALLED)",
                sourceGetter.address() == base + kEventSourceRva,
                setActive.address() == base + kSetRva);
            REX::INFO("NativeInitDiag: ALL STARTUP READ CHECKS COMPLETE; no events/hooks/tasks/writes");
        } else if (msg->type == M::kPostLoadGame) {
            REX::INFO("NativeInitDiag: PostLoadGame reached; no actions");
        } else if (msg->type == M::kNewGame) {
            REX::INFO("NativeInitDiag: NewGame reached; no actions");
        }
    }
}

F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(
    const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    if (!f4se || !info || f4se->IsEditor() ||
        f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "K2040_Quest_Native_Init_Diag";
    info->version = 1;
    return true;
}
F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* load) {
    if (!load || load->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    F4SE::Init(load, F4SE::InitInfo{
        .log = true,
        .logName = "K2040_Quest_Native_Init_Diag",
        .hook = false
    });
    REX::INFO("NativeInitDiag: checkpoint=00 plugin loaded, registering only F4SE message callback");
    const auto* messages = F4SE::GetMessagingInterface();
    return messages && messages->RegisterListener(OnMessage);
}
