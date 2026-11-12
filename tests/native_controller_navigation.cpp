#include "../src/ControllerNavigation.h"

#include <cassert>
#include <chrono>

using K2040::ControllerNavigation::Action;
using K2040::ControllerNavigation::ChordLatch;
using K2040::ControllerNavigation::ConflictsWithMenuNavigation;
using K2040::ControllerNavigation::Input;
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

    Sampler focus;
    assert(focus.Update(sample(0x1000,20000,0),false,t)==Action::None);
    assert(focus.Update(sample(0x1000,20000,0),true,t+1ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),true,t+2ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),false,t+3ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),true,t+4ms)==Action::None);
    assert(focus.Update(sample(),true,t+5ms)==Action::None);
    assert(focus.Update(sample(0,20000,0),true,t+6ms)==Action::Right);
}
