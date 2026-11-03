#pragma once

// Non-shipping research: isolated, bounded request / game task handshake.
// No game, F4SE, Prisma, Papyrus or engine mutation dependencies.
// Explicit browser request -> ONE queued F4SE AddTask -> fresh observation.
// In particular there are no recurring tasks and no player state writes.

#include <cstdint>
#include <optional>
#include <mutex>
#include <functional>
#include <memory>
#include <utility>

namespace K2040::OneShotResearch {
    struct Intent {
        std::uint32_t ticket{};
        std::uint32_t formId{};
        bool desiredTracked{};
        std::uint64_t saveEpoch{};
        std::uint64_t uiEpoch{};
    };

    enum class Result : std::uint8_t {
        kRejected,       // bad request/stale session/invalid source
        kAlreadyCurrent, // no write necessary
        kWouldWrite,     // eligibility proven, but DRY RUN only
        kUnverified      // unable to read both authoritative states
    };

    struct Observation {
        std::uint32_t resolvedFormId{};
        std::uint32_t selectedFormId{};
        std::uint64_t saveEpoch{};
        std::uint64_t uiEpoch{};
        bool og163{};
        bool onF4seTask{};
        bool loaded{};
        bool journalOpenFocused{};
        bool previouslyObserved{};
        bool inNormalPipboyFeed{};
        bool feedHasTrackedBit{};
        bool feedTracked{};
        std::uint16_t questFlags{};
        std::uint8_t questType{};
    };

    // OG TESQuest flag values independently verified via pinned CommonLibF4.
    inline constexpr std::uint16_t kEnabled = 0x0001u;
    inline constexpr std::uint16_t kCompleted = 0x0002u;
    inline constexpr std::uint16_t kFailed = 0x0040u;
    inline constexpr std::uint16_t kTracked = 0x0800u;
    inline constexpr std::uint8_t kMisc = 6u;

    [[nodiscard]] constexpr Result ReviewOnGameTask(
        const Intent& intent, const Observation& o) noexcept {
        if (!intent.ticket || !intent.formId ||
            !intent.saveEpoch || !intent.uiEpoch ||
            intent.formId != o.resolvedFormId ||
            intent.formId != o.selectedFormId ||
            intent.saveEpoch != o.saveEpoch ||
            intent.uiEpoch != o.uiEpoch ||
            !o.og163 || !o.onF4seTask || !o.loaded ||
            !o.journalOpenFocused || !o.previouslyObserved ||
            !o.inNormalPipboyFeed || o.questType == kMisc ||
            (o.questFlags & kEnabled) == 0 ||
            (o.questFlags & (kCompleted | kFailed)) != 0)
            return Result::kRejected;

        if (!o.feedHasTrackedBit)
            return Result::kUnverified;

        const bool native = (o.questFlags & kTracked) != 0;
        if (native != o.feedTracked)
            return Result::kUnverified;

        return native == intent.desiredTracked
            ? Result::kAlreadyCurrent : Result::kWouldWrite;
    }

    // Concurrency-limited controller for explicit UI actions. It only owns
    // IDs and generation numbers, never pointers to a TESQuest or Prisma view.
    // It rejects a second request until the current task settles.
    class Gate {
    public:
        void BeginSession(std::uint64_t saveEpoch, std::uint64_t uiEpoch) {
            std::lock_guard lock(mutex_);
            saveEpoch_ = saveEpoch;
            uiEpoch_ = uiEpoch;
            pending_.reset();
        }

        [[nodiscard]] std::optional<Intent> Offer(
            std::uint32_t formId, bool desired,
            std::uint64_t saveEpoch, std::uint64_t uiEpoch) {
            std::lock_guard lock(mutex_);
            if (!formId || !saveEpoch || !uiEpoch || pending_ ||
                saveEpoch != saveEpoch_ || uiEpoch != uiEpoch_ ||
                nextTicket_ == UINT32_MAX)
                return std::nullopt;
            Intent item{++nextTicket_, formId, desired, saveEpoch, uiEpoch};
            pending_ = item;
            return item;
        }

        [[nodiscard]] bool Current(const Intent& intent) const {
            std::lock_guard lock(mutex_);
            return Valid(intent);
        }

        // Release only the EXACT pending request; old queued callbacks from
        // a destroyed panel or reloaded game may not settle a new request.
        [[nodiscard]] bool Settle(const Intent& intent) {
            std::lock_guard lock(mutex_);
            if (!Valid(intent)) return false;
            pending_.reset();
            return true;
        }

    private:
        [[nodiscard]] bool Valid(const Intent& intent) const noexcept {
            return pending_.has_value() &&
                saveEpoch_ == intent.saveEpoch &&
                uiEpoch_ == intent.uiEpoch &&
                pending_->ticket == intent.ticket &&
                pending_->formId == intent.formId &&
                pending_->desiredTracked == intent.desiredTracked &&
                pending_->saveEpoch == intent.saveEpoch &&
                pending_->uiEpoch == intent.uiEpoch;
        }
        mutable std::mutex mutex_;
        std::optional<Intent> pending_;
        std::uint64_t saveEpoch_{};
        std::uint64_t uiEpoch_{};
        std::uint32_t nextTicket_{};
    };

    // Shared, injectable one-shot scheduler. The production equivalent
    // must pass F4SE::TaskInterface::AddTask as Submit. Tests inject a
    // fake executor to verify exact call count and canceled tasks.
    // Only Request primitives enter the queue; no native quest pointers.
    template <typename Submit, typename Observe, typename OnResult>
    [[nodiscard]] bool QueueOneShot(
        const std::shared_ptr<Gate>& gate,
        std::uint32_t selectedFormId,
        bool desiredTracked,
        std::uint64_t saveEpoch,
        std::uint64_t uiEpoch,
        Submit&& submit,
        Observe&& readOnTask,
        OnResult&& onResult) {
        if (!gate) return false;
        const auto ticket = gate->Offer(
            selectedFormId, desiredTracked, saveEpoch, uiEpoch);
        if (!ticket) return false;
        std::function<void()> work = [
            gate, request=*ticket,
            observe=std::forward<Observe>(readOnTask),
            result=std::forward<OnResult>(onResult)]() mutable {
            if (!gate->Current(request)) return;
            const auto current=observe(request);
            if (!gate->Current(request)) return;
            const auto decision=ReviewOnGameTask(request,current);
            if (gate->Settle(request)) result(decision);
        };
        if (!submit(std::move(work))) {
            // In particular, if the F4SE task interface is missing,
            // never leave a pending request blocking later actions.
            (void)gate->Settle(*ticket);
            return false;
        }
        return true;
    }

    // A mutation returns only a *provisional* result; it is never
    // "confirmed" until a subsequent authoritative Pip-Boy refresh.
    enum class Verification : std::uint8_t {
        kUnverified,
        kMismatch,
        kConfirmed
    };

    [[nodiscard]] constexpr Verification ConfirmAfterRefresh(
        const Intent& intent, const Observation& fresh) noexcept {
        if (!intent.formId || !intent.saveEpoch || !intent.uiEpoch ||
            intent.formId != fresh.resolvedFormId ||
            intent.saveEpoch != fresh.saveEpoch ||
            intent.uiEpoch != fresh.uiEpoch ||
            !fresh.loaded || !fresh.inNormalPipboyFeed ||
            !fresh.feedHasTrackedBit)
            return Verification::kUnverified;
        if (bool(fresh.questFlags & kTracked) != fresh.feedTracked)
            return Verification::kUnverified;
        return fresh.feedTracked == intent.desiredTracked
            ? Verification::kConfirmed : Verification::kMismatch;
    }

    static_assert(ReviewOnGameTask({}, {}) == Result::kRejected);
    static_assert(ConfirmAfterRefresh({}, {}) == Verification::kUnverified);
}
