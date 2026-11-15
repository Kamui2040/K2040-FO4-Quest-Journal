#pragma once
// Optional read-only theme query for standalone mod views starting after the
// shared hub. Absence of the hub preserves the owner's original appearance.
#include <Windows.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace K2040::ThemeClient {
using QueryFn = bool(__cdecl *)(const char*,char*,std::uint32_t) noexcept;
inline std::optional<std::string> Current(const char* knownModuleId){
    if(!knownModuleId)return std::nullopt;
    const auto hub=GetModuleHandleW(L"K2040_Unified_Settings.dll");
    if(!hub)return std::nullopt;
    auto* proc=GetProcAddress(hub,"K2040_CopyEffectiveThemeV1");
    if(!proc)return std::nullopt;
    const auto query=reinterpret_cast<QueryFn>(proc);
    char buffer[32]{};
    if(!query(knownModuleId,buffer,static_cast<std::uint32_t>(sizeof(buffer))))
        return std::nullopt;
    const std::string_view result(buffer);
    if(result!="default"&&result!="midnight"&&
       result!="high-contrast"&&result!="pipboy")return std::nullopt;
    return std::string(result);
}
}
