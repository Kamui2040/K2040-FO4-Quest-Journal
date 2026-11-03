#!/usr/bin/env python3
"""Verify a staged native Windows DLL without packaging or installing anything."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / "dist" / "Data" / "F4SE" / "Plugins" / "K2040_Quest_Journal.dll"

# Import the existing, tested PE export parser; do not maintain a second
# implementation with subtly different loader validation rules.
spec = importlib.util.spec_from_file_location("journal_test_packager",
                                             ROOT / "scripts" / "stage-test-package.py")
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


# Fingerprints are a second defense against shipping an experimental
# tracking flavor accidentally. No negative string scan can prove the absence
# of executable code; xmake's QA-off flags and the source-only backend gate
# remain mandatory independently.
FORBIDDEN_SHIPPING_MARKERS = (
    b"QuestRepeatableQA: SetActive ENTER",
    b"QuestWideCandidate: SetActive RETURNED",
    b"QuestGenericMiscQA: returned",
    b"QuestNativeWriteQA: ONE",
    b"DogmeatProduction: narrow candidate",
    b"SingleMiscQA: one-shot",
)

def reject_experimental_writer_markers(binary: bytes) -> None:
    for marker in FORBIDDEN_SHIPPING_MARKERS:
        if marker in binary:
            raise ValueError("experimental native tracking flavor present in shipping DLL")

# The same in-game R2 native writer source is compiled into an isolated
# test stage or the opt-in normal distribution stage. Do not mistake an
# arbitrary QA writer for the specifically accepted quest-wide native path.
VERIFIED_QUESTWIDE_MARKERS = (
    b"QuestWideCandidate: SetActive ENTER",
    b"QuestWideCandidate: SetActive RETURNED",
    b"QuestWideCandidate: independent native/Pip-Boy readback confirmed",
)
OTHER_WRITER_MARKERS = tuple(marker for marker in FORBIDDEN_SHIPPING_MARKERS
                             if marker != b"QuestWideCandidate: SetActive RETURNED")

def check_dll(path: Path, *, require_readonly_shipping: bool = False,
              require_questwide_tracking: bool = False):
    if path.is_symlink() or not path.is_file():
        raise ValueError("expected a regular Windows DLL, not a symlink or missing file")
    binary = path.read_bytes()
    package.inspect_pe(binary)
    if require_readonly_shipping and require_questwide_tracking:
        raise ValueError("contradictory read-only and quest-wide writer requirements")
    if require_readonly_shipping:
        reject_experimental_writer_markers(binary)
    if require_questwide_tracking:
        for marker in VERIFIED_QUESTWIDE_MARKERS:
            if marker not in binary:
                raise ValueError("accepted R2 quest-wide native writer missing")
        for marker in OTHER_WRITER_MARKERS:
            if marker in binary:
                raise ValueError("QA or experimental writer present beside quest-wide writer")
    return {"size": len(binary), "sha256": hashlib.sha256(binary).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, default=DEFAULT)
    parser.add_argument("--require-readonly-shipping", action="store_true",
                        help="reject known experimental writer fingerprints")
    parser.add_argument("--require-questwide-tracking", action="store_true",
                        help="require accepted R2 quest-wide native path and reject other writers")
    args = parser.parse_args()
    try:
        info = check_dll(args.dll, require_readonly_shipping=args.require_readonly_shipping,
                         require_questwide_tracking=args.require_questwide_tracking)
    except (ValueError, OSError) as error:
        print("BLOCKED: invalid Quest Journal DLL:", error, file=sys.stderr)
        return 1
    print("PASS: PE32+ x64 DLL with both non-forwarded F4SE loader exports")
    if args.require_readonly_shipping:
        print("PASS: known experimental native writer fingerprints absent")
    if args.require_questwide_tracking:
        print("PASS: accepted R2 quest-wide writer present, competing writers absent")
    print("DLL SHA-256:", info["sha256"])
    print("DLL bytes:", info["size"])
    print("Not installed or game-tested; imports and F4SE runtime ABI require separate review.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
