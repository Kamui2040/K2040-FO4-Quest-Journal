#pragma once

// Source-only proof of lossless FormID transfer through Papyrus 32-bit Int.
// No Bethesda or F4SE dependencies, and not referenced by production main.
#include "TrackingMailbox.h"
#include <array>
#include <bit>
#include <cstdint>
#include <limits>

namespace K2040::QuestTracking {
    static_assert(sizeof(std::int32_t) == sizeof(std::uint32_t));
    inline constexpr std::uint32_t kMaxPapyrusTicket =
        static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());

    // Returns three signed Papyrus cells: ticket, raw FormID bits, desired flag.
    // Papyrus may see a high-bit FormID as a negative integer. That is valid.
    [[nodiscard]] constexpr std::array<std::int32_t, 3> ToPapyrus(
        const PendingTracking& request) noexcept {
        return {static_cast<std::int32_t>(request.ticket),
                std::bit_cast<std::int32_t>(request.formId),
                request.desiredTracked ? 1 : 0};
    }

    [[nodiscard]] constexpr std::uint32_t FromPapyrusFormId(
        std::int32_t signedId) noexcept {
        return std::bit_cast<std::uint32_t>(signedId);
    }
}
