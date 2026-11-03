#include "TrackingPapyrusWire.h"
#include <cassert>
#include <cstdint>
#include <limits>

using namespace K2040::QuestTracking;

int main() {
    static_assert(kMaxPapyrusTicket == 0x7fffffffu);
    for (std::uint32_t form : {0x00000001u, 0x7fffffffu,
                               0x80000000u, 0xff123456u, 0xffffffffu}) {
        const auto packet = ToPapyrus(PendingTracking{37, 900, form, true});
        assert(packet[0] == 37 && packet[2] == 1);
        assert(FromPapyrusFormId(packet[1]) == form);
    }
    const auto untrack = ToPapyrus(PendingTracking{38, 900, 0xff123456u, false});
    assert(untrack[0] == 38 && untrack[2] == 0);
    assert(untrack[1] < 0);
    TrackingMailbox box;
    box.Reset(900);
    auto ticket = box.Offer(900, 0xff123456u, true);
    assert(ticket);
    auto claim = box.Acquire(900);
    assert(claim);
    auto wire = ToPapyrus(*claim);
    assert(wire[0] > 0 && FromPapyrusFormId(wire[1]) == 0xff123456u);
    assert(box.Settle(900, *ticket, true) == TrackingResult::kConfirmed);
}
