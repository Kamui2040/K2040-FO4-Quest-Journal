# Architecture overview

## Optional unified settings entry point

The versioned `K2040_GetSettingsAdapterV1` export offers an optional
settings-only entry point. It opens a new journal panel on the F4SE game
thread, remembers that Settings was requested, and waits for the matching
Prisma DOM-ready generation before switching to the existing Settings tab.
It keeps the current quest/settings ownership, input and cursor protection,
and ordinary Journal and Tracker shortcuts. A missing hub has no effect.
These source changes remain unmerged and untested in-game.


- `src/`: F4SE entry points, game-thread quest observer/collector, read-only data normalization, UI bridge, per-save data, settings, input/MCM integration and defensive tracking policy.
- `research/production_candidate/`: accepted ordinary/Misc quest-wide F4SE native adapter used by the released normal-tracking build.
- `research/native_one_shot/`: native quest scope and guarded repeatable ticket state machine, plus synthetic/legacy research inputs. Research/QA variants are disabled in normal builds.
- `view/`: original PrismaUI_F4 full-screen Journal, miniature tracker, settings model, independent palette presets and themed dropdown controls.
- `mcm/`: canonical `K2040 Mods` grouped parent and Quest Journal child, including optional standalone MCM layout.
- `tests/`: pure C++/Python/JavaScript regressions and synthetic game-state fixtures. Tests never constitute game runtime evidence.
- `scripts/`: pinned dependency staging, cross-build, independent PE export checks, source verification and isolated installation package creation.

## Quest tracking and lifecycle

Build the normal Fallout 4 1.10.163 version with `scripts/build-normal-tracking.sh`. Before tracking a quest, the plugin checks its native quest instance, Pip-Boy entry, and active save/session. Each queued request stays tied to the view that created it and can be cancelled before the native tracking call. A readback checks the result on that same quest. If the result is uncertain, the plugin blocks further tracking changes until the game restarts. It never edits quest stages or completion.

The legacy Papyrus `GameVM` dispatch path stays disabled. The normal unflagged `build-linux.sh` compiles a read-only fallback. External SDK and library inputs are pinned to exact upstream revisions.

## UI behavior

The Journal and tracker save their themes separately. `Commonwealth Blue` keeps the tracker's `commonwealth` value and the Journal's older `dark` value working as before. The default keyboard shortcuts are Shift+G for the Journal and G for the tracker; updates preserve custom shortcuts. MCM keyboard settings stay separate from controller settings. In Journal menus, the D-pad or left stick moves the selection, A chooses, and B goes back. The right stick and mouse can still control the cursor. When a display setting changes, the tracker redraws its current quest list without waiting for a new game update.
