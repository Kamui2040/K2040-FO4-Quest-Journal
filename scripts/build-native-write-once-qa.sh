#!/usr/bin/env bash
# Explicit, isolated, non-shipping Windows x64 Quest Journal dry-run QA build.
# Refuses game, Amethyst, repository or user's home as output directories.
# Never deploys any files; candidate package may be imported only into
# the already approved OG163 Collection Staging Amethyst profile, never
# as a second simultaneous Quest Journal copy. Do not create new profiles.
set -euo pipefail
root="$(cd -- "$(dirname -- "$0")/.." && pwd -P)"
for var in K2040_COMMONLIBF4_ROOT K2040_WINDOWS_SDK_ROOT K2040_LLVM_TOOL_DIR K2040_NATIVE_WRITE_QA_STAGE; do
    [[ -n "$(printenv "$var" 2>/dev/null || true)" ]] ||
        { echo "Missing $var" >&2; exit 2; }
done
for tool in xmake clang-cl llvm-objdump python3 node git file; do
    command -v "$tool" >/dev/null || { echo "Missing tool $tool" >&2; exit 2; }
done
common="$(realpath -e "$K2040_COMMONLIBF4_ROOT")"
sdk="$(realpath -e "$K2040_WINDOWS_SDK_ROOT")"
llvm="$(realpath -e "$K2040_LLVM_TOOL_DIR")"
mkdir -p "$K2040_NATIVE_WRITE_QA_STAGE"
stage="$(realpath -e "$K2040_NATIVE_WRITE_QA_STAGE")"
# Shared xmake QA staging variable required by the target.
export K2040_DRYRUN_QA_STAGE="$stage"
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
# Explicit private opt-in for a distinct generic Misc QA candidate;
# normal one-shot and production builds remain unchanged and fail closed.
generic_misc_flag=n
if [[ "${K2040_GENERIC_MISC_QA:-NO}" == 'YES' ]];then generic_misc_flag=y;fi
repeatable_flag=n
if [[ "${K2040_REPEATABLE_MISC_QA:-NO}" == 'YES' ]];then repeatable_flag=y;fi
if [[ "$repeatable_flag" == y && "$generic_misc_flag" == y ]];then
    echo 'BLOCKED: competing generic one-shot and repeatable native writer';exit 2
fi
xmake f -c -y -p windows -a x64 -m releasedbg \
  --quest_dryrun_qa=y \
  --quest_questwide_production_candidate=n \
  --quest_native_write_qa="$([[ "$repeatable_flag" == y ]] && echo n || echo y)" \
  --quest_generic_misc_questwide_qa="$generic_misc_flag" \
  --quest_repeatable_misc_qa="$repeatable_flag" \
  --quest_integrated_tracking_qa=n \
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
    echo 'BLOCKED: missing or stale QA flavor DLL' >&2;exit 1;
}
file "$dll" | grep -q 'PE32+ executable.*DLL'
for symbol in F4SEPlugin_Load F4SEPlugin_Query F4SEPlugin_Version;do
    llvm-objdump -p "$dll" | grep -q -w "$symbol" || {
        echo "Missing $symbol" >&2;exit 1;
    }
done
grep -q 'requestPipboyTrackingPreview' "$stage/Data/PrismaUI_F4/views/K2040QuestJournal/app.js"
grep -q 'qaSelectedQuest' "$stage/Data/PrismaUI_F4/views/K2040QuestJournal/app.js"
if [[ "$generic_misc_flag" == y ]];then
    grep -aFq 'QuestGenericMiscQA: returned' "$dll" || {
        echo 'BLOCKED: generic Misc QA reader not compiled' >&2;exit 1;
    }
    grep -Fq 'enableGenericMiscQuestWideQA' "$stage/Data/PrismaUI_F4/views/K2040QuestJournal/app.js"
fi
if [[ "$repeatable_flag" == y ]];then
    grep -aFq 'QuestRepeatableQA: SetActive RETURNED' "$dll" || {
        echo 'BLOCKED: repeatable Misc QA controller absent' >&2;exit 1;
    }
    if grep -aFq 'QuestNativeWriteQA: invoking ONE verified native SetActive' "$dll";then
        echo 'BLOCKED: competing one-shot writer in repeatable QA';exit 1
    fi
    grep -Fq 'enableRepeatableMiscTrackingQA' "$stage/Data/PrismaUI_F4/views/K2040QuestJournal/app.js"
fi
test -f "$stage/Data/F4SE/Plugins/K2040_Quest_Journal_Write_QA.ini"
grep -q '^IHaveBackedUpTestSave=NO$' "$stage/Data/F4SE/Plugins/K2040_Quest_Journal_Write_QA.ini"
grep -q '^PermitOneNativeSetActiveCall=NO$' "$stage/Data/F4SE/Plugins/K2040_Quest_Journal_Write_QA.ini"
grep -q '^PermitRepeatableQuestTracking=NO$' "$stage/Data/F4SE/Plugins/K2040_Quest_Journal_Write_QA.ini"
test -f "$stage/Data/MCM/Config/K2040_Mods/config.json" || {
    # MCM layouts may use JSON files with different letter case,
    # but both bundles must have staged actual files.
    test "$(find "$stage/Data/MCM/Config/K2040_Mods" -type f | wc -l)" -gt 0
}
test "$(find "$stage/Data/MCM/Config/K2040_Quest_Journal" -type f | wc -l)" -gt 0
echo "PASS: isolated full Journal QA build + Prisma/MCM assets staged"
echo "QA output: $stage/Data"
echo 'No game or Amethyst files deployed; write QA consents remain NO.'
