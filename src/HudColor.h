#pragma once
// Pure RGB-to-CSS conversion used for Fallout's live HUD colors.
// The live color source is CommonLibF4 HUDMenuUtils; no game types here.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace K2040::HudColor {
inline std::string ToCss(float red, float green, float blue) {
    constexpr char digits[] = "0123456789abcdef";
    std::string hex = "#000000";
    const float values[3] = {red, green, blue};
    for (int i = 0; i < 3; ++i) {
        const float finite = std::isfinite(values[i]) ? values[i] : 0.0F;
        const auto channel = static_cast<std::uint8_t>(
            std::lround(std::clamp(finite, 0.0F, 1.0F) * 255.0F));
        hex[1 + i * 2] = digits[(channel >> 4) & 15];
        hex[2 + i * 2] = digits[channel & 15];
    }
    return hex;
}
}
