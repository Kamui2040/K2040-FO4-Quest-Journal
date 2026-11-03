#pragma once
// Source-only, bounded summary of game-managed instanced objectives.
// The caller holds PlayerCharacter::questTargetLock while collecting,
// then sends only value types to this independent read-only index.
#include <cstdint>
#include <cstddef>
#include <unordered_map>
namespace K2040::GenericQuestInstance {
inline constexpr std::size_t kMaxObservations=512;
enum class State : std::uint8_t { kUnavailable, kNoDisplayedObjectives,
    kConsistent, kMultipleInstances, kCurrentInstanceMismatch };
struct Summary {
    std::uint32_t displayedCount{};
    std::uint32_t firstInstance{};
    std::uint32_t firstIndex{};
    bool multipleInstances{false};
};
class Index {
public:
    void Observe(std::uint32_t questFormID,std::uint32_t objectiveIndex,
                 std::uint32_t instanceID,bool displayed) {
        if (scanned_>=kMaxObservations){ complete_=false;return; }
        ++scanned_;
        if (!questFormID || !displayed)return;
        auto& entry=entries_[questFormID];
        if (!entry.displayedCount) {
            entry.firstInstance=instanceID;
            entry.firstIndex=objectiveIndex; // zero is a VALID objective index
        } else if (entry.firstInstance!=instanceID) entry.multipleInstances=true;
        ++entry.displayedCount;
    }
    [[nodiscard]] State Resolve(std::uint32_t formID,
                                std::uint32_t currentInstance) const {
        if(!complete_ || !formID)return State::kUnavailable;
        const auto it=entries_.find(formID);
        if(it==entries_.end())return State::kNoDisplayedObjectives;
        if(it->second.multipleInstances)return State::kMultipleInstances;
        return it->second.firstInstance==currentInstance
            ? State::kConsistent : State::kCurrentInstanceMismatch;
    }
    [[nodiscard]] std::uint32_t DisplayedCount(std::uint32_t formID) const {
        auto it=entries_.find(formID);
        return complete_ && it!=entries_.end()?it->second.displayedCount:0;
    }
    void MarkUnavailable() {complete_=false;}
    [[nodiscard]] bool Complete() const {return complete_;}
    [[nodiscard]] std::size_t Scanned() const {return scanned_;}
private:
    std::unordered_map<std::uint32_t,Summary> entries_{};
    std::size_t scanned_{};
    bool complete_{true};
};
[[nodiscard]] constexpr const char* Name(State value) {
    switch(value){
    case State::kUnavailable:return "unavailable";
    case State::kNoDisplayedObjectives:return "no-displayed-objectives";
    case State::kConsistent:return "consistent";
    case State::kMultipleInstances:return "multiple-instances";
    case State::kCurrentInstanceMismatch:return "instance-mismatch";
    }
    return "unavailable";
}
} // namespace K2040::GenericQuestInstance
