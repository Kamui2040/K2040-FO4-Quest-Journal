#pragma once
// Optional hub request API V1: call on the Fallout 4 game thread only.
// A successful call queues the hub opening after the owning panel releases
// Prisma focus. This does not transfer ownership of any settings or MCM state.
#ifdef _WIN32
# define K2040_REQUEST_MODULE_SETTINGS_CALL __cdecl
#else
# define K2040_REQUEST_MODULE_SETTINGS_CALL
#endif
using K2040RequestModuleSettingsV1 = bool
    (K2040_REQUEST_MODULE_SETTINGS_CALL*)(const char* moduleId) noexcept;
inline constexpr const char* K2040_REQUEST_MODULE_SETTINGS_V1_EXPORT =
    "K2040_RequestModuleSettingsV1";
