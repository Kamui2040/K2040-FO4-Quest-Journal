# K2040 FO4 Quest Journal

A full-screen quest journal and a small, customizable HUD tracker for **Fallout 4 (original PC version 1.10.163)**.

[Get the released mod on Nexus Mods](https://www.nexusmods.com/fallout4/mods/110911) · [GitHub releases](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/releases/latest)

## What it does

- Browse active and completed quests, search objectives, and sort by category.
- Mark favorites and track supported quests alongside the Pip-Boy.
- Pin quests to a mini tracker, with adjustable position, size, and quest count.
- Change the Journal and tracker appearance separately, including custom colors.
- Optionally open the Journal with Fallout 4's Quick Data key (normally J). This is off by default.

**Released version: v0.0.2.** The newer controller changes, shared settings editor, four Framework styles, and draggable tracker layout are still being tested. They are **not** included in the public release.

## Requirements

- Fallout 4 **1.10.163** (the original PC version, not next-gen or Anniversary Edition).
- [F4SE](https://f4se.silverlock.org/) for that game version.
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327), original-game version.
- [PrismaUI for Fallout 4](https://www.nexusmods.com/fallout4/mods/105454).
- MCM is optional for the released version.

## Install

1. Install the requirements above.
2. Download the Quest Journal ZIP from the [release page](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/releases/latest).
3. Import it into your mod manager and enable it. Replace older Quest Journal copies rather than running two at once.
4. Start the game through F4SE.

## Controls

| Action | Default key |
| --- | --- |
| Open or close the Journal | **Shift + G** |
| Show or hide the mini tracker | **G** |

The released version lets you change these keys through Journal settings or MCM. Existing custom bindings are kept when you update.

### Development builds (not released)

The current development build uses **K2040 Unified Settings V2** for its settings. Install the compatible hub separately: the Quest Journal test ZIP does not contain the hub, its MCM entry, or its settings screen.

The Journal's **Settings** button opens its page inside that hub. The hub owns the one settings shortcut; the Journal and tracker keep their separate *gameplay* shortcuts. Pip-Boy synchronization remains a checkbox in the Journal.

Development controller navigation uses the D-pad or left stick. Confirm and Back follow Fallout 4's actual mapped buttons, including Steam Input mappings where available, instead of assuming Xbox A/B. The right stick can move the cursor. The compact tracker also has an editor for dragging and resizing it, with Save, Abort, and Retry.

These changes have passed source/build checks, **not full in-game testing**. The Journal should stay open with a message if the separate hub is missing or incompatible.

## Compatibility and help

Quest Journal is built for Fallout 4 1.10.163. Some repeatable quests and mod-added objectives may behave differently. The next-gen, Anniversary Edition, and VR versions are not supported. See [Compatibility](docs/COMPATIBILITY.md) and the [Roadmap](docs/ROADMAP.md).

Found a bug? [Open a GitHub issue](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues) with your game version and what happened.

## Source

Licensed under [GPL-3.0](LICENSE). See [Building from source](docs/BUILDING.md) and [Third-party notices](THIRD_PARTY_NOTICES.md).
