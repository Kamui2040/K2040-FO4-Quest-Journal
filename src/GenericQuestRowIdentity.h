#pragma once
// Pure, read-only mapping between game-provided Pip-Boy rows and displayed
// player quest objectives. No native engine calls and NO write authorization.
// The caller must collect a bounded, fresh snapshot on the F4SE game thread.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace K2040::GenericQuestRowIdentity {
    constexpr std::size_t kMaxPipboyRows=512;
    constexpr std::size_t kMaxPlayerObjectives=512;
    enum class Kind : std::uint8_t { kQuest, kMiscObjective };
    struct Row {
        std::uint32_t formID{};
        Kind kind{Kind::kMiscObjective};
        std::optional<std::uint32_t> objectiveIndex;
        std::optional<std::uint32_t> instanceID;
        bool typedAndComplete{false};
    };
    struct Native {
        std::uint32_t ownerFormID{};
        std::uint32_t objectiveIndex{};
        std::uint32_t instanceID{};
        bool displayed{false};
    };
    enum class Status : std::uint8_t {
        kIncomplete, kInvalidRow, kQuestWideOnly,
        kMissingInstance, kNoCandidate, kAmbiguous,
        kDuplicatePipboyRow, kUniqueCandidateNotVerified
    };
    struct Result {
        Status status{Status::kIncomplete};
        // An ordinal into the same immutable native snapshot. NEVER an engine
        // pointer, a live command parameter or independent write permission.
        std::optional<std::size_t> nativeOrdinal{};
    };

    [[nodiscard]] inline std::vector<Result> Resolve(
        std::span<const Row> rows,
        std::span<const Native> playerObjectives,
        bool complete) {
        // Never allocate unbounded memory for a malformed game feed.
        if (rows.size()>kMaxPipboyRows)
            return {Result{Status::kIncomplete,{}}};
        std::vector<Result> results(rows.size());
        if (!complete || playerObjectives.size()>kMaxPlayerObjectives)
            return results;
        for (std::size_t r=0;r<rows.size();++r) {
            const auto& row=rows[r];
            if (!row.formID || !row.typedAndComplete) {
                results[r].status=Status::kInvalidRow;
                continue;
            }
            if (row.kind==Kind::kQuest) {
                // A top-level quest is a quest-wide tracking entity.
                // There is no basis to infer one selectable objective.
                results[r].status=Status::kQuestWideOnly;
                continue;
            }
            if(row.kind!=Kind::kMiscObjective){
                results[r].status=Status::kInvalidRow;
                continue;
            }
            if (!row.instanceID) {
                results[r].status=Status::kMissingInstance;
                continue;
            }
            std::size_t matches=0;
            for (std::size_t n=0;n<playerObjectives.size();++n) {
                const auto& objective=playerObjectives[n];
                if (!objective.displayed || objective.ownerFormID!=row.formID ||
                    objective.instanceID!=*row.instanceID)
                    continue;
                if (row.objectiveIndex &&
                    objective.objectiveIndex!=*row.objectiveIndex)
                    continue;
                if (++matches==1)results[r].nativeOrdinal=n;
            }
            if (!matches) results[r].status=Status::kNoCandidate;
            else if (matches>1) {
                results[r].status=Status::kAmbiguous;
                results[r].nativeOrdinal.reset();
            } else results[r].status=Status::kUniqueCandidateNotVerified;
        }
        // Compare immutable first-pass matches before marking collisions.
        // Three or more copies of the same row must all be rejected, not
        // leave the final duplicate as an apparent unique identity.
        std::vector<bool> collision(results.size(),false);
        for(std::size_t i=0;i<results.size();++i) {
            if(results[i].status!=Status::kUniqueCandidateNotVerified)
                continue;
            for(std::size_t j=i+1;j<results.size();++j) {
                if(results[j].status==Status::kUniqueCandidateNotVerified &&
                   results[i].nativeOrdinal==results[j].nativeOrdinal) {
                    collision[i]=collision[j]=true;
                }
            }
        }
        for(std::size_t i=0;i<results.size();++i)
            if(collision[i]) results[i]={Status::kDuplicatePipboyRow,{}};
        return results;
    }

    [[nodiscard]] inline const char* StatusName(Status s) noexcept {
        switch(s){
            case Status::kIncomplete:return "incomplete";
            case Status::kInvalidRow:return "invalid-row";
            case Status::kQuestWideOnly:return "quest-wide-only";
            case Status::kMissingInstance:return "missing-instance";
            case Status::kNoCandidate:return "no-candidate";
            case Status::kAmbiguous:return "ambiguous";
            case Status::kDuplicatePipboyRow:return "duplicate-pipboy-row";
            case Status::kUniqueCandidateNotVerified:return "candidate-NOT-native-command";
        }
        return "unknown";
    }
}
