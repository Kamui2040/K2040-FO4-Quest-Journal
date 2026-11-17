# Third-party software and licenses

Quest Journal's source code is licensed under **GPL-3.0**. See [LICENSE](LICENSE).

The project uses some external tools and libraries. They are installed or fetched separately rather than copied into this repository. These notes record their licenses and the exact versions used for builds.

- **CommonLibF4:** `libxse/commonlibf4`, pinned at `12beba2a89fe117a14f1707b88c99ecb1b12f8c0`. The [upstream repository](https://github.com/libxse/commonlibf4) uses the **MIT License** (Copyright 2019 ryan-rsm-mckenzie); see its `LICENSE`. Its source is a separate build input.
- **commonlib-shared:** Submodule pinned at `f0b1670ee9caac2e349497f6f3c08a69633a8ea7`. It uses **GPLv3** with a documented **Modding Exception** and **GPL-3.0 Linking Exception (with Corresponding Source)**. Check upstream `LICENSE` and `EXCEPTIONS` for the terms.
- **PrismaUI_F4:** The framework/SDK belong to their authors under a [separate license](https://github.com/PRISMA-USER-INTERFACE-FRAMEWORK/Fallout-4-Prisma-UI-Framework/blob/main/LICENSE.md). The original API header is staged locally from a pinned revision and ignored by Git. Players install PrismaUI separately. Quest Journal does not distribute a modified framework or Ultralight resources.
- **F4SE, Address Library, and Fallout 4:** External tools and game files installed separately.
- **K2040 Quick Attach Menu:** The author's separately maintained [GPL-3.0 mod](https://github.com/Kamui2040/K2040-Quick-Attach-Menu) informed some shared UI/input conventions. Quest Journal's source is maintained separately.

**When distributing a compiled DLL:** Provide the matching source revision, build instructions, project GPL license, and these notices. The linked dependency's corresponding-source and exception terms still apply where relevant.
