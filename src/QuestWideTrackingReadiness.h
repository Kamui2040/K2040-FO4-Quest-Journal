#pragma once
// Production-side, read-only eligibility summary for a *future* engine-managed
// TESQuest-wide tracking action. Never authorizes or performs a native write.
// The operation itself must revalidate independent live game-thread evidence.
#include <cstdint>

namespace K2040::QuestWideReadiness {
enum class Status : std::uint8_t {
    kInvalidIdentity, kUnavailable, kUnknownScope,
    kUnverifiedInstance, kNoDisplayedNativeObjectives,
    kNoMiscRows, kUnverifiedPipboyFeed, kCandidateReadOnly
};

struct Snapshot {
    std::uint32_t formId{};
    bool questActive{};
    bool historyOnly{};
    bool questWide{};
    bool isMisc{};
    bool instanceConsistent{};
    bool instancePresent{}; // zero is valid; do not coerce to bool(instance ID)
    std::uint32_t displayedNativeObjectives{};
    std::uint32_t miscRows{};
    bool pipboyFeedMatches{};
};

[[nodiscard]] constexpr Status Assess(const Snapshot& s) noexcept {
    if (!s.formId) return Status::kInvalidIdentity;
    if (!s.questActive || s.historyOnly) return Status::kUnavailable;
    if (!s.questWide) return Status::kUnknownScope;
    if (!s.instanceConsistent || !s.instancePresent)
        return Status::kUnverifiedInstance;
    if (!s.displayedNativeObjectives || s.displayedNativeObjectives>512u)
        return Status::kNoDisplayedNativeObjectives;
    if (s.isMisc && (!s.miscRows || s.miscRows>512u))
        return Status::kNoMiscRows;
    if (!s.pipboyFeedMatches) return Status::kUnverifiedPipboyFeed;
    return Status::kCandidateReadOnly;
}

[[nodiscard]] constexpr const char* Name(Status status) noexcept {
    switch(status) {
    case Status::kInvalidIdentity: return "invalid-quest";
    case Status::kUnavailable: return "not-active";
    case Status::kUnknownScope: return "unknown-scope";
    case Status::kUnverifiedInstance: return "instance-unverified";
    case Status::kNoDisplayedNativeObjectives: return "native-objectives-unavailable";
    case Status::kNoMiscRows: return "misc-feed-unavailable";
    case Status::kUnverifiedPipboyFeed: return "pipboy-feed-unverified";
    case Status::kCandidateReadOnly: return "candidate-read-only";
    }
    return "unknown-scope";
}
}
