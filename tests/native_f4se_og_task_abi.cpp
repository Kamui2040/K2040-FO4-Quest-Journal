#include "../research/f4se_og_api/TaskInterface0623.h"
#include <cassert>
#include <cstddef>
using namespace K2040::OGF4SE::ABI0623;
int main() {
    static_assert(kTaskInterfaceVersion == 2);
    static_assert(kSlotCount == 2);
    static_assert(sizeof(TaskInterface) == offsetof(TaskInterface, addUITask) +
                  sizeof(TaskInterface::addUITask));
    // There is no valid third member at the end of the OG ABI struct.
    // CommonLib's AddTaskPermanent would read from this out-of-bounds slot.
    assert(kSlotCount == 2);
}
