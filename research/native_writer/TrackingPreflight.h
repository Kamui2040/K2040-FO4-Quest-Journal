#pragma once

// Offline validation prototype for a future ESP-free OG native quest writer.
// Research-only: not included from src/, and has NO RE/F4SE/game dependency.
// A ready decision does not authorize execution in the shipping journal.

#include <cstdint>

namespace K2040::NativeTrackingResearch {
    inline constexpr std::uint16_t kQuestEnabled = 0x0001;
    inline constexpr std::uint16_t kQuestCompleted = 0x0002;
    inline constexpr std::uint16_t kQuestFailed = 0x0040;
    inline constexpr std::uint16_t kQuestTracked = 0x0800;
    inline constexpr std::int8_t kMiscQuestType = 6;

    enum class Decision : std::uint8_t {
        kDenied,
        kAlreadyMatching,
        kReadyForUnwiredReview
    };

    struct Request {
        std::uint32_t formId{};
        bool desiredTracked{};
        std::uint64_t saveEpoch{};
        std::uint64_t uiEpoch{};
    };

    struct Observation {
        // These must be freshly sourced from the authoritative game state
        // *on the game thread*, just before any future native call.
        std::uint32_t formId{};
        std::uint64_t currentSaveEpoch{};
        std::uint64_t currentUiEpoch{};
        bool runtimeIsOG163{};
        bool gameLoaded{};
        bool gameThread{};
        bool journalOpenAndFocused{};
        bool formIsQuest{};
        bool inCurrentPipboyFeed{};
        std::uint16_t flags{};
        std::int8_t questType{};
    };

    [[nodiscard]] constexpr Decision Evaluate(
        const Request& request, const Observation& observation) noexcept {
        if (!request.formId || !request.saveEpoch || !request.uiEpoch ||
            request.formId != observation.formId ||
            request.saveEpoch != observation.currentSaveEpoch ||
            request.uiEpoch != observation.currentUiEpoch ||
            !observation.runtimeIsOG163 || !observation.gameLoaded ||
            !observation.gameThread || !observation.journalOpenAndFocused ||
            !observation.formIsQuest || !observation.inCurrentPipboyFeed ||
            observation.questType == kMiscQuestType ||
            !(observation.flags & kQuestEnabled) ||
            (observation.flags & (kQuestCompleted | kQuestFailed)))
            return Decision::kDenied;

        const bool tracked = (observation.flags & kQuestTracked) != 0;
        return tracked == request.desiredTracked
            ? Decision::kAlreadyMatching : Decision::kReadyForUnwiredReview;
    }

    enum class Confirmation : std::uint8_t {
        kUnverified,
        kConfirmed,
        kMismatch
    };

    struct Postcondition {
        std::uint32_t formId{};
        std::uint64_t saveEpoch{};
        std::uint64_t uiEpoch{};
        bool pipboyFeedAvailable{};
        bool visibleInPipboyFeed{};
        bool engineFlagTracked{};
        bool pipboyFeedTracked{};
    };

    // An F4SE task being queued, a native function returning, or even an
    // engine flag changing alone does NOT demonstrate a successful write.
    [[nodiscard]] constexpr Confirmation Confirm(
        const Request& request, const Postcondition& observed) noexcept {
        if (!request.formId || request.formId != observed.formId ||
            !request.saveEpoch || request.saveEpoch != observed.saveEpoch ||
            !request.uiEpoch || request.uiEpoch != observed.uiEpoch ||
            !observed.pipboyFeedAvailable || !observed.visibleInPipboyFeed)
            return Confirmation::kUnverified;
        return observed.engineFlagTracked == request.desiredTracked &&
                       observed.pipboyFeedTracked == request.desiredTracked
            ? Confirmation::kConfirmed : Confirmation::kMismatch;
    }

    static_assert(Evaluate({}, {}) == Decision::kDenied);
    static_assert(Confirm({}, {}) == Confirmation::kUnverified);
}
