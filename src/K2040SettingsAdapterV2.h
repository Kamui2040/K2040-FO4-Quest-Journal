#pragma once
// Plain C ABI: all char pointers are UTF-8, valid until the next call to the
// same provider. Calls originate only on the Fallout 4 game thread.
#include <cstdint>
#ifdef _WIN32
# define K2040_SETTINGS_V2_CALL __cdecl
#else
# define K2040_SETTINGS_V2_CALL
#endif
inline constexpr std::uint32_t K2040_SETTINGS_ADAPTER_V2_ABI = 2;
struct K2040SettingsAdapterV2 {
    std::uint32_t size;
    std::uint32_t abiVersion;
    const char* moduleId;
    bool (K2040_SETTINGS_V2_CALL* isAvailable)() noexcept;
    const char* (K2040_SETTINGS_V2_CALL* manifestJson)() noexcept;
    const char* (K2040_SETTINGS_V2_CALL* valuesJson)() noexcept;
    bool (K2040_SETTINGS_V2_CALL* applyValue)(const char* fieldId,const char* jsonValue) noexcept;
    bool (K2040_SETTINGS_V2_CALL* resetSetting)(const char* fieldId) noexcept;
    // Optional: apply theme to an existing owner UI, without rewriting
    // native gameplay appearance preferences. NULL is valid.
    bool (K2040_SETTINGS_V2_CALL* applyTheme)(const char* themeId) noexcept;
};
// For adapter owners: return a stable static structure only. Return false on
// persistence failure; do not write another module's settings or MCM registry.
using K2040GetSettingsAdapterV2=const K2040SettingsAdapterV2*
    (K2040_SETTINGS_V2_CALL*)() noexcept;
inline constexpr const char* K2040_SETTINGS_V2_EXPORT="K2040_GetSettingsAdapterV2";
// Mod clients can optionally query the effective theme even before the hub
// has been opened, via the hub DLL's K2040_CopyEffectiveThemeV1 export.
// Signature: bool __cdecl(const char* knownModuleId,char* output,uint32_t capacity).
inline constexpr const char* K2040_THEME_QUERY_EXPORT="K2040_CopyEffectiveThemeV1";
