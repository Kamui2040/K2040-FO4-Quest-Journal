#pragma once
// Research-only correlation of read-only objective identities. Never a writer.
// A matching Pip-Boy numeric field is a CANDIDATE, not a proven row mapping.
#if !defined(K2040_TRACKING_INTEGRATED_QA)
#error Misc identity correlation is for isolated read-only QA only
#endif
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace K2040::MiscIdentity {
    struct NativeObjective {
        std::uint32_t index{};
        std::uint32_t instanceID{};
    };
    struct Row {
        std::optional<std::uint32_t> index;
        std::optional<std::uint32_t> instance;
    };
    enum class Status : std::uint8_t {
        kIncomplete, kMissingInstance, kNoCandidates,
        kUniqueCandidate, kAmbiguous
    };
    struct Correlation {
        Status status{Status::kIncomplete};
        std::size_t count{};
    };
    [[nodiscard]] inline Correlation Compare(
        const Row& row, std::span<const NativeObjective> native,
        bool complete) noexcept {
        if(!complete)return {Status::kIncomplete,0};
        if(!row.instance)return {Status::kMissingInstance,0};
        std::size_t matches=0;
        for(const auto& candidate:native) {
            if(candidate.instanceID!=*row.instance)continue;
            if(row.index && candidate.index!=*row.index)continue;
            ++matches;
        }
        if(!matches)return {Status::kNoCandidates,0};
        return {matches==1?Status::kUniqueCandidate:Status::kAmbiguous,matches};
    }
    [[nodiscard]] inline const char* StatusName(Status value) noexcept {
        switch(value) {
            case Status::kIncomplete:return "incomplete";
            case Status::kMissingInstance:return "missing-instance";
            case Status::kNoCandidates:return "no-candidates";
            case Status::kUniqueCandidate:return "single-candidate-not-verified";
            case Status::kAmbiguous:return "ambiguous";
        }
        return "unknown";
    }
}
