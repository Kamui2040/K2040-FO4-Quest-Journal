# K2040 FO4 Quest Journal

A full-screen quest journal and customizable on-screen tracker for **Fallout 4 (1.10.163)**. Browse your quests, keep track of objectives, and choose what appears on your HUD.

**[Download on Nexus Mods](https://www.nexusmods.com/fallout4/mods/110911)** · **[Download from GitHub](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/releases/latest)**

## Unreleased shared-settings adapter

This development branch exposes a versioned adapter for the optional K2040
Unified Settings hub. When selected there, Quest Journal opens directly to its
existing Settings tab using the current game-thread, focus and persistence
controls. Without the hub, Journal and Tracker retain their existing shortcuts
and settings UI. In-game acceptance remains pending. The developing HUD
replacement will be integrated separately when its settings are ready.

## Features

- Browse your quests and objectives, including completed quests.
- Find what you're looking for with search, sorting, categories and favorites.
- Track supported quests from the Journal and keep them in sync with your Pip-Boy.
- Pin quests to a compact tracker on your HUD. Choose where it appears, how large it is and how many quests it shows.
- Customize the Journal and tracker separately with five themes or your own colors.
- Change settings from the Journal or through the optional **K2040 Mods** MCM menu.
- Optionally use Fallout 4's Quick Data action (usually J) to open Quest Journal, following your current key binding. This is off by default.
- Keyboard and controller support.

## Requirements

- **Fallout 4 (original version 1.10.163)** — not the next-gen or Anniversary Edition version.
- [F4SE](https://f4se.silverlock.org/) for Fallout 4 1.10.163.
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327), original-game version.
- [PrismaUI for Fallout 4](https://www.nexusmods.com/fallout4/mods/105454).
- **Mod Configuration Menu (MCM)** is optional.

## Installation

1. Install the requirements listed above.
2. Download the latest Quest Journal ZIP from [Releases](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/releases/latest).
3. Import the ZIP into your mod manager and enable it. If you're updating, replace the older Quest Journal version rather than installing two copies.
4. Launch Fallout 4 through F4SE.

## Controls

| Action | Default shortcut |
| --- | --- |
| Open or close the Quest Journal | **Shift + G** |
| Show or hide the mini tracker | **G** |

You can change these shortcuts in MCM or the Journal settings. Your custom bindings will be kept when updating.

The upcoming v0.0.3 development build adds menu navigation: use the **D-pad** or **left stick** to move between controls, **A** to select, and **B** to go back. The right stick remains available for cursor navigation.

## Compatibility

Quest Journal is made for the original Fallout 4 (1.10.163). Some unusual or repeatable quests may have tracking limitations. See [Compatibility](docs/COMPATIBILITY.md) for more information.

## Source code and feedback

The source code is available under the [GPL-3.0 license](LICENSE). See [Building from source](docs/BUILDING.md) and [Third-party notices](THIRD_PARTY_NOTICES.md) for more information.

Found a bug or have a suggestion? [Open an issue](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues).
