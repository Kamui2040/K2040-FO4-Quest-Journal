// Isolated native-to-Prisma settings payload test. No game assets or live F4SE.
#include "JournalSettings.h"
#include <iostream>

int main() {
    using namespace K2040::Settings;
    std::cout << Serialize() << '\n';

    if (!Change("theme:contrast") ||
        !Change("trackerAnchor:bottom-right") ||
        !Change("questListWidth:430") ||
        !Change("trackerWidth:260") ||
        !Change("opacity:71") ||
        !Change("trackerEnabled:false") ||
        !Change("trackerShowCompleted:true"))
        return 1;
    std::cout << Serialize() << '\n';

    if (!Change("theme:custom") ||
        !Change("accent:#a1b2c3") ||
        !Change("trackerAnchor:bottom-left"))
        return 1;
    std::cout << Serialize() << '\n';

    // The Pip-Boy palette is engine-derived and not part of the saved INI.
    if (!Change("theme:pipboy")) return 1;
    std::cout << Serialize("#ff7733", "#0a0011", "#b34299") << '\n';

    // Independent journal appearance must coexist with the tracker theme.
    if (!Change("journalTheme:custom") ||
        !Change("journalAccent:#b12345") ||
        !Change("journalText:#eeeeaa") ||
        !Change("journalPanel:#111927") ||
        !Change("journalCompleted:#a2e377") ||
        !Change("journalOpacity:79") ||
        !Change("journalFontSize:125")) return 1;
    std::cout << Serialize("#ff7733", "#0a0011") << '\n';

    // Validate accepted and correctly serialized Fallout 76 in both
    // independent theme slots, while preserving the custom colours.
    if (!Change("theme:fallout76") || !Change("journalTheme:fallout76")) return 1;
    std::cout << Serialize("#ff7733", "#0a0011") << '\n';

    if (!Change("trackerAutoPinNew:true") ||
        !Change("trackerSyncPipboy:true")) return 1;
    std::cout << Serialize("#ff7733", "#0a0011") << '\n';
}
