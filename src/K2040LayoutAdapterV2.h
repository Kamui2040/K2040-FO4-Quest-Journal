#pragma once
// Optional full-geometry extension to K2040LayoutAdapterV1.
// V1 remains unmodified for position-only owners. Callbacks are game-thread.
// beginEdit captures last SAVED layout; resetPreview restores it in-place.
// previewGeometry must be reversible and MUST NOT persist before finishEdit(true).
#include <cstdint>
#ifdef _WIN32
#define K2040_LAYOUT_V2_CALL __cdecl
#else
#define K2040_LAYOUT_V2_CALL
#endif
inline constexpr std::uint32_t K2040_LAYOUT_ADAPTER_ABI_V2 = 2;
inline constexpr const char* K2040_LAYOUT_EXPORT_V2 = "K2040_GetLayoutAdapterV2";
struct K2040LayoutAdapterV2 {
    std::uint32_t size;
    std::uint32_t abiVersion;
    const char* moduleId;
    bool (K2040_LAYOUT_V2_CALL* isAvailable)() noexcept;
    const char* (K2040_LAYOUT_V2_CALL* elementsJson)() noexcept;
    bool (K2040_LAYOUT_V2_CALL* beginEdit)() noexcept;
    bool (K2040_LAYOUT_V2_CALL* previewGeometry)(
        const char* elementId, double x, double y,
        double width, double height) noexcept;
    bool (K2040_LAYOUT_V2_CALL* resetPreview)() noexcept;
    bool (K2040_LAYOUT_V2_CALL* finishEdit)(bool commit) noexcept;
};
using K2040GetLayoutAdapterV2=const K2040LayoutAdapterV2*
    (K2040_LAYOUT_V2_CALL*)() noexcept;
