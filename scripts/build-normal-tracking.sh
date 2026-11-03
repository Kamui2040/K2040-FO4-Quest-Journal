#!/usr/bin/env bash
# Standard, nondeploying OG163 Journal build with the accepted R2 native
# tracking controller explicitly selected; no experimental QA variant.
set -euo pipefail
set +x
root="$(cd -- "$(dirname -- "$0")/.." && pwd -P)"
export K2040_QUESTWIDE_NORMAL_TRACKING=YES
exec bash "$root/scripts/build-linux.sh"
