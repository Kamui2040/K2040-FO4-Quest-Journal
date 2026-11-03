#pragma once
// Minimal independent ABI MODEL (not a copy of F4SE source).
// Verified from official F4SE tag v0.6.23 (commit ea143b59):
// f4se/PluginAPI.h and f4se/PluginManager.cpp.
// Task interface v2 provides TWO function-pointer slots ONLY.
// WARNING: pinned CommonLibF4's version-2 wrapper erroneously assumes
// a third AddTaskPermanent pointer exists; NEVER access that slot.
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace K2040::OGF4SE::ABI0623 {
    struct TaskInterface {
        std::uint32_t interfaceVersion;
        void (*addTask)(void*);
        void (*addUITask)(void*);
    };
    static_assert(std::is_standard_layout_v<TaskInterface>);
    static_assert(offsetof(TaskInterface, interfaceVersion) == 0);
#ifdef _WIN64
    static_assert(offsetof(TaskInterface, addTask) == 8);
    static_assert(offsetof(TaskInterface, addUITask) == 16);
    static_assert(sizeof(TaskInterface) == 24);
#endif
    inline constexpr std::uint32_t kTaskInterfaceVersion = 2;
    inline constexpr std::size_t kSlotCount = 2;
}
