#include "../research/native_pipboy_selection/PipboyDispatchPrefixPolicy.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
using namespace K2040::PipboyDispatchResearch;
int main() {
    static_assert(kPrefixBytes==192);
    static_assert(ValidCodeRva(kExpectedCallRva,kPrefixBytes));
    static_assert(ValidCodeRva(kExpectedMapRva,kPrefixBytes));
    static_assert(ValidSlotRvas(kExpectedCallRva,kExpectedMapRva));
    assert(!ValidSlotRvas(kExpectedMapRva,kExpectedCallRva));
    assert(!ValidSlotRvas(kExpectedCallRva+1,kExpectedMapRva));
    assert(!ValidSlotRvas(kExpectedCallRva,kExpectedMapRva+1));
    assert(!ValidCodeRva(0,kPrefixBytes));
    assert(!ValidCodeRva(kExecutableEnd-10,kPrefixBytes));
    assert(!ValidCodeRva(kExpectedCallRva,257));
    assert(!ValidCodeRva(kExpectedCallRva,0));
    static_assert(!K2040::QuestTracking::CanWritePipboy());
}
