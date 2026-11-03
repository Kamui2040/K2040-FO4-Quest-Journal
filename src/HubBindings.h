#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace K2040::Hub {
enum class Action { Journal, Tracker };
struct Keyboard {
 bool installed{}, registered{}, assigned{}, conflict{};
 std::uint32_t keycode{}, modifiers{};
};
struct Pad {
 bool installed{}, conflict{};
 unsigned primary{}, held{};
};
bool Installed() noexcept;
bool NativeMcmOwnsRegistry() noexcept;
// UI-thread MCM bridge; never edit global Keybinds.json while native MCM owns it.
void RefreshNativeMcm() noexcept;
bool StageNativeMcmBinding(Action action,unsigned key,unsigned modifiers) noexcept;
Keyboard CachedNativeMcmBinding(Action action) noexcept;
void MigrateLegacyIfNeeded() noexcept;
void SeedDefaultKeyboardBindings() noexcept;
Keyboard ReadKeyboard(Action action) noexcept;
bool SaveKeyboard(Action action,unsigned key,unsigned modifiers) noexcept;
Pad ReadPad(Action action) noexcept;
bool SavePad(Action action,unsigned primary,unsigned held) noexcept;
std::uint32_t PadMask(unsigned button) noexcept;
std::string KeyboardLabel(const Keyboard& binding,std::string_view fallback);
std::string SnapshotJson();
bool DecodeRequest(std::string_view raw,Action& action,unsigned& first,unsigned& second) noexcept;
}
