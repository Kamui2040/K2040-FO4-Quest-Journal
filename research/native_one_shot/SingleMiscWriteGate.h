#pragma once
// Pure, fail-closed Dogmeat-only preflight; NEVER calls an engine API.
// Shared by the isolated verified experiment and separately gated production candidate.
#if !defined(K2040_SINGLE_MISC_WRITE_QA) && \
    !defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE)
#error Dogmeat-only gate may compile only into approved isolated builds
#endif
#include <cstdint>
namespace K2040::SingleMiscQA {
    // Vanilla Fallout4.esm DogmeatQuest. Never permit a similarly shaped
    // unrelated or mod-added Misc quest to inherit this verified path.
    inline constexpr std::uint32_t kVerifiedQuestFormID = 0x0006DFAFu;
    inline constexpr std::uint32_t kObservedIndex = 50;
    inline constexpr std::uint32_t kObservedInstance = 1;
    enum class Verdict : std::uint8_t { kReject, kAlready, kReady };
    struct Observation {
        bool validSession{};
        bool selectedAndObserved{};
        bool complete{};
        bool truncated{};
        bool rootRowAbsent{};
        bool exactlyOneNestedRow{};
        bool nestedActiveTyped{};
        bool nestedEnabledTyped{};
        bool nestedEnabled{};
        bool nestedCompleted{};
        bool nestedFailed{};
        bool nestedInstanceTyped{};
        bool nestedIndexAbsent{};
        bool exactlyOneNativeDisplayed{};
        std::uint32_t formID{};
        std::uint32_t nativeIndex{};
        std::uint32_t nativeInstance{};
        std::uint32_t nestedInstance{};
        std::uint32_t currentInstance{};
        std::uint32_t questType{};
        std::uint32_t questFlags{};
        bool nestedActive{};
    };
    [[nodiscard]] constexpr Verdict Review(const Observation& o,
                                           bool desired) noexcept {
        constexpr std::uint32_t kRunning=1u,kCompleted=2u,
                                kFailed=0x40u,kTracked=0x800u;
        if(o.formID!=kVerifiedQuestFormID ||
           !o.validSession || !o.selectedAndObserved || !o.complete ||
           o.truncated || !o.rootRowAbsent || !o.exactlyOneNestedRow ||
           !o.nestedActiveTyped || !o.nestedEnabledTyped || !o.nestedEnabled ||
           o.nestedCompleted || o.nestedFailed || !o.nestedInstanceTyped ||
           !o.nestedIndexAbsent || !o.exactlyOneNativeDisplayed ||
           o.nativeIndex!=kObservedIndex ||
           o.nativeInstance!=kObservedInstance ||
           o.nestedInstance!=kObservedInstance ||
           o.currentInstance!=kObservedInstance ||
           o.questType!=6u || !(o.questFlags&kRunning) ||
           (o.questFlags&(kCompleted|kFailed)) ||
           bool(o.questFlags&kTracked)!=o.nestedActive)
            return Verdict::kReject;
        return o.nestedActive==desired ? Verdict::kAlready:Verdict::kReady;
    }
}
