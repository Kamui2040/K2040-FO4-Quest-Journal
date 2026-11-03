# Architecture overview

- `src/`: F4SE entry points, game-thread quest observer/collector, read-only data normalization, UI bridge, per-save data, settings, input/MCM integration and defensive tracking policy.
- `research/production_candidate/`: accepted ordinary/Misc quest-wide F4SE native adapter shared by the opt-in normal-tracking build.
- `research/native_one_shot/`: native quest scope and guarded repeatable ticket state machine, plus synthetic/legacy research inputs. Research/QA variants are disabled in normal builds.
- `view/`: original PrismaUI_F4 full-screen Journal, miniature tracker, settings model, independent palette presets and themed dropdown controls.
- `mcm/`: canonical `K2040 Mods` grouped parent and Quest Journal child, including optional standalone MCM layout.
- `tests/`: pure C++/Python/JavaScript regressions and synthetic game-state fixtures. Tests never constitute game runtime evidence.
- `scripts/`: pinned dependency staging, cross-build, independent PE export checks, source verification and isolated installation package creation.

## Quest tracking and lifecycle

The normal 1.10.163 binary is compiled through `scripts/build-normal-tracking.sh`. Eligible ordinary and Misc quest-wide requests are validated against native quest instance identity, Pip-Boy feed scope and current session/save. Queued requests are bound to the originating view and may be cancelled only before a native setter call. Independent postwrite readback applies to the originally issued ticket, not a newly highlighted quest; uncertain outcomes block further writes in the current process. Native quest completion/stages are never edited.

The legacy Papyrus `GameVM` dispatch path stays disabled. The normal unflagged `build-linux.sh` compiles a read-only fallback. External SDK and library inputs are pinned to exact upstream revisions.

## UI behavior

The full-screen Journal and compact tracker have separate saved theme selections. `Commonwealth Blue` is the presentation name for the tracker's `commonwealth` value and the Journal's legacy `dark` value, preserving existing choices. Keyboard factory defaults are Shift+G (Journal) and G (mini tracker), without overwriting prior remaps. MCM keyboard assignments are grouped separately from controller settings. The tracker redraws its already received quest snapshot immediately when the visible-quest cap or mirror/completed objective visibility changes.
