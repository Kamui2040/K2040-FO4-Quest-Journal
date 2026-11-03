// Isolated, non-shipping FO4 OG 1.10.163 event observer.
// Subscribes to the engine event registry (a registration change), but
// NEVER changes quest flags/stages/saves, invokes TESQuest::SetActive,
// publishes synthetic events, installs hooks or starts threads.
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <F4SE/Version.h>
#include <REL/Relocation.h>
#include <Windows.h>
#ifdef ERROR
#undef ERROR  // Windows GDI macro conflicts with REX::ERROR logging.
#endif
#include <atomic>
#include <array>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace {
    using Event = RE::TESQuestEvent::Event;
    using EventSource = RE::BSTEventSource<Event>;
    using Getter = EventSource* (*)();
    constexpr std::uint64_t kVerifiedOgEventGetter = 476335;
    constexpr std::uintptr_t kSetActiveRva = 0x005D6AC0u;
    constexpr std::uintptr_t kGetterRva = 0x005E2FD0u;
    constexpr std::uintptr_t kDispatchRva = 0x005F5110u;
    constexpr std::uint16_t kTracked = 0x0800;
    constexpr std::uint32_t kEventLimit = 256;
    static_assert(sizeof(Event) == 0x10);
    static_assert(offsetof(RE::TESQuest, data) == 0xF0);
    static_assert(offsetof(RE::QUEST_DATA, flags) == 0x4);
    static_assert(static_cast<std::int32_t>(RE::TESQuestEvent::kUpdateQuestActiveStatus) == 0);
    static_assert(static_cast<std::uint16_t>(RE::QuestFlag::kActive) == kTracked);

    [[nodiscard]] bool MatchesCapturedOgEngine() noexcept {
        // Validate the actual mapped code before invoking even the
        // event-source getter. Addresses alone do not establish its ABI.
        const auto module = GetModuleHandleW(nullptr);
        if (!module) return false;
        const auto base = reinterpret_cast<std::uintptr_t>(module);
        constexpr std::size_t kReadSize = 0x45;
        const auto at = base + kSetActiveRva;
        MEMORY_BASIC_INFORMATION page{};
        if (!VirtualQuery(reinterpret_cast<const void*>(at), &page, sizeof(page)) ||
            page.State != MEM_COMMIT ||
            (page.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const auto protection = page.Protect & 0xFFu;
        if (protection != PAGE_EXECUTE && protection != PAGE_EXECUTE_READ &&
            protection != PAGE_EXECUTE_READWRITE &&
            protection != PAGE_EXECUTE_WRITECOPY) return false;
        const auto end = reinterpret_cast<std::uintptr_t>(page.BaseAddress) + page.RegionSize;
        if (at >= end || kReadSize > end - at) return false;

        const auto* code = reinterpret_cast<const unsigned char*>(at);
        constexpr std::array<unsigned char, 8> kKnownEntry{
            0x48u, 0x83u, 0xECu, 0x38u, 0x84u, 0xD2u, 0x74u, 0x0Eu
        };
        if (std::memcmp(code, kKnownEntry.data(), kKnownEntry.size()) != 0)
            return false;
        const auto callTarget = [code](std::size_t offset)->std::int64_t {
            if (code[offset] != 0xE8u) return -1;
            std::int32_t rel{};
            std::memcpy(&rel, code + offset + 1, sizeof(rel));
            return static_cast<std::int64_t>(kSetActiveRva)
                + static_cast<std::int64_t>(offset) + 5 + rel;
        };
        return callTarget(0x2Fu) == static_cast<std::int64_t>(kGetterRva) &&
               callTarget(0x3Cu) == static_cast<std::int64_t>(kDispatchRva);
    }

    std::atomic_bool attempted{false};
    std::atomic_uint32_t eventCount{0};
    std::atomic_uint32_t epoch{0};

    class Sink final : public RE::BSTEventSink<Event> {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            const Event& event, EventSource*) override {
            const auto n = eventCount.fetch_add(1, std::memory_order_relaxed);
            if (n >= kEventLimit) {
                if (n == kEventLimit)
                    REX::WARN("QuestEventObserver: event log limit reached");
                return RE::BSEventNotifyControl::kContinue;
            }
            // The engine owns event.quest; dereference only in its callback.
            // Never retain TESQuest* or send it to another thread.
            if (event.quest) {
                const auto id = event.quest->GetFormID();
                const auto flags = static_cast<std::uint16_t>(event.quest->data.flags);
                REX::INFO("QuestEventObserver: event={} form={:08X} tracked={} enabled={} completed={} failed={} epoch={}",
                    static_cast<std::int32_t>(event.changeType), id,
                    (flags & kTracked) != 0, (flags & 0x1u) != 0,
                    (flags & 0x2u) != 0, (flags & 0x40u) != 0,
                    epoch.load(std::memory_order_relaxed));
            } else {
                REX::INFO("QuestEventObserver: event={} null-quest epoch={}",
                    static_cast<std::int32_t>(event.changeType),
                    epoch.load(std::memory_order_relaxed));
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };
    // Lifetime is game-process-long. No unload/hot-reload path is supported.
    Sink sink;

    void F4SEAPI OnMessage(F4SE::MessagingInterface::Message* msg) {
        if (!msg) return;
        using M = F4SE::MessagingInterface;
        if (msg->type == M::kGameDataReady) {
            if (attempted.exchange(true, std::memory_order_acq_rel)) return;
            if (!MatchesCapturedOgEngine()) {
                REX::ERROR("QuestEventObserver: OG loaded instruction validation failed; no subscription");
                return;
            }
            REX::INFO("QuestEventObserver: verified two engine call destinations; acquiring source");
            // Resolves getter documented by the in-memory SetActive trace.
            // Only event-source initialization/registration; no quest calls.
            REL::Relocation<Getter> getter{REL::ID(kVerifiedOgEventGetter)};
            if (getter.address() != reinterpret_cast<std::uintptr_t>(
                                        GetModuleHandleW(nullptr)) + kGetterRva) {
                REX::ERROR("QuestEventObserver: Address Library conflict; no subscription");
                return;
            }
            auto* source = getter();
            if (source) {
                source->RegisterSink(&sink);
                REX::INFO("QuestEventObserver: subscribed (vanilla events only)");
            } else {
                REX::ERROR("QuestEventObserver: source unavailable");
            }
        } else if (msg->type == M::kPreLoadGame ||
                   msg->type == M::kPostLoadGame ||
                   msg->type == M::kNewGame) {
            const auto current = epoch.fetch_add(1, std::memory_order_relaxed) + 1;
            REX::INFO("QuestEventObserver: load boundary={} epoch={}", msg->type, current);
        } else if (msg->type == M::kPreSaveGame ||
                   msg->type == M::kPostSaveGame) {
            REX::INFO("QuestEventObserver: save boundary={} (name omitted)", msg->type);
        }
    }
}

F4SE_EXPORT bool F4SEAPI F4SEPlugin_Query(
    const F4SE::QueryInterface* f4se, F4SE::PluginInfo* info) {
    if (!f4se || !info || f4se->IsEditor() ||
        f4se->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    info->infoVersion = F4SE::PluginInfo::kVersion;
    info->name = "K2040_Quest_Event_Observer";
    info->version = 1;
    return true;
}
F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* load) {
    if (!load || load->RuntimeVersion() != F4SE::RUNTIME_1_10_163) return false;
    F4SE::Init(load, F4SE::InitInfo{
        .log = true,
        .logName = "K2040_Quest_Event_Observer",
        .hook = false
    });
    auto* messages = F4SE::GetMessagingInterface();
    return messages && messages->RegisterListener(OnMessage);
}
