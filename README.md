# K2040 FO4 Quest Journal

A full-screen quest journal and optional compact HUD tracker for **Fallout 4 original runtime 1.10.163**. Uses F4SE and PrismaUI_F4. No MAP76 dependency or external game assets are bundled.

**Project status:** public source checkout; a binary release has not yet been published from this repository. The corresponding 1.10.163 implementation was validated in-game before the public-source migration. Only the original Fallout 4 runtime is supported; Anniversary Edition, Creation Club changes and VR are not verified.

## Features

- Full-screen quest browser with objectives, search, status/category filters, favorites and manual HUD pins.
- Quest-wide track/untrack for eligible ordinary and Misc quests, with Pip-Boy and compass synchronization and fail-closed protection for ambiguous cases.
- Optional miniature tracker with four corners, themes, opacity, size, 1–12 visible quests, completed-objective display and Pip-Boy mirroring.
- Customizable independent Journal and tracker appearances: Commonwealth Blue, Classic Pip-Boy, High Contrast, Fallout 76 and Custom Colours.
- Integrated settings and K2040 Mods MCM grouping (when MCM is installed); controller support depends on PrismaUI's available controller features.
- Default keyboard shortcuts: **Shift+G** for full Journal, **G** to toggle mini tracker. Existing customized mappings are preserved.
- Per-save history limited to quests observed by the mod; private saves are never part of this repository.

## Requirements

- Fallout 4 **1.10.163 (original game)**, with matching original-game **F4SE 0.6.23** and Address Library for OG.
- A compatible **PrismaUI_F4** release exposing its Modern Core/View/Interop API (controller interface optional).
- **Mod Configuration Menu (MCM)** for the optional native MCM page; the built-in Journal settings remain available without MCM.

See [Compatibility](docs/COMPATIBILITY.md) for supported and unsupported cases.

## Installing the game mod

1. Install the supported original-game F4SE, Address Library and PrismaUI_F4 independently.
2. Obtain the Quest Journal installer ZIP from an official release **when one is published**. Do not treat a source checkout or experimental QA build as an installer.
3. Import the ZIP with your Fallout 4 mod manager. It contains `Data/F4SE/Plugins`, `Data/PrismaUI_F4/views` and optional grouped `Data/MCM` configuration. Install only one version of Quest Journal at a time.
4. Start Fallout 4 through the correct F4SE launcher, then use **Shift+G** for the Journal and **G** for the mini tracker.

The shared `K2040 Mods` MCM parent is identical across the author's compatible mods. Do not install duplicate conflicting copies or mix the grouped and standalone MCM variants.

## Building from source

This repository contains the complete original Quest Journal source and test inputs used for the standard 1.10.163 build, but not proprietary Windows tools, Bethesda files, PrismaUI framework binaries or vendor SDK files. The pinned external inputs can be obtained separately using the documented workflow.

See [Building on Linux](docs/BUILDING.md). In short, with the pinned CommonLibF4, official PrismaUI API header and licensed Windows/LLVM toolchain prepared:

```bash
bash scripts/verify-source.sh
bash scripts/build-normal-tracking.sh
python3 scripts/stage-test-package.py --output "$HOME/Downloads/K2040-FO4-Quest-Journal-OG163.zip" --mcm-mode grouped
```

`build-normal-tracking.sh` compiles the validated native quest-wide tracking path. `build-linux.sh` **without** the explicit tracking flag intentionally produces a read-only fallback. Neither script deploys a mod or publishes a release.

## Known limitations

Independent tracking of sibling Misc objectives and ambiguous multi-instance/radiant quests is not supported. There is no automatic native quest-stage editing, AE/VR support, or required MAP76 integration. Historical quests unseen by this mod cannot be reconstructed. See [Compatibility](docs/COMPATIBILITY.md).

## Feedback and license

Report bugs via [GitHub Issues](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues). Include game, F4SE and PrismaUI versions and clear reproduction steps; remove personal information from any logs.

Project-authored source is licensed under **GPL-3.0**: see [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md). The compiled plugin's corresponding source and build steps are available here; upstream libraries are obtained separately at the pinned revisions. No Bethesda, MAP76 or modified PrismaUI assets are redistributed.
