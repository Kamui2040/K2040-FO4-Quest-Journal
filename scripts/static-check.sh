#!/usr/bin/env bash
set -euo pipefail
set +x
cd "$(dirname "$0")/.."
node --check view/quest-model.js
node --check view/demo-data.js
node --check view/app.js
node --test tests/*.test.cjs
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    git diff --check
fi
printf '%s\n' 'PASS: browser-side JS syntax and pure model tests. This is NOT a native build or Fallout 4 runtime test.'

# Native co-save behavior test with deliberately minimal test-only F4SE stubs.
if command -v c++ >/dev/null 2>&1; then
  bin="$(mktemp)"
  trap 'rm -f "$bin"' EXIT
  c++ -std=c++20 -Wall -Wextra -Werror -Itests/stubs -Isrc \
    src/QuestViewState.cpp tests/native_view_state.cpp -o "$bin"
  "$bin"
  printf '%s\n' 'PASS: isolated co-save test stubs (NOT native game ABI).'
fi
