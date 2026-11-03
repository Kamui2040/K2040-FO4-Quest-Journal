#pragma once
// Engine-independent lifecycle for isolated QA and an explicitly opt-in
// normal-UI candidate; no RE/F4SE/Prisma calls, writes or timers.
#include "QuestWideScopePolicy.h"
#include <cstdint>
#include <mutex>
#include <optional>
namespace K2040::RepeatableQuestWide {
using K2040::OneShotResearch::Intent;
using K2040::OneShotResearch::Observation;
using K2040::OneShotResearch::GenericMiscEvidence;
using K2040::OneShotResearch::Result;
enum class Phase : std::uint8_t {
    kIdle, kQueued, kExecuting, kAwaitingRefresh, kBlocked
};
struct Ticket {
    std::uint64_t sequence{};
    Intent intent{};
    std::uint32_t instance{};
    std::uint32_t displayedCount{};

    std::uint32_t miscRows{};
    // Captured before the engine call; a postwrite type change is not a
    // valid refresh, even if a feed happens to contain the same FormID.
    std::uint8_t questType{};
    std::uint32_t ordinaryRows{};
};
struct Status {
    Phase phase{Phase::kIdle};
    std::optional<Ticket> pending{};
};
class Controller {
public:
    void BeginSession(std::uint64_t save, std::uint64_t view) {
        std::scoped_lock lock(mutex_);
        saveEpoch_=save; viewEpoch_=view;
        if (phase_==Phase::kQueued) { pending_.reset();phase_=Phase::kIdle; }
        else if (phase_!=Phase::kIdle) phase_=Phase::kBlocked;
        // An executing or already-written operation is not automatically
        // rearmed by a save/load, menu close or panel recreation.
    }
    [[nodiscard]] std::optional<Ticket> Offer(
        const Intent& intent, const Observation& o,
        std::optional<std::uint32_t> instance,
        const GenericMiscEvidence& evidence) {
        std::scoped_lock lock(mutex_);
        if (phase_!=Phase::kIdle || sequence_==UINT64_MAX ||
            intent.saveEpoch!=saveEpoch_ || intent.uiEpoch!=viewEpoch_ ||
            K2040::OneShotResearch::ReviewQuestWide(
                intent,o,instance,evidence)!=Result::kWouldWrite)
            return std::nullopt;

        Ticket t{++sequence_,intent,*instance,
                 evidence.nativeDisplayedCount,evidence.miscFeedRows,
                 o.questType,evidence.ordinaryFeedRows};
        pending_=t;phase_=Phase::kQueued;
        return t;
    }
    [[nodiscard]] bool Claim(const Ticket& ticket,
            const Observation& o, const GenericMiscEvidence& evidence) {
        std::scoped_lock lock(mutex_);
        if (phase_!=Phase::kQueued || !Matches(ticket) ||
            K2040::OneShotResearch::ReviewQuestWide(
                ticket.intent,o,ticket.instance,evidence)!=Result::kWouldWrite ||
            evidence.nativeDisplayedCount!=ticket.displayedCount ||

            evidence.miscFeedRows!=ticket.miscRows ||
            evidence.ordinaryFeedRows!=ticket.ordinaryRows ||
            o.questType!=ticket.questType)
            return false;
        phase_=Phase::kExecuting;
        return true;
    }
    // An invalidated claim that remains Queued has never reached a native
    // setter. Discard exactly that ticket without latching a false global
    // block; an already Executing/AwaitingRefresh write cannot be canceled.
    [[nodiscard]] bool CancelQueuedBeforeWrite(const Ticket& ticket) {
        std::scoped_lock lock(mutex_);
        if(phase_!=Phase::kQueued || !Matches(ticket))return false;
        pending_.reset();phase_=Phase::kIdle;return true;
    }
    // Must run BEFORE the first native mutation: only a proven non-call
    // can settle back to Idle. Once a write might have happened, block.
    [[nodiscard]] bool RefuseBeforeWrite(const Ticket& ticket) {
        std::scoped_lock lock(mutex_);
        if (!Matches(ticket) || phase_!=Phase::kExecuting)return false;
        pending_.reset();phase_=Phase::kIdle;return true;
    }
    [[nodiscard]] bool MarkWriteReturned(const Ticket& ticket) {
        std::scoped_lock lock(mutex_);
        if (!Matches(ticket) || phase_!=Phase::kExecuting)return false;
        refreshAttempts_=0;phase_=Phase::kAwaitingRefresh;return true;
    }
    // Called only after a fresh engine/Pip-Boy refresh on the game thread.
    // Nonmatching evidence must NEVER unlock another potentially dangerous
    // action. A different save or instance latches a process-local block.
    [[nodiscard]] bool ConfirmRefreshed(const Ticket& ticket,
            const Observation& after, const GenericMiscEvidence& evidence) {
        std::scoped_lock lock(mutex_);
        if (phase_!=Phase::kAwaitingRefresh || !Matches(ticket))return false;
        // A changed session, selected quest or instance is a hard block.
        // Only the SAME quest's temporarily outdated tracked-bit/feed
        // readback may be observed again; no additional game mutation.
        if (after.saveEpoch!=saveEpoch_ || after.uiEpoch!=viewEpoch_ ||
            !after.loaded || !after.og163 || !after.onF4seTask ||
            !after.journalOpenFocused || !after.previouslyObserved ||
            !(after.questFlags & K2040::OneShotResearch::kEnabled) ||
            (after.questFlags & (K2040::OneShotResearch::kCompleted |
                                 K2040::OneShotResearch::kFailed)) ||
            after.selectedFormId!=ticket.intent.formId ||
            after.resolvedFormId!=ticket.intent.formId ||

            after.questType!=ticket.questType ||
            !evidence.nativeComplete || !evidence.onlyCurrentInstance ||

            (ticket.questType==K2040::OneShotResearch::kMisc
                ? !evidence.selectedIsMisc || evidence.selectedIsOrdinary ||
                  evidence.ordinaryFeedRows!=0u
                : !evidence.selectedIsOrdinary || evidence.selectedIsMisc ||
                  !after.inNormalPipboyFeed || !after.feedHasTrackedBit ||
                  evidence.ordinaryFeedRows!=1u) ||
            evidence.questInstanceID!=ticket.instance ||
            evidence.nativeDisplayedCount!=ticket.displayedCount ||

            evidence.miscFeedRows!=ticket.miscRows ||
            evidence.ordinaryFeedRows!=ticket.ordinaryRows) {
            phase_=Phase::kBlocked;
            return false;
        }
        const bool native=(after.questFlags&K2040::OneShotResearch::kTracked)!=0;
        if (!evidence.pipboyFeedComplete || !evidence.feedAgreesWithQuest ||
            (ticket.questType!=K2040::OneShotResearch::kMisc &&
             (!after.inNormalPipboyFeed || !after.feedHasTrackedBit ||
              after.feedTracked!=native)) ||
            native!=ticket.intent.desiredTracked) {
            if (++refreshAttempts_>=kMaxReadbackAttempts)
                phase_=Phase::kBlocked;
            return false;
        }
        refreshAttempts_=0;phase_=Phase::kIdle;pending_.reset();return true;
    }
    void BlockUncertain(const Ticket& ticket) {
        std::scoped_lock lock(mutex_);
        if (Matches(ticket)) phase_=Phase::kBlocked;
    }
    [[nodiscard]] Status Get() const {
        std::scoped_lock lock(mutex_);
        return {phase_,pending_};
    }
private:
    [[nodiscard]] bool Matches(const Ticket& t) const {
        if (!pending_ || phase_==Phase::kBlocked)return false;
        const auto& a=*pending_;
        return a.sequence==t.sequence && a.intent.ticket==t.intent.ticket &&
               a.intent.formId==t.intent.formId &&
               a.intent.desiredTracked==t.intent.desiredTracked &&
               a.intent.saveEpoch==t.intent.saveEpoch &&
               a.intent.uiEpoch==t.intent.uiEpoch &&
               a.instance==t.instance &&
               saveEpoch_==t.intent.saveEpoch && viewEpoch_==t.intent.uiEpoch;
    }
    static constexpr std::uint8_t kMaxReadbackAttempts=3;
    mutable std::mutex mutex_{};
    std::uint8_t refreshAttempts_{};
    std::uint64_t saveEpoch_{},viewEpoch_{},sequence_{};
    Phase phase_{Phase::kIdle};
    std::optional<Ticket> pending_{};
};
} // namespace K2040::RepeatableQuestWide
