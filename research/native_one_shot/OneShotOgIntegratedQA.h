#pragma once
// Explicit in-game opt-in, TEST ONLY. No INI consent files.
// Bounded native SetActive on the already game-tested one-shot AddTask path.
#include "OneShotOgDryRun.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
namespace K2040::OneShotResearch {
    inline constexpr std::uint32_t kIntegratedMaxAttempts = 8;
    [[nodiscard]] inline bool ReserveIntegratedAttempt(
        std::atomic_uint32_t& used) noexcept {
        auto value=used.load(std::memory_order_acquire);
        while (value<kIntegratedMaxAttempts) {
            if (used.compare_exchange_weak(value,value+1,
                    std::memory_order_acq_rel,std::memory_order_acquire))
                return true;
        }
        return false;
    }
    enum class IntegratedResult : std::uint8_t {
        kRejected, kAlready, kUnverified, kConfirmed, kProvisional
    };
    [[nodiscard]] bool QueueIntegratedTracking(
        const std::shared_ptr<Gate>& gate,
        const char* borrowedUiPayload,
        std::uint64_t saveEpoch, std::uint64_t uiEpoch,
        std::atomic_bool& userEnabled,
        std::atomic_uint32_t& processAttempts,
        ReadContext readFresh,
        std::function<void(IntegratedResult)> report);
}
