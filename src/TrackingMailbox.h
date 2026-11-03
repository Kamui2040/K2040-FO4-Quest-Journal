#pragma once

// Source-only handshake prototype. NO engine, F4SE, Papyrus or Prisma calls.
// The production Journal -> Pip-Boy action remains disabled.
// Do not connect this class to gameplay without the approval and runtime
// validation gates in docs/ENGINE_TRACKING_RESEARCH.md.

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>

namespace K2040::QuestTracking {
    struct PendingTracking {
        std::uint32_t ticket;
        std::uint64_t session;
        std::uint32_t formId;
        bool desiredTracked;
    };

    enum class TrackingResult { kInvalid, kConfirmed, kMismatch };

    class TrackingMailbox {
    public:
        // Caller must provide a unique, nonzero save/view generation.
        void Reset(std::uint64_t session) {
            std::lock_guard lock(mutex_);
            session_ = session;
            pending_.reset();
            claimed_ = false;
        }

        [[nodiscard]] std::optional<std::uint32_t> Offer(
            std::uint64_t session, std::uint32_t formId, bool desired) {
            std::lock_guard lock(mutex_);
            if (!session || session != session_ || !formId || pending_ ||
                nextTicket_ >= static_cast<std::uint32_t>(
                    std::numeric_limits<std::int32_t>::max()))
                return std::nullopt;
            const auto ticket = ++nextTicket_;
            pending_ = PendingTracking{ticket, session, formId, desired};
            return ticket;
        }

        // Acquiring only transfers a request; it does not alter quest state.
        [[nodiscard]] std::optional<PendingTracking> Acquire(std::uint64_t session) {
            std::lock_guard lock(mutex_);
            if (!session || session != session_ || !pending_ || claimed_)
                return std::nullopt;
            claimed_ = true;
            return pending_;
        }

        // Recheck immediately before a script ever attempts a native Quest method.
        [[nodiscard]] bool IsCurrent(std::uint64_t session, std::uint32_t ticket) const {
            std::lock_guard lock(mutex_);
            return ValidClaim(session, ticket);
        }

        // Reporting != tracking success: require the observed game state to match.
        [[nodiscard]] TrackingResult Settle(
            std::uint64_t session, std::uint32_t ticket, bool observedTracked) {
            std::lock_guard lock(mutex_);
            if (!ValidClaim(session, ticket)) return TrackingResult::kInvalid;
            const auto result = pending_->desiredTracked == observedTracked
                ? TrackingResult::kConfirmed : TrackingResult::kMismatch;
            pending_.reset();
            claimed_ = false;
            return result;
        }

        [[nodiscard]] bool Cancel(std::uint64_t session, std::uint32_t ticket) {
            std::lock_guard lock(mutex_);
            if (!ValidClaim(session, ticket)) return false;
            pending_.reset();
            claimed_ = false;
            return true;
        }

    private:
        [[nodiscard]] bool ValidClaim(
            std::uint64_t session, std::uint32_t ticket) const noexcept {
            return session && session == session_ && claimed_ && pending_ &&
                   pending_->ticket == ticket && pending_->session == session;
        }

        mutable std::mutex mutex_;
        std::uint64_t session_{};
        std::uint32_t nextTicket_{};
        std::optional<PendingTracking> pending_;
        bool claimed_{false};
    };
} // namespace K2040::QuestTracking
