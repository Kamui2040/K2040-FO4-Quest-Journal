#pragma once
// Offline metadata-derived OG163 primary PipboyMenu code-object slots.
// These are read-only RVAs; NEVER convert to callable, hooked or writable code.
#include <cstddef>
#include <cstdint>

namespace K2040::PipboyDispatchResearch {
inline constexpr std::uintptr_t kImageSize=0x06CE6000u;
inline constexpr std::uintptr_t kExecutableStart=0x00001000u;
inline constexpr std::uintptr_t kExecutableEnd=0x02C0C000u;
inline constexpr std::uintptr_t kPipboyPrimaryVtableRva=0x02D22B18u;
inline constexpr std::uintptr_t kExpectedCallRva=0x00B94080u;
inline constexpr std::uintptr_t kExpectedMapRva=0x00B93D30u;
inline constexpr std::size_t kSlotCount=3u;
inline constexpr std::size_t kPrefixBytes=192u;
inline constexpr std::uintptr_t kImageBaseHeader=0x140000000ull;

static_assert(kPrefixBytes<=256);
static_assert(kPipboyPrimaryVtableRva+kSlotCount*sizeof(std::uintptr_t)<kImageSize);
static_assert(kExpectedCallRva+kPrefixBytes<kExecutableEnd);
static_assert(kExpectedMapRva+kPrefixBytes<kExecutableEnd);
[[nodiscard]] constexpr bool ValidCodeRva(const std::uintptr_t rva,
                                           const std::size_t size) noexcept {
    return size>0 && size<=256 && rva>=kExecutableStart &&
           rva<kExecutableEnd && size<=kExecutableEnd-rva;
}
[[nodiscard]] constexpr bool ValidSlotRvas(
        const std::uintptr_t callRva,
        const std::uintptr_t mapRva) noexcept {
    return callRva==kExpectedCallRva &&
           mapRva==kExpectedMapRva &&
           callRva!=mapRva &&
           ValidCodeRva(callRva,kPrefixBytes) &&
           ValidCodeRva(mapRva,kPrefixBytes);
}
}
