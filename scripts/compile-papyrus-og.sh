#!/usr/bin/env bash
# Offline, source-only Fallout 4 OG Papyrus build for research scripts.
# Stages original CK inputs outside repo/game, sanitizes PEX identity header,
# does not create a mod plugin, install game files or enable quest tracking.
set -euo pipefail

root="$(cd -- "$(dirname -- "$0")/.." && pwd -P)"
if [[ -z "${K2040_FO4_GAME_ROOT:-}" || -z "${K2040_PAPYRUS_STAGE:-}" ]]; then
    echo 'Set K2040_FO4_GAME_ROOT and K2040_PAPYRUS_STAGE.' >&2
    exit 2
fi
for tool in wine winepath unzip sha256sum python3 timeout mktemp; do
    command -v "$tool" >/dev/null || { echo "Missing $tool" >&2; exit 2; }
done
game="$(realpath -e "$K2040_FO4_GAME_ROOT")"
mkdir -p -- "$K2040_PAPYRUS_STAGE"
stage="$(realpath -e "$K2040_PAPYRUS_STAGE")"
if [[ "$stage" == / || "$stage" == "$HOME" ||
      "$stage" == "$root" || "$stage/" == "$root/"* ||
      "$stage" == "$game" || "$stage/" == "$game/"* ||
      "$root/" == "$stage/"* || "$game/" == "$stage/"* ]]; then
    echo 'Stage must be a separate dedicated directory outside game/repo.' >&2
    exit 2
fi

# The CK executable hash is for Steam depot 1946161 manifest
# 6928748513006443409, Creation Kit FileVersion 1.10.162.0.
ck_hash='00d0941de112e761bb33e6d6eb3c5e3f342b17add1059eca1593ea1abf0be3ac'
actual_hash="$(sha256sum "$game/CreationKit.exe" | cut -d' ' -f1)"
if [[ "$actual_hash" != "$ck_hash" ]]; then
    echo 'Creation Kit is not the verified 1.10.162.0 depot build.' >&2
    exit 2
fi
compiler="$game/Papyrus Compiler/PapyrusCompiler.exe"
assembler="$game/Papyrus Compiler/PapyrusAssembler.exe"
archive="$game/Data/Scripts/Source/Base/Base.zip"
for file in "$compiler" "$assembler" "$archive"; do
    [[ -s "$file" && ! -L "$file" ]] ||
        { echo 'A required OG Creation Kit input is missing.' >&2; exit 2; }
done

work="$(mktemp -d "$stage/run-XXXXXXXX")"
mkdir -p "$work"/{tools,imports,source,raw,pex,logs}
cp -p -- "$game/Papyrus Compiler/"* "$work/tools/"
cp -p -- "$root/research/papyrus/"*.psc "$work/source/"
unzip -qq "$archive" -d "$work/imports"
for src in Quest.psc Game.psc Institute_Papyrus_Flags.flg; do
    [[ -s "$work/imports/$src" ]] ||
        { echo "Missing vanilla source import: $src" >&2; exit 2; }
done

export WINEPREFIX="$stage/wineprefix"
export WINEDEBUG=-all
src_win="$(winepath -w "$work/source")"
imports_win="$(winepath -w "$work/imports")"
raw_win="$(winepath -w "$work/raw")"
flags_win="$(winepath -w "$work/imports/Institute_Papyrus_Flags.flg")"

# The -asmonly flag is deliberate: never produce an unsanitized PEX.
(
    cd "$work/tools"
    for name in K2040QuestJournalNative K2040QuestJournalWorker; do
        timeout --signal=TERM --kill-after=3s 60s wine PapyrusCompiler.exe "$name" \
            "-i=$src_win;$imports_win" "-o=$raw_win" "-f=$flags_win" \
            -asmonly >"$work/logs/$name.compile.log" 2>&1 ||
            { echo "Papyrus compile failed: $name" >&2; exit 1; }
        [[ -s "$work/raw/$name.pas" ]] ||
            { echo "No compiler assembly: $name" >&2; exit 1; }
        python3 "$root/scripts/sanitize-papyrus-pas.py" \
            --src "$work/raw/$name.pas" --out "$work/pex/$name.pas" \
            --object "$name" >"$work/logs/$name.sanitize.log"
        (
            cd "$work/pex"
            timeout --signal=TERM --kill-after=3s 60s wine \
                "$work/tools/PapyrusAssembler.exe" "$name" \
                >"$work/logs/$name.assemble.log" 2>&1
        ) || { echo "Sanitized Papyrus assemble failed: $name" >&2; exit 1; }
        [[ -s "$work/pex/$name.pex" ]] ||
            { echo "No sanitized bytecode: $name" >&2; exit 1; }
    done
)

python3 - "$work" <<'PY'
import pathlib, sys
work = pathlib.Path(sys.argv[1])
for name in ("K2040QuestJournalNative", "K2040QuestJournalWorker"):
    pex = (work / "pex" / f"{name}.pex").read_bytes()
    if any(s in pex for s in (b"Z:\\", b"/home/", b"Downloads",
                              b"NOBARA-PC", b".user \"k2040\"")):
        raise SystemExit(f"Private build metadata leaked: {name}")
    for required in (name.encode(), b"K2040BUILD", f"{name}.psc".encode()):
        if required not in pex:
            raise SystemExit(f"Unexpected PEX identity header: {name}")
    original = (work / "raw" / f"{name}.pas").read_text()
    sanitized = (work / "pex" / f"{name}.pas").read_text()
    assert original.split(".endInfo", 1)[1] == sanitized.split(".endInfo", 1)[1]
    print(f"PASS: {name} sanitized PEX, {len(pex)} bytes")
PY

# Unsanitized compiler .pas metadata must never be mistaken for deliverables.
rm -f -- "$work/raw/K2040QuestJournalNative.pas" \
    "$work/raw/K2040QuestJournalWorker.pas"
echo 'PASS: isolated official OG Papyrus compile and sanitized assembly.'
echo "Compiled files: $work/pex"
echo 'Research only: NO helper ESP, installation, deployment or game test.'
