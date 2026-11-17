# PrismaUI build dependency

Quest Journal needs PrismaUI's original API header to compile. The framework and header are **not included** in this source repository.

Use the official, unmodified `PrismaUI_F4_Modern_API.h`:

- [Upstream repository](https://github.com/PRISMA-USER-INTERFACE-FRAMEWORK/Fallout-4-Prisma-UI-Framework)
- Commit: `0daf5e38d3517772c8c28236c8857e5c037fa3bd`
- Path: `src/PrismaUI_F4_Modern_API.h`
- Git blob ID: `9784daab39bf66bb179e6ef12bba63c75e50d3b7`
- License: upstream `LICENSE.md` (the original unmodified header can be distributed with its license; modified framework distributions have restrictions)

To stage an existing original header without downloading or changing it:

```sh
python3 scripts/stage-prisma-header.py --source /path/to/upstream/PrismaUI_F4_Modern_API.h
```

**Why the exact version matters:** The script checks the header's Git blob ID and refuses to overwrite a different file. `build-preflight.py` checks the same version before compilation. This prevents an unnoticed framework update from changing how the plugin is built.

The header is ignored by Git and used **only for compilation**. The script does not commit vendor SDK files or install anything into Fallout 4.
