#pragma once
// Test-only, no engine dependencies. One deliberate engine-managed write
// per process, after a recent vanilla event and deliberate armed hotkey.
#include <cstdint>

namespace K2040::NativeWriteTest {
    constexpr std::uint16_t kEnabled = 0x0001u;
    constexpr std::uint16_t kCompleted = 0x0002u;
    constexpr std::uint16_t kFailed = 0x0040u;
    constexpr std::uint16_t kTracked = 0x0800u;

    struct Snapshot {
        bool armed{};
        bool alreadyUsed{};
        bool gameLoaded{};
        bool validOgRuntime{};
        bool matchingEngine{};
        bool deliberateChord{};
        bool gameWindowForeground{};
        bool gameTaskMatchesVanillaThread{};
        bool pipboyClosed{};
        bool currentlyVisibleInPipboy{};
        bool authoritativeFeedAvailable{};
        bool authoritativeFeedTracked{};
        std::uint32_t candidateFormId{};
        std::uint32_t resolvedFormId{};
        std::uint64_t sessionEpoch{};
        std::uint64_t candidateEpoch{};
        std::uint64_t candidateAgeMs{};
        std::uint16_t questFlags{};
        std::int8_t questType{};
    };

    enum class Decision { kDeny, kNoChange, kAllowExactlyOnce };

    [[nodiscard]] constexpr Decision Evaluate(const Snapshot& s) noexcept {
        if (!s.armed || s.alreadyUsed || !s.gameLoaded ||
            !s.validOgRuntime || !s.matchingEngine ||
            !s.deliberateChord || !s.gameWindowForeground ||
            !s.gameTaskMatchesVanillaThread || !s.pipboyClosed ||
            !s.currentlyVisibleInPipboy ||
            !s.authoritativeFeedAvailable ||
            !s.candidateFormId || s.candidateFormId != s.resolvedFormId ||
            !s.sessionEpoch || s.sessionEpoch != s.candidateEpoch ||
            s.candidateAgeMs > 45000 ||
            !(s.questFlags & kEnabled) ||
            (s.questFlags & (kCompleted | kFailed)) ||
            s.questType == 6)
            return Decision::kDeny;
        const bool currentlyTracked = (s.questFlags & kTracked) != 0;
        if (currentlyTracked != s.authoritativeFeedTracked)
            return Decision::kDeny;
        // The test always requests the OPPOSITE of the fresh vanilla
        // state: no arbitrary hidden quest and no change to stages.
        return Decision::kAllowExactlyOnce;
    }

    static_assert(Evaluate({}) == Decision::kDeny);
}
