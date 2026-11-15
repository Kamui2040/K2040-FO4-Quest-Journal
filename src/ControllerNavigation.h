#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>

namespace K2040::ControllerNavigation {
enum class Action : std::uint8_t { None, Up, Down, Left, Right, Activate, Back };
struct Input {
    std::uint16_t buttons{};
    std::int16_t leftX{};
    std::int16_t leftY{};
};

// Logical XInput button masks sourced from Fallout 4's menu ControlMap.
// Native menu bindings stay authoritative; these defaults apply only when
// the game mapping cannot be read or is not an ordinary single button.
struct MenuBindings {
    std::uint16_t accept{0x1000u}, cancel{0x2000u};
    std::uint16_t up{0x0001u}, down{0x0002u};
    std::uint16_t left{0x0004u}, right{0x0008u};

    [[nodiscard]] static constexpr std::uint16_t SupportedButton(
        std::uint32_t mapped,std::uint16_t fallback) noexcept {
        constexpr std::uint32_t gamepadButtons=0xF3FFu;
        return mapped && mapped<=0xFFFFu &&
               (mapped & (mapped-1))==0 &&
               (mapped & gamepadButtons)==mapped
            ? static_cast<std::uint16_t>(mapped) : fallback;
    }

    [[nodiscard]] constexpr bool Uses(std::uint16_t button) const noexcept {
        return button && (accept==button || cancel==button ||
                          up==button || down==button ||
                          left==button || right==button);
    }

    bool operator==(const MenuBindings&) const = default;
};

inline bool ConflictsWithMenuNavigation(std::uint32_t primaryMask,
                                        std::uint32_t heldMask,
                                        bool journalFocused,
                                        MenuBindings mapped={}) noexcept {
    const std::uint32_t menuButtons=static_cast<std::uint32_t>(mapped.accept)|
        mapped.cancel|mapped.up|mapped.down|mapped.left|mapped.right;
    return journalFocused&&((primaryMask|heldMask)&menuButtons)!=0;
}

// Detect a transition into the complete chord, regardless of button order.
// Reset from the live state when a binding changes to prevent a phantom press.
class ChordLatch {
public:
    void Reset(std::uint32_t buttons,std::uint32_t primary,
               std::uint32_t held) noexcept {
        wasMatched_=Matches(buttons,primary,held);
    }

    bool Update(std::uint32_t buttons,std::uint32_t primary,
                std::uint32_t held) noexcept {
        const bool matched=Matches(buttons,primary,held);
        const bool pressed=matched&&!wasMatched_;
        wasMatched_=matched;
        return pressed;
    }

private:
    static bool Matches(std::uint32_t buttons,std::uint32_t primary,
                        std::uint32_t held) noexcept {
        return primary&&(buttons&primary)==primary&&
               (!held||(buttons&held)==held);
    }
    bool wasMatched_{};
};

class Sampler {
public:
    using Clock=std::chrono::steady_clock;
    using TimePoint=Clock::time_point;

    Action Update(Input input,bool focused,TimePoint now,
                  MenuBindings mapped={}) noexcept {
        // Rebase on the live state when Fallout 4 mappings change: a held
        // newly assigned button is not a fresh press of the new action.
        if(mapped!=bindings_) {
            bindings_=mapped;
            aWasDown_=Pressed(input.buttons,bindings_.accept);
            bWasDown_=Pressed(input.buttons,bindings_.cancel);
            direction_=Action::None;
            blockedUntilNeutral_=aWasDown_||bWasDown_||
                                 ReadDirection(input,bindings_)!=Action::None;
            return Action::None;
        }
        const bool a=Pressed(input.buttons,bindings_.accept);
        const bool b=Pressed(input.buttons,bindings_.cancel);
        // A remapped confirm/back can share a direction button. It must not
        // also move focus while the player holds the action button.
        const auto heldDirection=(a||b)?Action::None:ReadDirection(input,bindings_);
        if(!focused) {
            blockedUntilNeutral_=a||b||heldDirection!=Action::None;
            aWasDown_=a;
            bWasDown_=b;
            direction_=Action::None;
            return Action::None;
        }
        if(blockedUntilNeutral_) {
            aWasDown_=a;
            bWasDown_=b;
            direction_=Action::None;
            if(!a&&!b&&heldDirection==Action::None)blockedUntilNeutral_=false;
            return Action::None;
        }
        const bool pressA=a&&!aWasDown_;
        const bool pressB=b&&!bWasDown_;
        aWasDown_=a;
        bWasDown_=b;
        if(pressB)return Action::Back;
        if(pressA)return Action::Activate;

        const auto next=heldDirection;
        if(next==Action::None) {
            direction_=Action::None;
            return Action::None;
        }
        if(next!=direction_) {
            direction_=next;
            nextRepeat_=now+std::chrono::milliseconds(350);
            return direction_;
        }
        if(now<nextRepeat_)return Action::None;
        nextRepeat_=now+std::chrono::milliseconds(120);
        return direction_;
    }

private:
    static bool Pressed(std::uint16_t buttons,std::uint16_t mask) noexcept {
        return mask && (buttons&mask)==mask;
    }

    static Action ReadDirection(Input input,const MenuBindings& mapped) noexcept {
        // Use Fallout 4's mapped digital navigation before interpreting the
        // analog left stick; the game still owns Steam Input translation.
        if(Pressed(input.buttons,mapped.up))return Action::Up;
        if(Pressed(input.buttons,mapped.down))return Action::Down;
        if(Pressed(input.buttons,mapped.left))return Action::Left;
        if(Pressed(input.buttons,mapped.right))return Action::Right;

        const auto x=static_cast<std::int32_t>(input.leftX);
        const auto y=static_cast<std::int32_t>(input.leftY);
        constexpr std::int32_t deadZone=7849;
        if(x*x+y*y<=deadZone*deadZone)return Action::None;
        if(std::abs(y)>=std::abs(x))return y>0?Action::Down:Action::Up;
        return x>0?Action::Right:Action::Left;
    }

    Action direction_{Action::None};
    bool aWasDown_{};
    bool bWasDown_{};
    TimePoint nextRepeat_{};
    bool blockedUntilNeutral_{};
    MenuBindings bindings_{};
};
} // namespace K2040::ControllerNavigation
