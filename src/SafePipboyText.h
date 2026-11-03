#pragma once
// Defensive copy of Pip-Boy strings. Never follows unbounded/unknown pointers.
// This is not a substitute for validating the target game's memory layout.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace K2040::Quests {
inline std::string ReadBoundedGameString(const char* input) {
    constexpr std::size_t kMaxTextBytes = 4096;
    if (!input || reinterpret_cast<std::uintptr_t>(input) < 0x10000)
        return {};
    std::string result;
    // Check every crossed Windows virtual-memory region before reading bytes.
    // A 4 KB cap prevents malformed game text from consuming unbounded memory.
    while (result.size() < kMaxTextBytes) {
        const auto* cursor = input + result.size();
        std::size_t available = kMaxTextBytes - result.size();
#ifdef _WIN32
        MEMORY_BASIC_INFORMATION region{};
        if (!::VirtualQuery(cursor, &region, sizeof(region)) ||
            region.State != MEM_COMMIT ||
            (region.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
            return {};
        const auto protection = region.Protect & 0xff;
        if (protection != PAGE_READONLY && protection != PAGE_READWRITE &&
            protection != PAGE_WRITECOPY && protection != PAGE_EXECUTE_READ &&
            protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY)
            return {};
        const auto begin = reinterpret_cast<std::uintptr_t>(cursor);
        const auto end = reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize;
        if (end <= begin || end < reinterpret_cast<std::uintptr_t>(region.BaseAddress))
            return {};
        const auto readable = static_cast<std::size_t>(end - begin);
        if (readable < available) available = readable;
#endif
        const void* terminator = std::memchr(cursor, '\0', available);
        const auto count = terminator
            ? static_cast<std::size_t>(static_cast<const char*>(terminator) - cursor)
            : available;
        result.append(cursor, count);
        if (terminator) return result;
    }
    // Missing NUL within the strict cap is invalid, not a truncated quest.
    return {};
}
}
