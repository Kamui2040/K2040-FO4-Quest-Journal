# Building Quest Journal on Linux

This produces a Windows x64 F4SE plugin for **Fallout 4 1.10.163**. You can build it on Linux, but the result still needs to be tested in the actual game.

## Tools and pinned dependencies

Install LLVM/clang-cl, Xmake 3, Bash, Python 3, and Node.js. You also need compatible MSVC headers/libraries and the Windows SDK.

The build checks these exact external sources:

- **CommonLibF4:** `libxse/commonlibf4` commit `12beba2a89fe117a14f1707b88c99ecb1b12f8c0` (MIT).
- **commonlib-shared:** submodule commit `f0b1670ee9caac2e349497f6f3c08a69633a8ea7` (GPLv3 with modding/linking exceptions).
- **PrismaUI_F4 API header:** unmodified `PrismaUI_F4_Modern_API.h` from commit `0daf5e38d3517772c8c28236c8857e5c037fa3bd`, blob `9784daab39bf66bb179e6ef12bba63c75e50d3b7`. Its license belongs to PrismaUI.

Run `scripts/bootstrap-source-deps.sh` to fetch the pinned public sources into a directory of your choice. Install the Windows toolchain separately. **Do not commit the PrismaUI vendor header** or local SDK files.

**Why pin versions?** A framework or header update can compile cleanly while changing the behavior expected by an F4SE plugin.

## Check the settings manifest

Install the development-only JSON validator, then check that the settings list is valid:

```bash
npm ci --ignore-scripts --no-audit --no-fund
npm run validate:settings
```

The dependency is pinned in `package-lock.json` and is **not** included in the game installer.

`scripts/verify-source.sh` also checks the manifest against the native settings definitions and tests the schema copied into this repository. The hub does not need to be running for these tests.

**Why:** The shared editor reads the manifest, but the Journal's C++ plugin owns the real settings. Both must agree on field names, limits, and types.

See [Settings catalog](SETTINGS_CATALOG_V1.md) for the fields and their behavior.

## Build and stage a test ZIP

Set these paths for your installed build tools and pinned dependency tree:

```bash
export K2040_COMMONLIBF4_ROOT="/path/to/pinned/commonlibf4"
export K2040_WINDOWS_SDK_ROOT="/path/to/your/windows-sdk-and-msvc"
export K2040_LLVM_TOOL_DIR="/path/to/your/native-llvm-tools"

bash scripts/verify-source.sh
bash scripts/build-normal-tracking.sh
python3 scripts/stage-test-package.py   --output "$HOME/Downloads/testing/K2040-Quest-Journal-v0.0.4.zip"   --mcm-mode none
```

`build-normal-tracking.sh` makes the gameplay tracking build. An unflagged `build-linux.sh` makes a read-only fallback; do not confuse the two.

The build verifies pinned revisions, the PrismaUI header checksum, Windows DLL/F4SE exports, and tracking markers. Compiled files go to `dist/Data`. The packaging script puts the mod, license, credits, and installation instructions in a ZIP. **It does not install the mod.** Keep only the newest validated Quest Journal package directly under `~/Downloads/testing/`, without nested folders. Use `K2040-Quest-Journal-vX.Y.Z.zip` for each package. Increase the version when contents change. The current `v0.0.4` is **development QA**, not a public release; the published version is still `v0.0.2`.

For development testing, keep the Journal ZIP separate from the compatible **Unified Settings Hub** package. The Journal ZIP contains its DLL, provider, and PrismaUI files, but **not** `K2040_Unified_Settings.dll`, hub UI, hub MCM, or user preferences. The Journal has no separate MCM settings page in this branch.

The older `scripts/package-unified-vnext-qa.py` helper remains available to reproduce an earlier combined test package, but it is **not** the normal way to prepare the current QA build.

## Live tracker layout

The Journal exports `K2040LayoutAdapterV2`, matching the hub's ABI. The hub displays the drag/resize overlay, but the Journal plugin checks the proposed rectangle and controls when it is saved to the user's existing INI. Canceling or retrying a preview must not change any quest data.

**Why:** Position previews should be reversible, and the shared editor should never write into another mod's settings or saves.

## What the tests prove

`bash scripts/verify-source.sh` runs JavaScript, Python, and synthetic native C++ tests. If Playwright is not installed, the browser smoke test is marked **SKIPPED**, not passed. A successful DLL build also does not prove that the game can load every screen correctly.

Before accepting a new build, complete the [in-game checklist](UNIFIED_SETTINGS_VNEXT_QA.md): controller mapping, quest tracking, tracker positioning, Save/Abort/Retry, and settings persistence.

## Sharing builds

Read [Third-party notices](../THIRD_PARTY_NOTICES.md) before distributing a compiled plugin. Provide its matching source revision, build instructions, GPL license, and notices. The game, F4SE, PrismaUI, and other external dependencies are installed separately.
