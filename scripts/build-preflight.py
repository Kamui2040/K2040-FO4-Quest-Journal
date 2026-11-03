#!/usr/bin/env python3
"""Read-only preflight for the external Windows x64 build prerequisites.

Does not download, install, enable CI, build, or modify game files.
"""
import argparse
from prisma_header_pin import official_header_matches
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
COMMONLIB_REV = "12beba2a89fe117a14f1707b88c99ecb1b12f8c0"
SHARED_REV = "f0b1670ee9caac2e349497f6f3c08a69633a8ea7"
HEADER = ROOT / "external" / "prismaui_f4" / "PrismaUI_F4_Modern_API.h"


def revision(folder):
    try:
        result = subprocess.run(["git", "-C", str(folder), "rev-parse", "HEAD"],
                                capture_output=True, text=True, check=True, timeout=10)
        return result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return None


def sdk_include_roots(sdk):
    """Recognize Windows SDK layout without assuming one particular build."""
    if not sdk:
        return []
    kit = Path(sdk) / "Windows Kits" / "10" / "Include"
    if not kit.is_dir():
        return []
    return [p for p in kit.iterdir() if p.is_dir()
            and all((p / name).is_dir() for name in ("shared", "um", "ucrt"))]


def inspect(commonlib, sdk, host, which=shutil.which, get_revision=revision):
    checks = []
    def check(label, ok, detail):
        checks.append((label, bool(ok), detail))
    check("Git available", bool(which("git")), "git required for pinned revision checks")
    check("xmake available", bool(which("xmake")), "xmake 3.0+ required by xmake.lua")
    check("clang-cl available", bool(which("clang-cl")), "LLVM clang-cl compiler required for Linux cross-build")
    common = Path(commonlib) if commonlib else None
    check("CommonLibF4 checkout", bool(common and (common / "xmake.lua").is_file()),
          "Set K2040_COMMONLIBF4_ROOT to the pinned xmake checkout")
    check("CommonLibF4 pinned revision", bool(common and get_revision(common) == COMMONLIB_REV),
          "Expected " + COMMONLIB_REV)
    shared = common / "lib" / "commonlib-shared" if common else None
    check("commonlib-shared pinned revision", bool(shared and get_revision(shared) == SHARED_REV),
          "Expected " + SHARED_REV)
    header_ok = HEADER.is_file() and official_header_matches(HEADER.read_bytes())
    check("Pinned unmodified official Prisma header", header_ok,
          "Stage the pinned upstream header with scripts/stage-prisma-header.py")
    if host == "linux":
        check("Windows SDK include roots", bool(sdk_include_roots(sdk)),
              "Supply --sdk <extracted SDK root> (shared, um, ucrt subdirectories)")
    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--commonlib", default=os.getenv("K2040_COMMONLIBF4_ROOT", ""))
    parser.add_argument("--sdk", default=os.getenv("K2040_WINDOWS_SDK_ROOT", ""))
    args = parser.parse_args()
    checks = inspect(args.commonlib, args.sdk, sys.platform)
    for name, good, detail in checks:
        print(f"{'PASS' if good else 'BLOCKED'}: {name}" +
              ("" if good else f" — {detail}"))
    if any(not good for _, good, _ in checks):
        print("\nBuild prerequisites incomplete. No compilation or installation attempted.")
        return 1
    print("\nPreflight passed; a real xmake Windows x64 build is still required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
