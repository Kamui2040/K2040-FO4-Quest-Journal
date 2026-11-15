# Architecture overview

- `src/`: F4SE entry points, game-thread quest observer/collector, read-only data normalization, UI bridge, per-save data, settings, input/MCM integration and defensive tracking policy.
- `research/production_candidate/`: accepted ordinary/Misc quest-wide F4SE native adapter used by the released normal-tracking build.
- `research/native_one_shot/`: native quest scope and guarded repeatable ticket state machine, plus synthetic/legacy research inputs. Research/QA variants are disabled in normal builds.
- `view/`: original PrismaUI_F4 full-screen Journal and passive tracker, independent legacy appearance normalization and gamepad menu controls. The former per-mod settings editor UI has been retired; the shared K2040 hub owns the only editor surface.
- `view/settings-v1-manifest.json`: validated K2040 Settings Schema V1 metadata used by the **shared V2 renderer** through this mod's `K2040_GetSettingsAdapterV2` native export. The hub does not directly own or overwrite native preference storage. V2 adapter and query ABI headers must match the hub's checked-in originals byte-for-byte.
- MCM staging includes only the canonical K2040 Mods parent.
  Quest Journal has no child MCM entry or gameplay keybind registry.
  The Unified Settings hub owns the single configurable MCM shortcut.
  Owner-local Journal/tracker shortcuts are edited through the hub.
- `tests/`: pure C++/Python/JavaScript regressions and synthetic game-state fixtures. Tests never constitute game runtime evidence.
- `scripts/`: pinned dependency staging, cross-build, independent PE export checks, source verification and isolated installation package creation.

## Quest tracking and lifecycle

Build the normal Fallout 4 1.10.163 version with `scripts/build-normal-tracking.sh`. Before tracking a quest, the plugin checks its native quest instance, Pip-Boy entry, and active save/session. Each queued request stays tied to the view that created it and can be cancelled before the native tracking call. A readback checks the result on that same quest. If the result is uncertain, the plugin blocks further tracking changes until the game restarts. It never edits quest stages or completion.

The legacy Papyrus `GameVM` dispatch path stays disabled. The normal unflagged `build-linux.sh` compiles a read-only fallback. External SDK and library inputs are pinned to exact upstream revisions.

## UI behavior

The Journal and tracker save their themes separately. `Commonwealth Blue` keeps the tracker's `commonwealth` value and the Journal's older `dark` value working as before. The default keyboard shortcuts are Shift+G for the Journal and G for the tracker; updates preserve custom shortcuts. MCM keyboard settings stay separate from controller settings. In Journal menus, the D-pad or left stick moves the selection, A chooses, and B goes back. The right stick and mouse can still control the cursor. When a display setting changes, the tracker redraws its current quest list without waiting for a new game update.

The optional K2040LayoutAdapterV2 export is an independent feature of the
Quest Journal owner DLL. Only the existing compact tracker is a movable
HUD element. Its DOM reports current normalized viewport bounds, the owner
validates hub-originated geometry and applies reversible preview to the
tracker view, and only a successful Save commits all four integer viewport
basis-point coordinates atomically into the existing per-user INI. Legacy
anchor/width controls disable custom placement when changed. Neither
preview nor abort changes the Pip-Boy one-way mirror, quest save, or HUD
tracker's individual gameplay hotkeys. The hub owns all overlay input.
