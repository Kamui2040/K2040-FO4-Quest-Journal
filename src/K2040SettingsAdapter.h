#pragma once
// Versioned cross-DLL settings adapter. No STL or game types cross this ABI.
#include <cstdint>
#ifdef _WIN32
# define K2040_SETTINGS_CALL __cdecl
#else
# define K2040_SETTINGS_CALL
#endif

inline constexpr std::uint32_t K2040_SETTINGS_ADAPTER_ABI = 1;
struct K2040SettingsAdapterV1
{
    std::uint32_t size;          // Must equal sizeof(K2040SettingsAdapterV1).
    std::uint32_t abiVersion;    // Exact match required.
    const char* moduleId;       // Stable known identifier, not a display name.
    bool (K2040_SETTINGS_CALL* isAvailable)() noexcept;
    bool (K2040_SETTINGS_CALL* openSettings)() noexcept;
};

// Each participating plugin may export exactly this function when ready.
using K2040GetSettingsAdapterV1 = const K2040SettingsAdapterV1*
    (K2040_SETTINGS_CALL*)() noexcept;
inline constexpr const char* K2040_SETTINGS_EXPORT = "K2040_GetSettingsAdapterV1";
