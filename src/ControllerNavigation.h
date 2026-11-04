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

inline bool ConflictsWithMenuNavigation(std::uint32_t primaryMask,
                                        std::uint32_t heldMask,
                                        bool journalFocused) noexcept {
    constexpr std::uint32_t menuButtons=0x300fu;
    return journalFocused&&((primaryMask|heldMask)&menuButtons)!=0;
}

class Sampler {
public:
    using Clock=std::chrono::steady_clock;
    using TimePoint=Clock::time_point;

    Action Update(Input input,bool focused,TimePoint now) noexcept {
        const bool a=(input.buttons&0x1000u)!=0;
        const bool b=(input.buttons&0x2000u)!=0;
        const auto heldDirection=ReadDirection(input);
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
    static Action ReadDirection(Input input) noexcept {
        // Prefer the digital pad when it is held, avoiding conflicts with stick drift.
        if(input.buttons&0x0001u)return Action::Up;
        if(input.buttons&0x0002u)return Action::Down;
        if(input.buttons&0x0004u)return Action::Left;
        if(input.buttons&0x0008u)return Action::Right;

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
};
} // namespace K2040::ControllerNavigation
