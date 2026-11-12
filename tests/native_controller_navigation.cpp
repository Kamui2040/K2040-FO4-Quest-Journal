#include "../src/ControllerNavigation.h"

#include <cassert>
#include <chrono>

using K2040::ControllerNavigation::Action;
using K2040::ControllerNavigation::ChordLatch;
using K2040::ControllerNavigation::ConflictsWithMenuNavigation;
using K2040::ControllerNavigation::Input;
using K2040::ControllerNavigation::MenuBindings;
using K2040::ControllerNavigation::Sampler;
using Clock=std::chrono::steady_clock;
using namespace std::chrono_literals;

int main() {
    assert(!ConflictsWithMenuNavigation(0x1000,0,false));
    assert(ConflictsWithMenuNavigation(0x1000,0,true));
    assert(ConflictsWithMenuNavigation(0x0100,0x0004,true));
    assert(!ConflictsWithMenuNavigation(0x0100,0,true));

    // A gamepad chord must fire exactly once when its last button is pressed,
    // irrespective of whether the modifier or the primary button came first.
    constexpr std::uint32_t primary=0x4000u, held=0x0100u;
    ChordLatch primaryFirst;
    assert(!primaryFirst.Update(primary,primary,held));
    assert(primaryFirst.Update(primary|held,primary,held));
    assert(!primaryFirst.Update(primary|held,primary,held));
    assert(!primaryFirst.Update(primary,primary,held));
    assert(primaryFirst.Update(primary|held,primary,held));
    assert(!primaryFirst.Update(held,primary,held));
    assert(primaryFirst.Update(primary|held,primary,held));

    ChordLatch modifierFirst;
    assert(!modifierFirst.Update(held,primary,held));
    assert(modifierFirst.Update(primary|held,primary,held));
    assert(!modifierFirst.Update(primary|held,primary,held));
    assert(!modifierFirst.Update(0,primary,held));

    ChordLatch rebound;
    rebound.Reset(primary|held,primary,held);
    assert(!rebound.Update(primary|held,primary,held));
    assert(!rebound.Update(0,primary,held));
    assert(rebound.Update(primary|held,primary,held));
    assert(!rebound.Update(primary|held,0,held));
    assert(!rebound.Update(0,0,held));

    ChordLatch singleButton;
    assert(singleButton.Update(primary,primary,0));
    assert(!singleButton.Update(primary,primary,0));
    assert(!singleButton.Update(0,primary,0));
    assert(singleButton.Update(primary,primary,0));

    // Triggers occupy additional bits beyond XInput's 16 digital buttons.
    ChordLatch trigger;
    assert(!trigger.Update(1u<<16,1u<<17,1u<<16));
    assert(trigger.Update((1u<<16)|(1u<<17),1u<<17,1u<<16));

    Sampler sampler;
    const auto t=Clock::time_point{};
    const auto sample=[](std::uint16_t buttons=0,std::int16_t x=0,std::int16_t y=0) {
        return Input{buttons,x,y};
    };

    assert(sampler.Update(sample(),true,t)==Action::None);
    assert(sampler.Update(sample(0x0001,20000,0),true,t+1ms)==Action::Up);
    assert(sampler.Update(sample(0x0001),true,t+349ms)==Action::None);
    assert(sampler.Update(sample(0x0001),true,t+351ms)==Action::Up);
    assert(sampler.Update(sample(0x0001),true,t+471ms)==Action::Up);
    assert(sampler.Update(sample(),true,t+480ms)==Action::None);
    assert(sampler.Update(sample(0x0004),true,t+481ms)==Action::Left);

    Sampler stick;
    assert(stick.Update(sample(0,7000,0),true,t)==Action::None);
    assert(stick.Update(sample(0,12000,0),true,t+1ms)==Action::Right);
    assert(stick.Update(sample(0,20000,21000),true,t+2ms)==Action::Down);
    assert(stick.Update(sample(),true,t+2500us)==Action::None);
    assert(stick.Update(sample(0,21000,21000),true,t+3ms)==Action::Down);

    Sampler buttons;
    assert(buttons.Update(sample(0x1000),true,t)==Action::Activate);
    assert(buttons.Update(sample(0x1000),true,t+1ms)==Action::None);
    assert(buttons.Update(sample(),true,t+2ms)==Action::None);
    assert(buttons.Update(sample(0x1000),true,t+3ms)==Action::Activate);
    assert(buttons.Update(sample(0x2000),true,t+4ms)==Action::Back);

    // Native menu mappings, not Xbox face-button positions, choose actions.
    constexpr MenuBindings defaults{};
    static_assert(MenuBindings::SupportedButton(0x4000u,0x1000u)==0x4000u);
    static_assert(MenuBindings::SupportedButton(0xFFu,0x1000u)==0x1000u);
    static_assert(MenuBindings::SupportedButton(0x0400u,0x1000u)==0x1000u);
    static_assert(MenuBindings::SupportedButton(0x3000u,0x1000u)==0x1000u);
    static_assert(MenuBindings::SupportedButton(0x10000u,0x1000u)==0x1000u);
    static_assert(MenuBindings::SupportedButton(0xFFFFFFFFu,0x1000u)==0x1000u);
    static_assert(defaults.Uses(0x1000u));
    static_assert(defaults.Uses(0x2000u));
    static_assert(!defaults.Uses(0x4000u));

    MenuBindings remapped;
    remapped.accept=0x4000u; // mapped confirm (often Nintendo's physical X)
    remapped.cancel=0x8000u;
    remapped.up=0x0100u;     // remapped directional button
    assert(remapped.Uses(0x4000u));
    assert(remapped.Uses(0x8000u));
    assert(ConflictsWithMenuNavigation(0x4000u,0,true,remapped));
    assert(ConflictsWithMenuNavigation(0,0x0100u,true,remapped));
    assert(!ConflictsWithMenuNavigation(0x1000u,0,true,remapped));
    assert(!ConflictsWithMenuNavigation(0x4000u,0,false,remapped));
    Sampler mapped;
    assert(mapped.Update(sample(),true,t,remapped)==Action::None);
    assert(mapped.Update(sample(0x1000),true,t+1ms,remapped)==Action::None);
    assert(mapped.Update(sample(0x4000),true,t+2ms,remapped)==Action::Activate);
    assert(mapped.Update(sample(0x4000),true,t+3ms,remapped)==Action::None);
    assert(mapped.Update(sample(),true,t+4ms,remapped)==Action::None);
    assert(mapped.Update(sample(0x8000),true,t+5ms,remapped)==Action::Back);
    assert(mapped.Update(sample(),true,t+6ms,remapped)==Action::None);
    assert(mapped.Update(sample(0x0001),true,t+7ms,remapped)==Action::None);
    assert(mapped.Update(sample(0x0100),true,t+8ms,remapped)==Action::Up);

    // Rebinding while a button is held cannot produce a phantom click.
    Sampler rebind;
    assert(rebind.Update(sample(),true,t)==Action::None);
    assert(rebind.Update(sample(0x4000),true,t+1ms,remapped)==Action::None);
    assert(rebind.Update(sample(0x4000),true,t+2ms,remapped)==Action::None);
    assert(rebind.Update(sample(),true,t+3ms,remapped)==Action::None);
    assert(rebind.Update(sample(0x4000),true,t+4ms,remapped)==Action::Activate);

    // Mapping confirm to a directional bit must not move focus as well.
    MenuBindings shared=remapped;
    shared.accept=0x0001u;
    shared.up=0x0001u;
    Sampler overlapping;
    assert(overlapping.Update(sample(),true,t,shared)==Action::None);
    assert(overlapping.Update(sample(0x0001),true,t+1ms,shared)==Action::Activate);
    assert(overlapping.Update(sample(0x0001),true,t+500ms,shared)==Action::None);

    Sampler focus;
    assert(focus.Update(sample(0x1000,20000,0),false,t)==Action::None);
    assert(focus.Update(sample(0x1000,20000,0),true,t+1ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),true,t+2ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),false,t+3ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),true,t+4ms)==Action::None);
    assert(focus.Update(sample(),true,t+5ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),true,t+6ms)==Action::Right);
}
