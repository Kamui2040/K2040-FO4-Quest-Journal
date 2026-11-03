// DO NOT BUILD: forensic archive of the original crashing test.
// Official F4SE v0.6.23 exports no AddTaskPermanent function in its
// version-2 TaskInterface. CommonLibF4 pinned header incorrectly
// declares a third slot, causing an invalid function-pointer call.
// Keep this historical source but make all accidental compilation fail.
#error K2040_NATIVE_WRITER_QUARANTINED_OG_F4SE_0_6_23_TASK_ABI_MISMATCH
// NON-SHIPPING, OPT-IN F4SE diagnostic for Fallout 4 OG 1.10.163 ONLY.
// This program CAN perform exactly one REAL engine-managed SetActive call.
// Disabled at installation: requires two explicit INI consents PLUS
// Ctrl+Alt+Shift+F11 held for 2 seconds following a fresh vanilla
// Pip-Boy quest toggle. Never install without a backed-up test save.
// NO hooks, injected script, quest start/completion, or direct flag writes.
#include <RE/Fallout.h>
#include <RE/P/PipboyDataManager.h>
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
#include <mutex>
#include <string>
#include "TestPolicy.h"

namespace {
    using Event = RE::TESQuestEvent::Event;
    using Source = RE::BSTEventSource<Event>;
    using SourceGetter = Source*(*)();
    using NativeSetActive = void(*)(RE::TESQuest*, bool);
    constexpr std::uintptr_t kSetRva = 0x005D6AC0u;
    constexpr std::uintptr_t kEventSourceRva = 0x005E2FD0u;
    constexpr std::uintptr_t kEventDispatchRva = 0x005F5110u;
    constexpr std::uint64_t kSourceId = 476335u;
    constexpr std::uint64_t kSetterId = 1567562u;
    constexpr std::uint64_t kMaxCandidateAgeMs = 45000;
    constexpr std::uint64_t kHoldDurationMs = 2000;
    static_assert(offsetof(RE::TESQuest, data) == 0xF0);
    static_assert(offsetof(RE::QUEST_DATA, flags) == 4);
    static_assert(sizeof(Event) == 16);

    struct Candidate {
        std::uint32_t formId{};
        std::uint64_t epoch{};
        std::uint64_t timeMs{};
        DWORD vanillaThread{};
        bool tracked{};
    };

    std::mutex candidateMutex;
    Candidate candidate;
    std::atomic_uint64_t saveEpoch{1};
    std::atomic_bool gameLoaded{false};
    std::atomic_bool armed{false};
    std::atomic_bool attempted{false};
    std::atomic_uint32_t nativeEventCount{0};
    std::atomic_uint32_t expectedFormId{0};
    std::atomic_uint64_t holdSinceMs{0};
    std::atomic_bool previousPressed{false};

    [[nodiscard]] bool MatchesOg() noexcept {
        auto* module = GetModuleHandleW(nullptr);
        if (!module) return false;
        const auto base = reinterpret_cast<std::uintptr_t>(module);
        const auto addr = base + kSetRva;
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(reinterpret_cast<const void*>(addr), &memory, sizeof(memory)) ||
            memory.State != MEM_COMMIT ||
            (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const auto prot = memory.Protect & 0xFFu;
        if (prot != PAGE_EXECUTE && prot != PAGE_EXECUTE_READ &&
            prot != PAGE_EXECUTE_READWRITE && prot != PAGE_EXECUTE_WRITECOPY)
            return false;
        const auto end = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
        if (end <= addr || end - addr < 0x45u) return false;
        const auto* code = reinterpret_cast<const unsigned char*>(addr);
        constexpr std::array<unsigned char, 8> prefix{
            0x48, 0x83, 0xEC, 0x38, 0x84, 0xD2, 0x74, 0x0E
        };
        if (std::memcmp(code, prefix.data(), prefix.size()) != 0) return false;
        const auto callTarget = [code](std::size_t off) -> std::int64_t {
            if (code[off] != 0xE8) return -1;
            std::int32_t rel{};
            std::memcpy(&rel, code + off + 1, sizeof(rel));
            return static_cast<std::int64_t>(kSetRva) +
                   static_cast<std::int64_t>(off) + 5 + rel;
        };
        return callTarget(0x2F) == static_cast<std::int64_t>(kEventSourceRva) &&
               callTarget(0x3C) == static_cast<std::int64_t>(kEventDispatchRva);
    }

    [[nodiscard]] std::wstring IniPath() {
        wchar_t exe[MAX_PATH]{};
        const auto count = GetModuleFileNameW(nullptr, exe, MAX_PATH);
        if (!count || count >= MAX_PATH) return {};
        std::wstring path(exe, count);
        const auto slash = path.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return {};
        path.resize(slash + 1);
        return path + L"Data\\F4SE\\Plugins\\K2040_Quest_Native_Write_Test.ini";
    }

    [[nodiscard]] bool ExplicitlyArmed() {
        const auto file = IniPath();
        if (file.empty()) return false;
        const auto exact = [&file](const wchar_t* key) {
            wchar_t buffer[32]{};
            const auto n = GetPrivateProfileStringW(
                L"Safety", key, L"NO", buffer, 32, file.c_str());
            return n == 3 && std::wcscmp(buffer, L"YES") == 0;
        };
        return exact(L"IHaveBackedUpSave") && exact(L"PermitOneNativeTrackingWrite");
    }

    struct Feed {
        bool available{};
        bool found{};
        bool tracked{};
    };

    [[nodiscard]] Feed ReadMainQuestFeed(std::uint32_t id) {
        auto* manager = RE::PipboyDataManager::GetSingleton();
        if (!manager || !manager->questData.questArray) return {};
        Feed result{true, false, false};
        std::uint32_t seen = 0;
        for (auto* value : manager->questData.questArray->elements) {
            if (++seen > 1024) break;
            if (!value || value->GetType() != RE::PipboyValue::kObject)
                continue;
            auto* entry = static_cast<RE::PipboyObject*>(value);
            const auto field = [&entry](const char* key) {
                return entry->GetMember<RE::PipboyValue*>(RE::BSFixedString(key));
            };
            auto* form = field("formID");
            if (!form || form->GetType() != RE::PipboyValue::kUint32) continue;
            const auto formId = static_cast<std::uint32_t>(
                *static_cast<RE::PipboyPrimitiveValue<std::uint32_t>*>(form));
            if (formId != id) continue;
            auto* active = field("active");
            if (!active || active->GetType() != RE::PipboyValue::kBool)
                return result;  // Missing authoritative flag: fail closed.
            result.found = true;
            result.tracked = static_cast<bool>(
                *static_cast<RE::PipboyPrimitiveValue<bool>*>(active));
            return result;
        }
        return result;
    }

    [[nodiscard]] bool ForegroundGame() {
        auto* window = GetForegroundWindow();
        DWORD process = 0;
        if (window) GetWindowThreadProcessId(window, &process);
        return process == GetCurrentProcessId();
    }

    [[nodiscard]] bool MenusClear() {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return false;
        for (const char* name : {"PipboyMenu", "PauseMenu", "LoadingMenu",
                                 "MainMenu", "Console", "FavoritesMenu"}) {
            if (ui->GetMenuOpen(RE::BSFixedString(name))) return false;
        }
        return true;
    }

    class Observer final : public RE::BSTEventSink<Event> {
    public:
        RE::BSEventNotifyControl ProcessEvent(const Event& event, Source*) override {
            if (event.changeType != RE::TESQuestEvent::kUpdateQuestActiveStatus ||
                !event.quest || !gameLoaded.load(std::memory_order_acquire))
                return RE::BSEventNotifyControl::kContinue;
            const auto id = event.quest->GetFormID();
            if (id == expectedFormId.load(std::memory_order_acquire))
                nativeEventCount.fetch_add(1, std::memory_order_relaxed);
            if (!attempted.load(std::memory_order_acquire)) {
                std::lock_guard lock(candidateMutex);
                candidate = {
                    id,
                    saveEpoch.load(std::memory_order_relaxed),
                    GetTickCount64(),
                    GetCurrentThreadId(),
                    (static_cast<std::uint16_t>(event.quest->data.flags) &
                     K2040::NativeWriteTest::kTracked) != 0
                };
                REX::INFO("QuestWriteTest: recent game tracking event captured (no quest text or save data)");
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };
    Observer observer;

    void TryOnce() {
        // Irreversible in this process, including for denied requests.
        if (attempted.exchange(true, std::memory_order_acq_rel)) return;
        Candidate prior{};
        { std::lock_guard lock(candidateMutex); prior = candidate; }
        const auto session = saveEpoch.load(std::memory_order_acquire);
        const auto now = GetTickCount64();
        auto* quest = prior.formId ? RE::TESForm::GetFormByID<RE::TESQuest>(prior.formId) : nullptr;
        const auto feed = prior.formId ? ReadMainQuestFeed(prior.formId) : Feed{};
        const auto flags = quest ? static_cast<std::uint16_t>(quest->data.flags) : 0u;

        K2040::NativeWriteTest::Snapshot validation{};
        validation.armed = armed.load();
        validation.alreadyUsed = false;  // The atomic exchange above owns this one attempt.
        validation.gameLoaded = gameLoaded.load();
        validation.validOgRuntime = true;  // Both F4SE Query + Load verified OG 1.10.163.
        validation.matchingEngine = MatchesOg();
        validation.deliberateChord = true;  // Only called after verified two-second chord.
        validation.gameWindowForeground = ForegroundGame();
        validation.gameTaskMatchesVanillaThread =
            prior.vanillaThread && prior.vanillaThread == GetCurrentThreadId();
        validation.pipboyClosed = MenusClear();
        validation.currentlyVisibleInPipboy = feed.found;
        validation.authoritativeFeedAvailable = feed.available;
        validation.authoritativeFeedTracked = feed.tracked;
        validation.candidateFormId = prior.formId;
        validation.resolvedFormId = quest ? quest->GetFormID() : 0;
        validation.sessionEpoch = session;
        validation.candidateEpoch = prior.epoch;
        validation.candidateAgeMs = now >= prior.timeMs ? now - prior.timeMs : UINT64_MAX;
        validation.questFlags = flags;
        validation.questType = quest ? quest->data.questType : 0;
        if (!prior.formId || prior.timeMs == 0 ||
            validation.candidateAgeMs > kMaxCandidateAgeMs ||
            (quest && prior.tracked != ((flags & K2040::NativeWriteTest::kTracked) != 0)) ||
            K2040::NativeWriteTest::Evaluate(validation) !=
                K2040::NativeWriteTest::Decision::kAllowExactlyOnce) {
            REX::WARN(
                "QuestWriteTest: preflight DENIED, NO CALL; epoch={} fresh={} sameThread={} "
                "focus={} menusClear={} feedReady={} visible={} feedMatchesNative={} eligibleFlags={} "
                "sameObservedState={} codeMatch={}",
                prior.epoch == session,
                validation.candidateAgeMs <= kMaxCandidateAgeMs && prior.timeMs != 0,
                validation.gameTaskMatchesVanillaThread,
                validation.gameWindowForeground,
                validation.pipboyClosed,
                validation.authoritativeFeedAvailable,
                validation.currentlyVisibleInPipboy,
                validation.authoritativeFeedTracked ==
                    ((validation.questFlags & K2040::NativeWriteTest::kTracked) != 0),
                (validation.questFlags & K2040::NativeWriteTest::kEnabled) != 0 &&
                    !(validation.questFlags & (K2040::NativeWriteTest::kCompleted |
                                              K2040::NativeWriteTest::kFailed)),
                quest && prior.tracked ==
                    ((validation.questFlags & K2040::NativeWriteTest::kTracked) != 0),
                validation.matchingEngine);
            return;
        }
        REL::Relocation<NativeSetActive> setter{REL::ID(kSetterId)};
        if (setter.address() != reinterpret_cast<std::uintptr_t>(
                                    GetModuleHandleW(nullptr)) + kSetRva) {
            REX::ERROR("QuestWriteTest: relocation conflict; NOT CALLED");
            return;
        }
        const auto desired = !prior.tracked;
        expectedFormId.store(prior.formId, std::memory_order_release);
        REX::WARN("QuestWriteTest: invoking ONE native TESQuest::SetActive call");
        setter(quest, desired);
        const auto after = static_cast<std::uint16_t>(quest->data.flags);
        const auto updatedFeed = ReadMainQuestFeed(prior.formId);
        REX::INFO(
            "QuestWriteTest: RETURNED desired={} bit={} feedPresent={} feedTracked={} observedMatchingEvents={}",
            desired, (after & K2040::NativeWriteTest::kTracked) != 0,
            updatedFeed.found, updatedFeed.tracked, nativeEventCount.load());
        // Result is provisional. The user must verify compass, Pip-Boy,
        // game stability and save/reload. Never retry automatically.
    }

    void FrameTick() {
        if (!armed.load(std::memory_order_relaxed) ||
            attempted.load(std::memory_order_relaxed) ||
            !gameLoaded.load(std::memory_order_relaxed)) return;
        const bool pressed =
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_MENU) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
            (GetAsyncKeyState(VK_F11) & 0x8000) &&
            ForegroundGame() && MenusClear();
        if (!pressed) {
            previousPressed.store(false, std::memory_order_relaxed);
            holdSinceMs.store(0, std::memory_order_relaxed);
            return;
        }
        const auto now = GetTickCount64();
        if (!previousPressed.exchange(true, std::memory_order_relaxed)) {
            holdSinceMs.store(now, std::memory_order_relaxed);
            return;
        }
        const auto since = holdSinceMs.load(std::memory_order_relaxed);
        if (since && now >= since && now - since >= kHoldDurationMs)
            TryOnce();
    }

    void F4SEAPI HandleMessage(F4SE::MessagingInterface::Message* msg) {
        if (!msg) return;
        using M = F4SE::MessagingInterface;
        if (msg->type == M::kPreLoadGame) {
            gameLoaded.store(false);
            saveEpoch.fetch_add(1);
            { std::lock_guard lock(candidateMutex); candidate = {}; }
        } else if (msg->type == M::kPostLoadGame) {
            saveEpoch.fetch_add(1);
            gameLoaded.store(true);
            REX::INFO("QuestWriteTest: load boundary, no pending candidate");
        } else if (msg->type == M::kNewGame) {
            saveEpoch.fetch_add(1);
            gameLoaded.store(false); // deny writer on new-game sessions
        } else if (msg->type == M::kGameDataReady) {
            // No source getter, no permanent task, no engine changes
            // whatsoever unless BOTH explicit INI consents were set.
            if (!ExplicitlyArmed()) {
                REX::INFO("QuestWriteTest: DISARMED by default; no callbacks installed");
                return;
            }
            if (!MatchesOg()) {
                REX::ERROR("QuestWriteTest: executable mismatch, refusing arming");
                return;
            }
            REL::Relocation<SourceGetter> getter{REL::ID(kSourceId)};
            if (getter.address() != reinterpret_cast<std::uintptr_t>(
                                        GetModuleHandleW(nullptr)) + kEventSourceRva) {
                REX::ERROR("QuestWriteTest: event getter relocation mismatch");
                return;
            }
            auto* tasks = F4SE::GetTaskInterface();
            if (!tasks || tasks->Version() < F4SE::TaskInterface::kVersion) {
                REX::ERROR("QuestWriteTest: F4SE game task interface unavailable");
                return;
            }
            auto* source = getter();
            if (!source) {
                REX::ERROR("QuestWriteTest: no quest event source");
                return;
            }
            source->RegisterSink(&observer);
            armed.store(true, std::memory_order_release);
            tasks->AddTaskPermanent(FrameTick);
            REX::WARN("QuestWriteTest: ARMED; hold Ctrl+Alt+Shift+F11 for 2 seconds after VANILLA Pip-Boy toggle");
        }
    }
}
F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(
    const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    if (!f4se || !info || f4se->IsEditor() ||
        f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "K2040_Quest_Native_Write_Test";
    info->version = 1;
    return true;
}
F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* load) {
    if (!load || load->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    F4SE::Init(load, F4SE::InitInfo{
        .log = true,
        .logName = "K2040_Quest_Native_Write_Test",
        .hook = false
    });
    auto* messages = F4SE::GetMessagingInterface();
    return messages && messages->RegisterListener(HandleMessage);
}
