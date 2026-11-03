// Compilation-only research. NOT in production xmake.lua or shipped mod.
// NO F4SE entrypoints, no task scheduling, no hooks, no UI callbacks,
// no static initialization and no automatic execution.
//
// The OG 1.10.163 loaded SetActive function at RVA 0x005D6AC0
// receives quest object in RCX and bool in DL, toggles TESQuest.flags
// 0x800, then dispatches TESQuestEvent::kUpdateQuestActiveStatus.
// Relocation ID 1567562 was independently verified against the user's
// installed Address Library and published OG mapping. Return value
// semantics are NOT verified, so the prototype intentionally discards it.
//
// IMPORTANT: This translation unit is excluded from the shipping build;
// callable native writes still require explicit separate runtime QA.
#include <RE/Fallout.h>
#include <REL/Relocation.h>
#include <cstdint>
#include "TrackingPreflight.h"

namespace K2040::NativeTrackingResearch {
    static_assert(offsetof(RE::TESQuest, data) == 0xF0);
    static_assert(offsetof(RE::QUEST_DATA, flags) == 0x4);
    static_assert(static_cast<std::uint16_t>(RE::QuestFlag::kActive) == kQuestTracked);
    static_assert(sizeof(RE::TESQuestEvent::Event) == 0x10);
    static_assert(RE::TESQuestEvent::kUpdateQuestActiveStatus == 0);

    // Do not expose a callable engine entrypoint here. To reduce the risk
    // of accidental experimentation, only the compiler-verified ABI alias
    // and ID are retained. A separate approved experimental test module
    // must independently implement and review invocation, form lookup,
    // lifecycle and native-thread ownership.
    using OgSetActiveSignature = void(*)(RE::TESQuest*, bool);
    inline constexpr std::uint64_t kOGSetActiveRelocationID = 1567562;
    [[maybe_unused]] constexpr std::uint64_t kOGGetActiveRelocationID = 690102;
    static_assert(sizeof(OgSetActiveSignature) == sizeof(void(*)()));
}
