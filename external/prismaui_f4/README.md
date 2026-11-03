# PrismaUI_F4 build-only SDK input

The Quest Journal deliberately does **not** vendor the Prisma framework or its SDK header. The official unmodified modern API header is a separately obtained build prerequisite and is ignored by Git.

Use the pinned official, unmodified API header from PrismaUI_F4:

- Official upstream: https://github.com/PRISMA-USER-INTERFACE-FRAMEWORK/Fallout-4-Prisma-UI-Framework
- Commit: `0daf5e38d3517772c8c28236c8857e5c037fa3bd`
- Path: `src/PrismaUI_F4_Modern_API.h`
- Git blob ID: `9784daab39bf66bb179e6ef12bba63c75e50d3b7`
- License: upstream `LICENSE.md` (original version may be distributed with its unmodified license; modified framework distributions are restricted)

To stage an existing original header without downloading or modifying it:

```sh
python3 scripts/stage-prisma-header.py --source /path/to/upstream/PrismaUI_F4_Modern_API.h
```

The script verifies the exact pinned upstream Git blob ID and refuses overwrites of a different file. `build-preflight.py` enforces the same pin. No framework source, binaries, or vendor SDK are committed to Quest Journal, and staging never deploys anything to Fallout 4.
