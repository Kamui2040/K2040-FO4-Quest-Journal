#!/usr/bin/env python3
"""Pin a build-only, unmodified official Prisma modern header by Git blob SHA."""
import hashlib

UPSTREAM_COMMIT = "0daf5e38d3517772c8c28236c8857e5c037fa3bd"
OFFICIAL_HEADER_BLOB = "9784daab39bf66bb179e6ef12bba63c75e50d3b7"


def git_blob_sha(content: bytes) -> str:
    """Git's blob ID includes a length-prefixed type marker, not just SHA1(data)."""
    prefix = b"blob " + str(len(content)).encode("ascii") + b"\0"
    return hashlib.sha1(prefix + content).hexdigest()


def official_header_matches(content: bytes) -> bool:
    return (len(content) == 21201 and
            git_blob_sha(content) == OFFICIAL_HEADER_BLOB and
            b"namespace PRISMA_UI_FLAT_API" in content and
            b"PrismaUI_F4_GetAPI" in content)
