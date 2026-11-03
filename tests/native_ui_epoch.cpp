#include "UILifecycleEpoch.h"
#include <cassert>
#include <vector>

int main() {
    K2040::UILifecycleEpoch guard;
    const auto first = guard.Capture();
    assert(guard.IsCurrent(first));
    std::vector<int> completed;
    auto runIfCurrent = [&](std::uint64_t epoch, int id) {
        if (guard.IsCurrent(epoch)) completed.push_back(id);
    };
    // Emulate delayed F4SE execution from browser callbacks.
    runIfCurrent(first, 1);
    guard.Invalidate();  // Panel closed or transitioned to another save.
    runIfCurrent(first, 2);
    const auto second = guard.Capture();
    runIfCurrent(second, 3);
    guard.Invalidate();  // Newly opened journal takes over.
    runIfCurrent(second, 4);
    assert((completed == std::vector<int>{1,3}));
    assert(!guard.IsCurrent(first));
    assert(!guard.IsCurrent(second));

    // Journal toggles never invalidate a tracker DOM-ready task.
    K2040::UILifecycleEpoch journal;
    K2040::UILifecycleEpoch tracker;
    const auto pendingTracker=tracker.Capture();
    const auto pendingJournal=journal.Capture();
    journal.Invalidate(); // Open a full-screen journal while tracker loads.
    assert(!journal.IsCurrent(pendingJournal));
    assert(tracker.IsCurrent(pendingTracker));
    journal.Invalidate(); // Close it before tracker dispatch.
    assert(tracker.IsCurrent(pendingTracker));
    tracker.Invalidate(); // Save transition destroys both views.
    assert(!tracker.IsCurrent(pendingTracker));
}
