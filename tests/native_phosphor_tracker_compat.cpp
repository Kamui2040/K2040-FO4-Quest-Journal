#include "TrackerCompatibility.h"

using K2040::TrackerCompatibility::ShowMiniTracker;

// Host-only test: no Fallout runtime, F4SE or Phosphor code is needed.
static_assert(ShowMiniTracker(true, false));  // no conflicting HUD
static_assert(!ShowMiniTracker(true, true));  // Phosphor takes priority
static_assert(!ShowMiniTracker(false, false)); // user's own preference wins
static_assert(!ShowMiniTracker(false, true));

int main() { return 0; }
