# Quest Journal — K2040 Settings Schema V1 inventory

**Status (2026-11-15):** source-only settings catalog. The existing PrismaUI
settings screen, MCM pages, saved INI files, keybind behavior and optional
Unified Settings V1 adapter remain untouched. No game deployment or runtime
acceptance has occurred.

## Source of truth and inventory

- Contract: `K2040-Unified-Settings/docs/SETTINGS_SCHEMA_V1.md` and its
  Draft 2020-12 schema, copied byte-for-byte into `schemas/`.
- Existing runtime owner: `src/JournalSettings.h/.cpp`; on Linux/Proton
  stored in the owning mod's user-local INI, with legacy game-relative import.
- Existing UI: `view/journal.html`, `view/journal-settings.js`, `view/app.js`.
- MCM keyboard/controller bindings: `mcm/**/K2040_Quest_Journal/`.
- Definition: `view/settings-v1-manifest.json`, module ID `quest-journal`.
  `nativeSettingsSchemaVersion: 1` denotes this first stable native preference
  key inventory; the current INI does **not** yet implement a versioned schema
  handshake, native V2 editor adapter or JSON persistence.

| Future section | Editable fields | Reason |
| --- | ---: | --- |
| General | 2 | Enable tracker; optional Phosphor coexistence |
| Tracker layout | 6 | Position, width, quest limit, completed, new quests, Pip-Boy sync |
| Tracker appearance | 7 | Theme, opacity, scale, four custom palette colors |
| Journal appearance | 7 | Independent theme, opacity, font scale, four palette colors |
| Journal layout | 3 | Density, two column widths |
| Controls | 1 | Quick Data redirect respects the game's effective binding |
| Maintenance | 1 action | Module-owned reset of native preferences |

**Total:** 26 editable persisted preferences, one reset action. All 30
`Preferences` members are accounted for: 26 visible and 4 internal.

## Duplicate, legacy and unsupported controls

- Native `opacity` and `fontSize` survive as older persisted fields.
  Their current visible equivalents are `trackerOpacity` and `trackerScale`.
  Mark them `internalKeys`: **do not delete, migrate or reset them**.
- `journalHotkey` and `trackerHotkey` remain persisted standalone fallbacks,
  not duplicate controls in the future common editor. Existing standalone
  keyboard selectors remain usable; MCM hotkey registry continues to own its
  separate assignments. They are `internalKeys` in the new catalog.
- Controller action/held-button settings are MCM-owned keys
  (`iQuest*Pad*:` in `settings.ini`), not native `Preferences` properties.
  The V1 schema cannot represent MCM hotkey or controller capture as a value
  control. Do not invent surrogate select fields or alter these assignments.
- The current live color editor exposes color picker, hex input and three RGB
  sliders for each color. These are alternate presentations of **one native
  color value**, not additional settings: each color receives one manifest
  field. Existing editor widgets remain in place until renderer QA.
- `hudColor`, `hudBackgroundColor` and `pipboyColor` are game-derived,
  read-only rendering inputs, never writable or stored preferences.
- The two theme keys and custom palettes **must remain independent**.
  Legacy `journalTheme=default` is normalized by the native owner; do not
  expose a redundant `default` choice or silently rewrite saved theme data.
- `trackerOverridePhosphor` is intentionally a distinct opt-in, not
  redundant with `trackerEnabled`; it only affects Quest Journal's tracker.
- `redirectQuestKey` is a behavior preference, **not** a new standalone
  key assignment: it follows Fallout 4's current Quick Data mapping.
- The V1 `visibleWhen` contract references only toggles. Tracker fields
  depend on `trackerEnabled`, but custom color fields cannot use a
  `theme=custom` select condition. They are advanced fields; help text says
  when they apply. Hidden/inactive values are never removed from storage.
- The reset action describes the existing native `ResetToDefaults` operation,
  which also resets standalone fallback shortcuts. MCM binding records and
  save-specific quest state are outside its scope. It is metadata only until
  an explicitly supported native edit adapter exists.

## Compatibility and next integration gates

The definition is intentionally **not loaded** by the old standalone UI.
A future shared K2040 renderer must render both the mod's settings page and
the hub's integrated two-pane editor from this same validated manifest.
Live reads/writes, validation, preview/save/reset, native side effects and
thread-affinity must be handled by an owner-controlled, versioned adapter,
never by direct edits from the renderer. An incompatible adapter fails closed.

Do not remove the old UI, change keybinds, or deploy until the shared renderer,
adapter and independent Fallout 4 OG 1.10.163 runtime QA pass. Existing
unmerged controller and adapter feature worktrees are independent changes;
this catalog does not imply their gameplay acceptance or merge.

## Source validation

```bash
npm ci --ignore-scripts --no-audit --no-fund
npm run validate:settings
npm test
bash scripts/verify-source.sh
```

Pinned AJV 8.20.0 is a **development-only** dependency, not part of the
Fallout 4 package. `verify-source.sh` fails closed if it is absent, and runs
both the V1 validator and the existing regression suites. The targeted tests
verify native-to-manifest coverage, choices/ranges/steps, disabled-field
retention, isolated hotkeys, schema hashes and invalid-manifest rejection.
