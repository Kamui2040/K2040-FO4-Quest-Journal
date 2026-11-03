# Third-party source and dependency notices

Quest Journal's project-authored source and interface are licensed under GPL-3.0; see the repository's `LICENSE`. This source repository contains no Bethesda game data, F4SE loader binaries, MAP76 assets, modified PrismaUI framework, Windows SDK redistribution or personal gameplay logs.

- **CommonLibF4:** the pinned `libxse/commonlibf4` checkout uses the **MIT License** (Copyright 2019 ryan-rsm-mckenzie). Upstream source: https://github.com/libxse/commonlibf4 at `12beba2a89fe117a14f1707b88c99ecb1b12f8c0`. Its full MIT text is in the upstream `LICENSE`; the source is a separate build input, not vendored into the repo.
- **commonlib-shared:** pinned CommonLibF4 submodule at `f0b1670ee9caac2e349497f6f3c08a69633a8ea7` is GPLv3 with a documented **Modding Exception** and **GPL-3.0 Linking Exception (with Corresponding Source)**. See its upstream `LICENSE` and `EXCEPTIONS`. The corresponding-source obligations remain relevant to a distributed linked binary; the dependency is resolved as a separately pinned upstream source tree.
- **PrismaUI_F4:** the official Fallout 4 framework/SDK belongs to its authors under a separate license: https://github.com/PRISMA-USER-INTERFACE-FRAMEWORK/Fallout-4-Prisma-UI-Framework/blob/main/LICENSE.md. The unmodified official API header is staged locally from a pinned upstream revision and excluded from Git. Only original Quest Journal UI sources are published; users install PrismaUI_F4 independently. No modified framework or Ultralight resources are distributed.
- **F4SE, Address Library and Fallout 4:** independent end-user runtime/game dependencies. Nothing from their redistributable or non-redistributable binary distributions is bundled.
- **MAP76:** public technical reference for quest data; no MAP76 source or proprietary web assets are copied. https://github.com/BerryDangerous/MAP76
- **K2040 Quick Attach Menu:** the author's own separately licensed GPL-3.0 mod informed shared K2040 UI/input conventions. Quest Journal source is independently maintained. https://github.com/Kamui2040/K2040-Quick-Attach-Menu

A compiled Quest Journal distribution should accompany an accessible exact source revision and build instructions, the project GPL license and this notice. This is source-and-attribution documentation, not a grant to redistribute any upstream proprietary assets.
