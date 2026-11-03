#!/usr/bin/env bash
# Build isolated Fallout 4 OG read-only staged startup diagnostic on Linux.
# NEVER copies DLL into game, mod-manager directories or project dist/.
set -euo pipefail
root="$(cd -- "$(dirname -- "$0")/.." && pwd -P)"
for name in K2040_COMMONLIBF4_ROOT K2040_WINDOWS_SDK_ROOT K2040_LLVM_TOOL_DIR K2040_STAGED_INIT_STAGE; do
    [[ -n "${!name:-}" ]] || { echo "Set $name" >&2; exit 2; }
done
for name in xmake clang-cl llvm-objdump file git; do
    command -v "$name" >/dev/null || { echo "Missing $name" >&2; exit 2; }
done
common="$(realpath -e "$K2040_COMMONLIBF4_ROOT")"
sdk="$(realpath -e "$K2040_WINDOWS_SDK_ROOT")"
llvm="$(realpath -e "$K2040_LLVM_TOOL_DIR")"
mkdir -p "$K2040_STAGED_INIT_STAGE"
stage="$(realpath -e "$K2040_STAGED_INIT_STAGE")"
case "$stage" in
    /|"$HOME"|"$root"|"$root"/*|*SteamLibrary*|*steamapps*|*/Data/F4SE/*)
        echo 'Refuse game, home, deployment or repository staging' >&2
        exit 2 ;;
esac
case "$root" in "$stage"/*) echo 'Refuse project staging parent' >&2; exit 2;; esac
test "$(git -C "$common" rev-parse HEAD)" =     '12beba2a89fe117a14f1707b88c99ecb1b12f8c0' ||
    { echo 'Unexpected CommonLibF4 checkout' >&2; exit 2; }
msvc_root="$sdk/VC/Tools/MSVC"
msvc_ver="$(find "$msvc_root" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort -V | tail -n 1)"
win_root="$sdk/Windows Kits/10"
win_ver="$(find "$win_root/Include" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort -V | tail -n 1)"
test -n "$msvc_ver" && test -n "$win_ver"
msvc="$msvc_root/$msvc_ver"
export K2040_STAGED_INIT_OUTPUT_DIR="$stage/bin"
export PATH="$llvm:$PATH"
export INCLUDE="$msvc/include;$win_root/Include/$win_ver/ucrt;$win_root/Include/$win_ver/um;$win_root/Include/$win_ver/shared"
export K2040_WINDOWS_LINK_DIRS="$msvc/lib/x64:$win_root/Lib/$win_ver/um/x64:$win_root/Lib/$win_ver/ucrt/x64"
test -s "$llvm/link.exe"
mkdir -p "$stage/build" "$stage/bin"
proj="$root/research/native_staged_init"

# -P is essential: xmake otherwise searches the parent project and
# can wrongly report success without building this separate target.
xmake f -P "$proj" -c -y -p windows -a x64 -m releasedbg \
    '--toolchain=clang-cl[llvm]' --sdk="$sdk" \
    --ar="$llvm/link.exe" --mrc=llvm-rc \
    '--ldflags=/MANIFEST:NO' '--shflags=/MANIFEST:NO' \
    --linkdirs="$K2040_WINDOWS_LINK_DIRS" -o "$stage/build"
# The exact generated directory is required for pinned CommonLibF4 and
# old xmake's "gens/." dependency emission on Nobara.
for kind in .objs .deps; do
    mkdir -p "$stage/build/$kind/K2040_Quest_Staged_Init/windows/x64/releasedbg/gens"
done
stamp="$(mktemp)"
trap 'rm -f -- "$stamp"' EXIT
xmake -P "$proj" -j 1 -r K2040_Quest_Staged_Init
dll="$stage/bin/K2040_Quest_Staged_Init.dll"
test -s "$dll" && test "$dll" -nt "$stamp" ||
    { echo 'BLOCKED: no newly linked isolated diagnostic DLL' >&2; exit 1; }
file "$dll" | grep -q 'PE32+ executable.*DLL'
for symbol in F4SEPlugin_Load F4SEPlugin_Query F4SEPlugin_Version; do
    llvm-objdump -p "$dll" | grep -q -w "$symbol" ||
        { echo "Missing F4SE export $symbol" >&2; exit 1; }
done
echo 'PASS: linked isolated OG nonmutating event diagnostic PE with three F4SE exports'
echo "Staged at: $dll"
echo 'NOT installed, deployed, loaded, or validated in running Fallout 4.'
