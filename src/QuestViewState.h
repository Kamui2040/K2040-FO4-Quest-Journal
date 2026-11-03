#pragma once
#include <cstdint>
#include <unordered_set>
#include <string_view>
namespace K2040::QuestViewState {
    enum class Kind : std::uint8_t { Pin, Favorite, Hidden };
    // Display metadata only: does not alter Fallout quest flags or objectives.
    // Returns whether the operation succeeded, not the new checked state.
    // Both inserting and removing an existing entry return true.
    bool Toggle(Kind kind, std::uint32_t formId);
    void Clear();
    const std::unordered_set<std::uint32_t>& Get(Kind kind);
    // Presentation-only categories, never mutate TESQuest.
    bool SetCategory(std::uint32_t formId, std::string_view category);
    std::string_view CategoryFor(std::uint32_t formId);
    // Records only quests the Pip-Boy has exposed; never indexes undiscovered quests.
    bool Remember(std::uint32_t formId);
    bool PinNew(std::uint32_t formId);
    bool InitialFeedSeen();
    void CompleteInitialFeed();
    const std::unordered_set<std::uint32_t>& Observed();
    void RegisterSerialization();
}
