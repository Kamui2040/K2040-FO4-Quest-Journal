#include "TrackingMailbox.h"
#include <cassert>
#include <cstdint>
#include <thread>
#include <vector>

using K2040::QuestTracking::TrackingMailbox;
using K2040::QuestTracking::TrackingResult;

int main() {
    TrackingMailbox mailbox;
    assert(!mailbox.Offer(1, 0x1234, true));
    assert(!mailbox.Acquire(1));
    mailbox.Reset(1);
    assert(!mailbox.Offer(1, 0, true));
    assert(!mailbox.Offer(0, 0x1234, true));
    assert(!mailbox.Offer(2, 0x1234, true));

    auto first = mailbox.Offer(1, 0x1234, true);
    assert(first && *first != 0);
    assert(!mailbox.Offer(1, 0x5678, false));
    assert(!mailbox.Acquire(2));
    auto request = mailbox.Acquire(1);
    assert(request && request->ticket == *first);
    assert(request->formId == 0x1234 && request->desiredTracked);
    assert(!mailbox.Acquire(1)); // Single consumer.
    assert(mailbox.IsCurrent(1, *first));
    assert(!mailbox.IsCurrent(1, *first + 1));
    assert(!mailbox.Cancel(1, *first + 1));
    assert(mailbox.Settle(1, *first + 1, true) == TrackingResult::kInvalid);
    assert(mailbox.Settle(1, *first, false) == TrackingResult::kMismatch);
    assert(!mailbox.IsCurrent(1, *first));
    assert(mailbox.Settle(1, *first, true) == TrackingResult::kInvalid);

    auto second = mailbox.Offer(1, 0x5678, false);
    assert(second && *second > *first);
    assert(mailbox.Acquire(1));
    assert(mailbox.Settle(1, *second, false) == TrackingResult::kConfirmed);
    assert(!mailbox.Acquire(1));

    auto stale = mailbox.Offer(1, 0x99, true);
    assert(stale && mailbox.Acquire(1));
    mailbox.Reset(2); // Save/load invalidates any Papyrus-side in-flight request.
    assert(!mailbox.IsCurrent(1, *stale));
    assert(!mailbox.Cancel(1, *stale));
    assert(mailbox.Settle(1, *stale, true) == TrackingResult::kInvalid);
    assert(!mailbox.Acquire(1));
    auto current = mailbox.Offer(2, 0x77, true);
    assert(current && *current > *stale);
    assert(mailbox.Acquire(2));
    assert(mailbox.Cancel(2, *current));
    assert(!mailbox.Cancel(2, *current));

    mailbox.Reset(0); // No active save.
    assert(!mailbox.Offer(0, 0x1234, true));
    mailbox.Reset(3);
    // Lock-protected queue: concurrent callers must not win more than one ticket.
    constexpr int workers = 10;
    std::vector<std::thread> threads;
    std::vector<std::uint32_t> tickets(workers);
    for (int i = 0; i < workers; ++i) {
        threads.emplace_back([&mailbox, &tickets, i] {
            tickets[i] = mailbox.Offer(3, 0x2000u + i, true).value_or(0);
        });
    }
    for (auto& worker : threads) worker.join();
    int accepted = 0;
    for (auto ticket : tickets) accepted += ticket != 0;
    assert(accepted == 1);
    const auto race = mailbox.Acquire(3);
    assert(race);
    assert(mailbox.Cancel(3, race->ticket));
}
