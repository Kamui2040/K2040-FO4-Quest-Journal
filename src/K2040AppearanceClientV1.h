#pragma once
// Optional VNext appearance query. Copy into an individual K2040 mod's include
// directory and validate the returned JSON before applying visual tokens.
// This header neither loads the hub nor modifies the mod's saved preferences.
#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
namespace K2040::AppearanceClient {
using CopyFn=bool(__cdecl*)(const char*,char*,std::uint32_t) noexcept;
inline std::optional<std::string> Current(const char* registeredModuleId){
    if(!registeredModuleId)return std::nullopt;
    const auto hub=GetModuleHandleW(L"K2040_Unified_Settings.dll");
    if(!hub)return std::nullopt;
    const auto proc=GetProcAddress(hub,"K2040_CopyAppearanceV1");
    if(!proc)return std::nullopt;
    const auto copy=reinterpret_cast<CopyFn>(proc);
    char buffer[4096]{};
    if(!copy(registeredModuleId,buffer,static_cast<std::uint32_t>(sizeof(buffer))))
        return std::nullopt;
    // A future/mismatched hub must not cause an unterminated string read.
    const auto* terminator=std::find(std::begin(buffer),std::end(buffer),'\0');
    if(terminator==std::end(buffer))return std::nullopt;
    // The owning mod must validate theme, palette and semantic RGB colors.
    // Build with MSVC/clang-cl: char* and const char* cannot form one
    // input-iterator constructor pair. Use the validated bounded length.
    return std::string(buffer,static_cast<std::size_t>(terminator-buffer));
}
}
