#!/usr/bin/env bash
# Linux/Bazzite -> Windows x64 build using the proven Minimap and Quick Attach toolchain.
# Does not deploy to Fallout 4 or publish any release.
set -euo pipefail

root="$(cd -- "$(dirname -- "$0")/.." && pwd)"
for name in K2040_COMMONLIBF4_ROOT K2040_WINDOWS_SDK_ROOT K2040_LLVM_TOOL_DIR; do
    if ! printenv "$name" >/dev/null; then
        echo "Missing required environment variable: $name" >&2
        exit 1
    fi
done

# clang-cl need not be installed globally: like the proven Quick Attach
# setup, the requested LLVM directory may contain all native host tools.
[[ -d "$K2040_LLVM_TOOL_DIR" ]] ||
    { echo "Missing LLVM host tools directory: $K2040_LLVM_TOOL_DIR" >&2; exit 1; }
export PATH="$K2040_LLVM_TOOL_DIR:$PATH"
for tool in xmake clang-cl node python3 git; do
    command -v "$tool" >/dev/null 2>&1 ||
        { echo "Missing build/test tool: $tool" >&2; exit 1; }
done

# Optional reuse of the exact original SDK header from the minimap checkout.
# No download, overwrite of a different version, or game deployment occurs.
if [[ -n "${K2040_PRISMA_HEADER_SOURCE:-}" ]]; then
    python3 "$root/scripts/stage-prisma-header.py" --source "$K2040_PRISMA_HEADER_SOURCE"
fi

[[ -f "$K2040_COMMONLIBF4_ROOT/xmake.lua" ]] ||
    { echo "Missing CommonLibF4 xmake.lua" >&2; exit 1; }
[[ -f "$K2040_LLVM_TOOL_DIR/link.exe" ]] ||
    { echo "Missing LLVM linker/archiver: $K2040_LLVM_TOOL_DIR/link.exe" >&2; exit 1; }

msvc_root="$K2040_WINDOWS_SDK_ROOT/VC/Tools/MSVC"
sdk_root="$K2040_WINDOWS_SDK_ROOT/Windows Kits/10"
[[ -d "$msvc_root" && -d "$sdk_root/Include" ]] ||
    { echo "Missing Windows MSVC/SDK directories" >&2; exit 1; }

msvc_version="$(find "$msvc_root" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort -V | tail -n 1)"
sdk_version="$(find "$sdk_root/Include" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort -V | tail -n 1)"
[[ -n "$msvc_version" && -n "$sdk_version" ]] ||
    { echo "Cannot discover MSVC or Windows SDK version" >&2; exit 1; }

msvc="$msvc_root/$msvc_version"
windows_linkdirs="$msvc/lib/x64:$sdk_root/Lib/$sdk_version/um/x64:$sdk_root/Lib/$sdk_version/ucrt/x64"

for dir in "$msvc/lib/x64" "$sdk_root/Lib/$sdk_version/um/x64" "$sdk_root/Lib/$sdk_version/ucrt/x64"; do
    [[ -d "$dir" ]] || { echo "Missing library path: $dir" >&2; exit 1; }
done

export K2040_WINDOWS_LINK_DIRS="$windows_linkdirs"

# Nobara/Linux clang-cl does not discover the extracted MSVC C++ standard
# headers automatically. On Linux clang-cl expects a semicolon-delimited
# INCLUDE list, just as an MSVC developer environment would provide.
export INCLUDE="$msvc/include;$sdk_root/Include/$sdk_version/ucrt;$sdk_root/Include/$sdk_version/um;$sdk_root/Include/$sdk_version/shared${INCLUDE:+;$INCLUDE}"
command -v llvm-rc >/dev/null || { echo "Missing LLVM Windows resource compiler: llvm-rc" >&2; exit 1; }

cd "$root"
# Normal distribution path is still explicitly selected for the R2-accepted
# writer, not silently enabled by a cached xmake configuration. An ordinary
# no-flag build must remain read-only until the release gate is approved.
tracking_flag=n
if [[ "${K2040_QUESTWIDE_NORMAL_TRACKING:-NO}" == YES ]]; then
    tracking_flag=y
elif [[ "${K2040_QUESTWIDE_NORMAL_TRACKING:-NO}" != NO ]]; then
    echo 'K2040_QUESTWIDE_NORMAL_TRACKING must be YES or NO' >&2;exit 2
fi
python3 scripts/build-preflight.py --commonlib "$K2040_COMMONLIBF4_ROOT" --sdk "$K2040_WINDOWS_SDK_ROOT"
echo "Building K2040 Quest Journal with clang-cl[llvm]"
echo "MSVC version: $msvc_version; Windows SDK: $sdk_version"

# Do not replace this with --toolchain=llvm or automatic MSVC detection:
# these chose an incompatible CMake/spdlog compiler setup during investigation.
xmake f -c -y \
    -p windows \
    -a x64 \
    -m releasedbg \
    --quest_dryrun_qa=n \
    --quest_questwide_production_candidate=n \
    --quest_questwide_normal_tracking="$tracking_flag" \
    --quest_native_write_qa=n \
    --quest_repeatable_misc_qa=n \
  --quest_integrated_tracking_qa=n \
    '--toolchain=clang-cl[llvm]' \
    --sdk="$K2040_WINDOWS_SDK_ROOT" \
    --ar="$K2040_LLVM_TOOL_DIR/link.exe" \
    --mrc=llvm-rc \
    '--ldflags=/MANIFEST:NO' \
    '--shflags=/MANIFEST:NO' \
    --linkdirs="$K2040_WINDOWS_LINK_DIRS"

# Capture an independent freshness marker *before* compiling. The staged
# DLL may exist from a previous source revision; do not report it as this build.
build_marker="$(mktemp)"
trap 'rm -f -- "$build_marker"' EXIT
xmake -r K2040_Quest_Journal
bash scripts/verify-source.sh

dll="$root/dist/Data/F4SE/Plugins/K2040_Quest_Journal.dll"
[[ -s "$dll" ]] || { echo "BLOCKED: build produced no DLL" >&2; exit 1; }
[[ "$dll" -nt "$build_marker" ]] || {
    echo "BLOCKED: staged DLL was not updated by this build; refuse stale tester" >&2
    exit 1
}
if [[ "$tracking_flag" == y ]]; then
    python3 scripts/check-built-dll.py --dll "$dll" --require-questwide-tracking
    if [[ -f "$root/dist/Data/F4SE/Plugins/K2040_Quest_Journal_Write_QA.ini" ]]; then
        echo 'BLOCKED: QA consent INI in normal quest-wide build' >&2;exit 1
    fi
    echo 'PASS: normal OG163 Journal with R2-accepted quest-wide tracking; staging only'
else
    python3 scripts/check-built-dll.py --dll "$dll" --require-readonly-shipping
    echo 'PASS: read-only normal build; native writer disabled'
fi
echo "Build and PE loader-export checks passed: $dll"
echo "No files deployed to Fallout 4."
