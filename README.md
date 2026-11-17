# K2040 FO4 Quest Journal

A full-screen quest journal and customizable on-screen tracker for **Fallout 4 (1.10.163)**. Browse your quests, keep track of objectives, and choose what appears on your HUD.

**[Download on Nexus Mods](https://www.nexusmods.com/fallout4/mods/110911)** · **[Download from GitHub](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/releases/latest)**

## Features

- Browse your quests and objectives, including completed quests.
- Find what you're looking for with search, sorting, categories and favorites.
- Track supported quests from the Journal and keep them in sync with your Pip-Boy.
- Pin quests to a compact HUD tracker. In the development build, the Unified Settings V2 layout editor lets you drag and resize it, then Save, Abort or Retry without changing quest tracking.
- Customize the Journal and tracker with four shared visual styles and separately saved colors.
- Edit settings in the **K2040 Unified Settings V2** two-pane editor using the configurable shortcut owned by the separate Unified Settings hub. Individual Journal settings controls are no longer maintained.
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

**Development QA builds:** Install the Quest Journal ZIP and the compatible Unified Settings Hub separately. The Quest Journal ZIP contains only the mod DLL, provider and PrismaUI views; it does not bundle the hub DLL, hub UI or MCM.
The **Settings** button in the Journal opens the **Quest Journal** page in the updated hub, not a standalone settings renderer. If the hub is unavailable or outdated, the Journal stays open and displays a message. The Pip-Boy sync checkbox remains directly in the Journal.

## Controls

| Action | Default shortcut |
| --- | --- |
| Open or close the Quest Journal | **Shift + G** |
| Show or hide the mini tracker | **G** |

These are **gameplay shortcuts**, not settings-only shortcuts. The VNext development build configures Journal/tracker keyboard and controller shortcuts in the shared Unified Settings **Controls** category, not in Quest Journal's MCM. The hub alone installs and configures the shared settings shortcut through its MCM entry. This owner package contains no separate MCM entries. The published v0.0.2 package is unchanged.

The upcoming v0.0.3 development build adds D-pad and left-stick menu navigation and follows Fallout 4's effective Confirm/Back assignments rather than assuming Xbox button labels. The right stick remains available for cursor navigation.

## Compatibility

Quest Journal is made for the original Fallout 4 (1.10.163). Some unusual or repeatable quests may have tracking limitations. See [Compatibility](docs/COMPATIBILITY.md) for more information. Anniversary Edition support is planned, but not available; see the [Roadmap](docs/ROADMAP.md).

## Source code and feedback

The source code is available under the [GPL-3.0 license](LICENSE). See [Building from source](docs/BUILDING.md) and [Third-party notices](THIRD_PARTY_NOTICES.md) for more information.

Found a bug or have a suggestion? [Open an issue](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues).
