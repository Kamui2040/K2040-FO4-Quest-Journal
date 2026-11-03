#pragma once
// Independently compiled Dogmeat-only candidate, not the QA writer backend.
#if !defined(K2040_DOGMEAT_PRODUCTION_CANDIDATE)
#error The tested Dogmeat candidate must be selected explicitly
#endif
#include <cstdint>
#include <functional>
namespace K2040::DogmeatProduction {
    enum class Result : std::uint8_t { kRejected, kAlready, kProvisional };
    // Caller: F4SE game thread, selected/observed FormID + current
    // save/view generations + engine version and task interface checked.
    // The callback revalidates session state immediately before the call.
    [[nodiscard]] Result ExecuteVerifiedToggle(
        std::uint32_t formID, bool desiredTracked,
        const std::function<bool()>& validSession);
}
