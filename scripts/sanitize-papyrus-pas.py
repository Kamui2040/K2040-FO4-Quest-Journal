#!/usr/bin/env python3
"""Normalize official Fallout 4 Papyrus assembly metadata before PEX packaging.

Only alters the three .info header identity fields; aborts on an unknown
header layout. Does not require or distribute the Bethesda toolchain.
"""
import argparse
import pathlib
import re


def sanitize(source: str, script_name: str) -> str:
    if not re.fullmatch(r"K2040QuestJournal(?:Native|Worker)", script_name):
        raise ValueError("unexpected helper script")
    if source.count(".endInfo") != 1:
        raise ValueError("missing or repeated Papyrus .info section")
    header, tail = source.split(".endInfo", 1)
    if not header.lstrip().startswith(".info"):
        raise ValueError("Papyrus .info must precede object tables")
    for field, value in (
        ("source", script_name + ".psc"),
        ("user", "K2040"),
        ("computer", "K2040BUILD"),
    ):
        pattern = rf'(?m)^(\s*\.{field}\s+)"[^"\r\n]*"'
        header, count = re.subn(
            pattern,
            lambda match: match.group(1) + '"' + value + '"',
            header,
        )
        if count != 1:
            raise ValueError(f"expected exactly one .{field} header field")
    return header + ".endInfo" + tail


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", type=pathlib.Path, required=True)
    parser.add_argument("--out", type=pathlib.Path, required=True)
    parser.add_argument("--object", required=True)
    args = parser.parse_args()
    assert args.src.resolve() != args.out.resolve(), "refuse in-place rewrite"
    source = args.src.read_text(encoding="utf-8-sig")
    result = sanitize(source, args.object)
    args.out.write_text(result, encoding="utf-8", newline="\r\n")
    print("PASS: sanitized Papyrus assembly identity header")


if __name__ == "__main__":
    main()
