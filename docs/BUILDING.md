# Reproducible Windows x64 F4SE build from Linux

The project uses Xmake and the same Linux-to-Windows toolchain approach as K2040 Quick Attach Menu. The supplied source does **not** contain Windows SDK libraries, the proprietary game, F4SE binaries or PrismaUI runtime/framework assets.

## Pinned external source inputs

- CommonLibF4 `libxse/commonlibf4` commit `12beba2a89fe117a14f1707b88c99ecb1b12f8c0` (MIT).
- Its `commonlib-shared` submodule commit `f0b1670ee9caac2e349497f6f3c08a69633a8ea7` (GPLv3 with modding/linking exceptions).
- Unmodified `PrismaUI_F4_Modern_API.h` from upstream PrismaUI_F4 commit `0daf5e38d3517772c8c28236c8857e5c037fa3bd`, blob `9784daab39bf66bb179e6ef12bba63c75e50d3b7`. License belongs to upstream PrismaUI_F4. **Do not commit the vendor header.**

`scripts/bootstrap-source-deps.sh` can fetch/pin public source dependencies into a chosen directory. Obtain any extracted MSVC headers/libraries and Windows SDK legally and independently; prepare LLVM/clang-cl, xmake 3, Bash, Python 3 and Node.js. The script never downloads or redistributes Microsoft's toolchain.

## Build

```bash
export K2040_COMMONLIBF4_ROOT="/path/to/pinned/commonlibf4"
export K2040_WINDOWS_SDK_ROOT="/path/to/your/windows-sdk-and-msvc"
export K2040_LLVM_TOOL_DIR="/path/to/your/native-llvm-tools"

bash scripts/verify-source.sh
bash scripts/build-normal-tracking.sh
python3 scripts/stage-test-package.py \
  --output "$HOME/Downloads/K2040-FO4-Quest-Journal-OG163.zip" \
  --mcm-mode grouped
```

The Xmake recipe verifies exact CommonLibF4 and submodule revisions, the PrismaUI API header's checksum and the Windows x64 PE image's F4SE exports and native tracking/readback markers. The standard tracked release flavor is selected explicitly by `build-normal-tracking.sh`. An unflagged `build-linux.sh` remains read-only and is useful as a rollback test; it is not the gameplay-feature build.

The build stage is `dist/Data`. The packaging script includes only the mod's compiled DLL, original authored UI, grouped MCM configuration, license and public installation guidance. It never installs files into Fallout 4, launches the game or creates a GitHub release. Inspect the package and release version before publishing it.

## Tests and scope

`bash scripts/verify-source.sh` runs Node/JavaScript, Python and synthetic C++ tests. If browser Playwright is unavailable, its smoke test is explicitly reported **SKIPPED**. PE export and source tests are not substitutes for actual Fallout 4 compatibility tests. Never claim an untested runtime PASS.

## Dependencies and redistribution

See [Third-party notices](../THIRD_PARTY_NOTICES.md). Keep the repository's license and corresponding source publicly available whenever distributing its compiled GPL project code. The original PrismaUI framework, CommonLibF4 source, F4SE and game files are separate dependencies and are not bundled in this repository or installer.
