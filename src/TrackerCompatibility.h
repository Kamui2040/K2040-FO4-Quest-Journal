#pragma once

// Quest Journal owns only its own mini tracker. Phosphor HUD owns its HUD and
// must never be disabled, modified or queried through unsupported internals.
namespace K2040::TrackerCompatibility {
    [[nodiscard]] constexpr bool ShowMiniTracker(bool userEnabled,
                                                 bool conflictingHudActive,
                                                 bool allowWithConflictingHud) noexcept {
        return userEnabled && (!conflictingHudActive || allowWithConflictingHud);
    }
}
