#pragma once
#include <array>
#include <cstdint>
// Gameplay action labels and XInput masks only. No MCM registry bridge.
namespace K2040::Hub {
enum class Action { Journal, Tracker };
inline std::uint32_t PadMask(unsigned button) noexcept {
    constexpr std::array<std::uint32_t,17> codes{
        0,0x1000,0x2000,0x4000,0x8000,0x0001,0x0002,0x0004,0x0008,
        0x0100,0x0200,1u<<16,1u<<17,0x0010,0x0020,0x0040,0x0080
    };
    return button<codes.size()?codes[button]:0;
}
}
