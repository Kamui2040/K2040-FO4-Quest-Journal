#!/usr/bin/env bash
# Source-only recovery inputs for K2040 Quest Journal.
# Requires git, curl, python3; no vendor binaries or game files are copied.
set -euo pipefail
root="$(cd -- "$(dirname -- "$0")/.." && pwd)"
target="${1:-$HOME/Projects/Libraries/QuestJournal/commonlibf4-pinned}"
common_rev=12beba2a89fe117a14f1707b88c99ecb1b12f8c0
shared_rev=f0b1670ee9caac2e349497f6f3c08a69633a8ea7
prisma_rev=0daf5e38d3517772c8c28236c8857e5c037fa3bd
for cmd in git curl python3; do command -v "$cmd" >/dev/null || { echo "Missing $cmd" >&2; exit 1; }; done
if [[ ! -e "$target" ]]; then
  mkdir -p "$(dirname "$target")"
  git clone https://github.com/libxse/commonlibf4.git "$target"
fi
[[ -d "$target/.git" ]] || { echo "Unexpected existing folder $target" >&2; exit 1; }
if [[ "$(git -C "$target" rev-parse HEAD)" != "$common_rev" ]]; then
  [[ -z "$(git -C "$target" status --porcelain)" ]] || { echo "Refusing dirty dependency checkout" >&2; exit 1; }
  git -C "$target" fetch origin "$common_rev"
  git -C "$target" checkout --detach "$common_rev"
fi
git -C "$target" submodule update --init lib/commonlib-shared
[[ "$(git -C "$target" rev-parse HEAD)" == "$common_rev" ]]
[[ "$(git -C "$target/lib/commonlib-shared" rev-parse HEAD)" == "$shared_rev" ]]
header="$root/external/prismaui_f4/PrismaUI_F4_Modern_API.h"
if [[ ! -f "$header" ]]; then
  temp="$(mktemp)"
  trap 'rm -f "$temp"' EXIT
  curl --fail --silent --show-error --location --max-time 60 \
    "https://raw.githubusercontent.com/PRISMA-USER-INTERFACE-FRAMEWORK/Fallout-4-Prisma-UI-Framework/$prisma_rev/src/PrismaUI_F4_Modern_API.h" -o "$temp"
  python3 "$root/scripts/stage-prisma-header.py" --source "$temp"
fi
python3 -c 'import pathlib,sys; sys.path.insert(0,str(pathlib.Path(sys.argv[1])/"scripts"));from prisma_header_pin import official_header_matches;assert official_header_matches(pathlib.Path(sys.argv[2]).read_bytes());print("PASS: pinned original PrismaUI header")' "$root" "$header"
echo "PASS: pinned external source dependencies; CommonLibF4=$target"
echo "NEXT: legitimately obtain Windows MSVC/SDK x64 include/libs, LLVM, xmake, Node."
echo "THEN: export K2040_COMMONLIBF4_ROOT='$target'; see docs/BUILDING.md"
