#include "TrackerCompatibility.h"

using K2040::TrackerCompatibility::ShowMiniTracker;

// Pure host-only policy matrix: preserve the saved tracker setting, and
// override Phosphor detection only when explicitly enabled by the user.
static_assert(ShowMiniTracker(true, false, false));
static_assert(ShowMiniTracker(true, false, true));
static_assert(!ShowMiniTracker(true, true, false)); // safe default auto-hide
static_assert(ShowMiniTracker(true, true, true));   // user opts into our tracker
static_assert(!ShowMiniTracker(false, false, false));
static_assert(!ShowMiniTracker(false, false, true));
static_assert(!ShowMiniTracker(false, true, false));
static_assert(!ShowMiniTracker(false, true, true)); // disabled stays disabled

int main() { return 0; }
