#!/usr/bin/env bash
# Isolated OG163 one-shot read-only vanilla Pipboy GFx selection observer.
# Refuses game, Amethyst, repository or user's home as output directories.
# Never deploy. No quest writer or mutation UI is compiled.
# Only ONE Quest Journal DLL may be active in the existing Amethyst profile.
set -euo pipefail
root="$(cd -- "$(dirname -- "$0")/.." && pwd -P)"
for var in K2040_COMMONLIBF4_ROOT K2040_WINDOWS_SDK_ROOT K2040_LLVM_TOOL_DIR K2040_PIPBOY_GFX_QA_STAGE; do
    [[ -n "$(printenv "$var" 2>/dev/null || true)" ]] ||
        { echo "Missing $var" >&2; exit 2; }
done
for tool in xmake clang-cl llvm-objdump python3 node git file; do
    command -v "$tool" >/dev/null || { echo "Missing tool $tool" >&2; exit 2; }
done
common="$(realpath -e "$K2040_COMMONLIBF4_ROOT")"
sdk="$(realpath -e "$K2040_WINDOWS_SDK_ROOT")"
llvm="$(realpath -e "$K2040_LLVM_TOOL_DIR")"
mkdir -p "$K2040_PIPBOY_GFX_QA_STAGE"
stage="$(realpath -e "$K2040_PIPBOY_GFX_QA_STAGE")"
# Dedicated stage; do not create production or integrated writer artifacts.
case "$stage" in
    /|"$HOME"|"$root"|"$root"/*|*SteamLibrary*|*steamapps*|*Amethyst*|*/Data/F4SE/*)
        echo 'BLOCKED: unsafe QA build destination' >&2;exit 2 ;;
esac
case "$root" in "$stage"/*) echo 'BLOCKED: stage is project parent' >&2; exit 2;; esac
test "$(git -C "$common" rev-parse HEAD)" = '12beba2a89fe117a14f1707b88c99ecb1b12f8c0'
test -f "$root/external/prismaui_f4/PrismaUI_F4_Modern_API.h"
msvc_root="$sdk/VC/Tools/MSVC"
msvc_version="$(find "$msvc_root" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort -V | tail -n1)"
win_root="$sdk/Windows Kits/10"
sdk_version="$(find "$win_root/Include" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' | sort -V | tail -n1)"
test -n "$msvc_version" && test -n "$sdk_version"
msvc="$msvc_root/$msvc_version"
export PATH="$llvm:$PATH"
export INCLUDE="$msvc/include;$win_root/Include/$sdk_version/ucrt;$win_root/Include/$sdk_version/um;$win_root/Include/$sdk_version/shared"
export K2040_WINDOWS_LINK_DIRS="$msvc/lib/x64:$win_root/Lib/$sdk_version/um/x64:$win_root/Lib/$sdk_version/ucrt/x64"
for dir in "$msvc/lib/x64" "$win_root/Lib/$sdk_version/um/x64" "$win_root/Lib/$sdk_version/ucrt/x64";do test -d "$dir";done
test -s "$llvm/link.exe"
node --check "$root/view/app.js"
python3 "$root/scripts/build-preflight.py" --commonlib "$common" --sdk "$sdk"
mkdir -p "$stage/build" "$stage/bin"
cd "$root"
xmake f -c -y -p windows -a x64 -m releasedbg \
  --quest_dryrun_qa=n \
  --quest_questwide_production_candidate=n \
  --quest_native_write_qa=n \
  --quest_integrated_tracking_qa=n \
  --quest_single_misc_write_qa=n \
  --quest_single_misc_rc=n \
  --quest_dogmeat_production_candidate=n \
  --quest_selector_readonly_qa=n \
  --quest_pipboy_selection_readonly_qa=y \
  '--toolchain=clang-cl[llvm]' --sdk="$sdk" \
  --ar="$llvm/link.exe" --mrc=llvm-rc \
  '--ldflags=/MANIFEST:NO' '--shflags=/MANIFEST:NO' \
  --linkdirs="$K2040_WINDOWS_LINK_DIRS" \
  -o "$stage/build"
# Pinned CommonLibF4 plugin rule generates a gens/. dependency path.
# On Nobara xmake can fail to create it unless both object/deps
# directories are pre-created (same verified workaround as the observer).
for kind in .objs .deps;do
    mkdir -p "$stage/build/$kind/K2040_Quest_Journal/windows/x64/releasedbg/gens"
done
stamp="$(mktemp)"
trap 'rm -f -- "$stamp"' EXIT
xmake -j 1 -r K2040_Quest_Journal
dll="$stage/Data/F4SE/Plugins/K2040_Quest_Journal.dll"
test -s "$dll" && test "$dll" -nt "$stamp" || {
    echo 'BLOCKED: missing or stale Pipboy GFx QA DLL' >&2;exit 1;
}
file "$dll" | grep -q 'PE32+ executable.*DLL'
for symbol in F4SEPlugin_Load F4SEPlugin_Query F4SEPlugin_Version;do
    llvm-objdump -p "$dll" | grep -q -w "$symbol" || {
        echo "Missing $symbol" >&2;exit 1;
    }
done
grep -q 'receiveQuestSnapshot' "$stage/Data/PrismaUI_F4/views/K2040QuestJournal/app.js"
test ! -e "$stage/Data/F4SE/Plugins/K2040_Quest_Journal_Write_QA.ini"
test -f "$stage/Data/MCM/Config/K2040_Mods/config.json" || {
    # MCM layouts may use JSON files with different letter case,
    # but both bundles must have staged actual files.
    test "$(find "$stage/Data/MCM/Config/K2040_Mods" -type f | wc -l)" -gt 0
}
test "$(find "$stage/Data/MCM/Config/K2040_Quest_Journal" -type f | wc -l)" -gt 0
echo "PASS: isolated Pipboy GFx-only DLL built without any quest writer QA defines"
echo "Pipboy GFx read-only output: $stage/Data"
echo 'No game or Amethyst files deployed.'
