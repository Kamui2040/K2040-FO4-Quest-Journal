#!/usr/bin/env python3
"""Stage an explicitly supplied verified Prisma header; never download or deploy."""
import argparse
import os
from pathlib import Path
import tempfile

from prisma_header_pin import official_header_matches, git_blob_sha, UPSTREAM_COMMIT

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "external" / "prismaui_f4" / "PrismaUI_F4_Modern_API.h"


def stage_header(source: Path, destination: Path = DEST) -> str:
    if not source.is_file():
        raise ValueError("Prisma header source does not exist")
    blob = source.read_bytes()
    if not official_header_matches(blob):
        raise ValueError("Source is not the unmodified pinned official modern Prisma header")
    if destination.exists():
        if destination.is_file() and destination.read_bytes() == blob:
            return "already staged"
        raise FileExistsError("An existing different Prisma header must not be overwritten")
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Create in the destination directory for atomic rename; ensure no temporary
    # source from an unsuccessful attempt is left in the repository.
    descriptor, tmp = tempfile.mkstemp(prefix=".prisma-header-", dir=destination.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(blob)
        # A hardlink publishes the completed temporary file atomically and
        # refuses to replace a destination that appeared concurrently.
        # os.replace() would silently overwrite in that race.
        try:
            os.link(tmp, destination)
        except FileExistsError as err:
            raise FileExistsError("Prisma header destination appeared while staging") from err
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)
    return "staged"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path,
                        help="Existing official header file, e.g. from the pinned minimap repo")
    args = parser.parse_args()
    try:
        result = stage_header(args.source)
    except (OSError, ValueError) as err:
        parser.exit(1, "BLOCKED: " + str(err) + "\n")
    print(f"PASS: {result}; Prisma pinned upstream commit {UPSTREAM_COMMIT}; build input only")
    print("No game installation or mod-manager deployment performed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
