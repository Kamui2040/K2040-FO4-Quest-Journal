#pragma once
// Pure, read-only candidate comparison for OG163 Pip-Boy quest row fields.
// A row's named numeric members are NOT proven to match the three
// values extracted by SetQuestActive from a vanilla selectedEntry object.
// In particular, the native onQuestSelection callback populates the
// Misc objectives list separately from general quest-feed display leaves.
#include <cstdint>
#include <optional>

namespace K2040::SelectorRowCorrelation {
    enum class Kind : std::uint8_t {
        kQuestRoot, kMiscChild, kSyntheticMiscRoot
    };
    enum class Status : std::uint8_t {
        kInvalidRow, kSyntheticRootTypeSix, kSyntheticRootTypeUnknown,
        kSyntheticRootTypeMismatch, kUnresolvedQuest, kFormMismatch,
        kMissingInstance, kInstanceMismatch, kMissingCategory,
        kAmbiguousCategory, kCategoryMismatch, kCandidateNotVerified
    };
    struct Observation {
        Kind kind{Kind::kQuestRoot};
        std::optional<std::uint32_t> rowFormID{};
        std::optional<std::uint32_t> rowInstanceID{};
        std::optional<std::uint32_t> rowCategory{};
        bool categoryAmbiguous{};
        bool nativeQuestFound{};
        std::uint32_t nativeFormID{};
        std::uint32_t nativeCurrentInstanceID{};
        std::uint32_t nativeQuestType{};
    };
    [[nodiscard]] constexpr Status Compare(const Observation& o) noexcept {
        if (!o.rowFormID)
            return Status::kInvalidRow;
        if (o.kind==Kind::kSyntheticMiscRoot) {
            if (*o.rowFormID!=0u)
                return Status::kInvalidRow;
            if (o.categoryAmbiguous)
                return Status::kAmbiguousCategory;
            if (!o.rowCategory)
                return Status::kSyntheticRootTypeUnknown;
            return *o.rowCategory==6u
                ? Status::kSyntheticRootTypeSix
                : Status::kSyntheticRootTypeMismatch;
        }
        if (*o.rowFormID==0u)
            return Status::kInvalidRow;
        if (!o.nativeQuestFound)
            return Status::kUnresolvedQuest;
        if (*o.rowFormID!=o.nativeFormID)
            return Status::kFormMismatch;
        if (!o.rowInstanceID)
            return Status::kMissingInstance;
        if (*o.rowInstanceID!=o.nativeCurrentInstanceID)
            return Status::kInstanceMismatch;
        if (o.categoryAmbiguous)
            return Status::kAmbiguousCategory;
        if (!o.rowCategory)
            return Status::kMissingCategory;
        if (*o.rowCategory!=o.nativeQuestType)
            return Status::kCategoryMismatch;
        // Equality of named, typed members to TESQuest fields is structural
        // evidence only. Never authorize a vanilla command from this.
        return Status::kCandidateNotVerified;
    }
    [[nodiscard]] constexpr const char* Name(Status s) noexcept {
        switch(s) {
            case Status::kInvalidRow: return "invalid-row";
            case Status::kSyntheticRootTypeSix: return "misc-root-type-six-candidate";
            case Status::kSyntheticRootTypeUnknown: return "misc-root-type-unknown";
            case Status::kSyntheticRootTypeMismatch: return "misc-root-type-mismatch";
            case Status::kUnresolvedQuest: return "unresolved-native-quest";
            case Status::kFormMismatch: return "form-mismatch";
            case Status::kMissingInstance: return "missing-instance";
            case Status::kInstanceMismatch: return "instance-mismatch";
            case Status::kMissingCategory: return "missing-row-type";
            case Status::kAmbiguousCategory: return "conflicting-row-type-keys";
            case Status::kCategoryMismatch: return "type-mismatch";
            case Status::kCandidateNotVerified: return "structural-candidate-NOT-command-ABI";
        }
        return "unknown";
    }
}
