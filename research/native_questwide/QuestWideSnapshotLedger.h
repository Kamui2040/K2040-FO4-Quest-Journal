#pragma once
// Pure bounded ledger for a read-only native Pip-Boy quest feed.
// No RE/F4SE, game memory, strings, callback invocation or game writes.
#include "QuestWideNativePreflight.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace K2040::QuestWideNativeResearch {
struct FeedLedger {
    std::array<FeedRow,kMaxFeedRows> matchingRows{};
    std::size_t count{};
    std::size_t scanned{};
    bool complete{true};
    bool truncated{false};

    // All visited top-level and nested entries count against one cap.
    // Only rows for the requested TESQuest are retained.
    void Observe(std::optional<std::uint32_t> formID,
                 std::optional<std::uint32_t> instanceID,
                 std::optional<bool> active,
                 RowKind kind, std::uint32_t targetFormID) noexcept {
        if(scanned>=kMaxFeedRows) {
            complete=false;truncated=true;return;
        }
        ++scanned;
        if(!formID) {
            complete=false;return;
        }
        if(*formID!=targetFormID || !targetFormID)return;
        if(count>=matchingRows.size()) {
            complete=false;truncated=true;return;
        }
        matchingRows[count++]={*formID,instanceID.value_or(0),kind,
                               instanceID.has_value()&&active.has_value(),
                               active.value_or(false)};
    }
    [[nodiscard]] std::span<const FeedRow> Rows() const noexcept {
        return {matchingRows.data(),count};
    }
};
}
