// Compile-only design probe: NOT included in the production xmake target.
// Every native entry point denies writes. No mailbox or engine mutations.
// The Papyrus worker MUST NOT be packaged without an independently verified
// and approved implementation, owned quest record and save/load QA.
#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <cstdint>
#include <variant>
#include <vector>

namespace K2040::ResearchTracking {
    using namespace std::literals;
    namespace {
        bool IsEnabled(std::monostate) { return false; }
        std::vector<std::int32_t> Claim(std::monostate) { return {}; }
        bool IsCurrent(std::monostate, std::int32_t) { return false; }
        bool Complete(std::monostate, std::int32_t, bool) { return false; }
        bool Cancel(std::monostate, std::int32_t) { return false; }
    }

    bool RegisterDisabledBridge(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) return false;
        vm->BindNativeMethod("K2040QuestJournalNative"sv, "IsEnabled"sv,
                             IsEnabled, false);
        vm->BindNativeMethod("K2040QuestJournalNative"sv, "Claim"sv,
                             Claim, false);
        vm->BindNativeMethod("K2040QuestJournalNative"sv, "IsCurrent"sv,
                             IsCurrent, false);
        vm->BindNativeMethod("K2040QuestJournalNative"sv, "Complete"sv,
                             Complete, false);
        vm->BindNativeMethod("K2040QuestJournalNative"sv, "Cancel"sv,
                             Cancel, false);
        return true;
    }
}
