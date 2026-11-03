# Compatibility and boundaries

## Supported configuration

- Fallout 4 original 1.10.163, F4SE 0.6.23 and matching original Address Library.
- PrismaUI_F4 Modern Core, View and Interop APIs matching the pinned upstream SDK header described in `external/prismaui_f4/README.md`.
- MCM optional; standard settings and original keyboard shortcuts work without MCM. An optional controller interface is used when PrismaUI supplies it.
- Ordinary and Misc **quest-wide** tracking with native readback for eligible, unambiguous quests. HUD pinning and Pip-Boy mirroring are independently configurable.

## Not supported or not verified

- Anniversary Edition / Creation Club runtime changes, Fallout 4 VR, mismatched F4SE and PrismaUI builds.
- Independent per-objective marker toggling for separate sibling Misc objectives; ambiguous radiant or multi-instance quest identities fail closed.
- History for quests never observed by the mod, arbitrary user-created folder names or direct MAP76 map focus.
- Exhaustive compatibility with every third-party quest mod or very large load order. The model includes synthetic high-volume tests; these do not establish in-game rendering performance.

## Safety properties

The old Papyrus VM-dispatched tracking path is disabled. Only the separately reviewed OG163 native quest-wide tracking path can request a tracking change, subject to fresh game-thread validation, single-ticket lifecycle handling and independent native/Pip-Boy confirmation. A prewrite rejection cannot mutate quest state; an uncertain postwrite result blocks further writes in that process rather than blindly retrying. Quest stages, quest completion and script-owned objective progress are not edited by the journal.

For bug reports, include the supported runtime and installed mod versions, expected versus observed behavior and anonymized logs. Never attach personal saves or identifiers in public issues unless you explicitly choose to share them.
