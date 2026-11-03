#pragma once
#include <atomic>
#include <cstdint>

// Thread-safe generation token. A queued F4SE task may not outlive the
// Prisma panel/save generation that originated it.
namespace K2040 {
class UILifecycleEpoch final {
public:
    std::uint64_t Capture() const noexcept {
        return epoch_.load(std::memory_order_acquire);
    }
    bool IsCurrent(std::uint64_t generation) const noexcept {
        return generation == Capture();
    }
    void Invalidate() noexcept {
        epoch_.fetch_add(1, std::memory_order_acq_rel);
    }
private:
    std::atomic<std::uint64_t> epoch_{1};
};
}
