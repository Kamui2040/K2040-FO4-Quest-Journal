# Reproducible Windows x64 F4SE build from Linux

You can build the Fallout 4 Windows x64 plugin on Linux. The build uses Xmake and LLVM/clang-cl, along with the Windows SDK and pinned CommonLibF4 and PrismaUI files.

## Pinned external source inputs

- CommonLibF4 `libxse/commonlibf4` commit `12beba2a89fe117a14f1707b88c99ecb1b12f8c0` (MIT).
- Its `commonlib-shared` submodule commit `f0b1670ee9caac2e349497f6f3c08a69633a8ea7` (GPLv3 with modding/linking exceptions).
- Unmodified `PrismaUI_F4_Modern_API.h` from upstream PrismaUI_F4 commit `0daf5e38d3517772c8c28236c8857e5c037fa3bd`, blob `9784daab39bf66bb179e6ef12bba63c75e50d3b7`. License belongs to upstream PrismaUI_F4. **Do not commit the vendor header.**

Use `scripts/bootstrap-source-deps.sh` to fetch the pinned public source dependencies into a directory you choose. Obtain the MSVC headers and libraries and the Windows SDK independently. You also need LLVM/clang-cl, Xmake 3, Bash, Python 3, and Node.js.

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

The build checks the pinned CommonLibF4 revisions, the PrismaUI header checksum, and the Windows DLL's F4SE exports and tracking markers. `build-normal-tracking.sh` selects the standard tracking build. Running `build-linux.sh` without a build option creates a read-only fallback; it is not the gameplay build.

The compiled files are staged under `dist/Data`. The packaging script creates a ZIP with the mod, license, credits and installation guide. It does not install the mod.

## Tests and scope

`bash scripts/verify-source.sh` runs Node/JavaScript, Python and synthetic C++ tests. If browser Playwright is unavailable, its smoke test is explicitly reported **SKIPPED**. Build tests cannot verify every in-game situation.

## Dependencies and redistribution

See [Third-party notices](../THIRD_PARTY_NOTICES.md). When sharing a compiled copy, provide the matching source code, build instructions, project license, and this notice. The PrismaUI framework, CommonLibF4 source, F4SE, and game files are separate requirements; this repository and installer do not include them.
