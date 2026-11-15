# Quest Journal — K2040 Settings Schema V1 inventory

## Unified Editor V2 owner candidate — 2026-11-15

The `feature/unified-editor-v2` development branch exposes the validated
V1 JSON catalog through a versioned native V2 adapter. The separate
Quest Journal settings page and its edit callbacks have been retired.
Changes are validated and persisted by the owner through `ChangeAndSave`
and `ResetToDefaults`; the hub never owns the local INI, MCM keyboard
registry, or per-save quest state.

The common theme is applied as a transient view overlay without rewriting
native Journal or tracker palettes. Global theme selection and individual
per-mod overrides belong to the shared hub renderer, not a new Quest Journal
renderer. The browser-only Journal/tracker smoke tests, synthetic native
suite and Windows x64 F4SE source build passed. This is not in-game QA:
runtime verification, HUD's outstanding DLL and deployment remain pending.

## Source of truth and inventory

- Contract: pinned shared Settings Schema V1, included locally under
  schemas/; validator and schema hashes are verified by source tests.
- Existing runtime owner: `src/JournalSettings.h/.cpp`; on Linux/Proton
  stored in the owning mod's user-local INI, with legacy game-relative import.
- Existing runtime views: `view/journal.html`, `view/journal-settings.js`,
  `view/app.js`. These remain a Journal/tracker only; settings editing happens
  only inside the shared V2 hub renderer.
- Journal/tracker gameplay shortcuts are now owner-local validated
  preferences exposed through the common hub Controls page.
  No Quest Journal child MCM page or registry action is shipped. Legacy
  controller assignments are imported read-only if absent in owner settings.
- Definition: `view/settings-v1-manifest.json`, module ID `quest-journal`.
  `nativeSettingsSchemaVersion: 1` denotes this first stable native preference
  key inventory. The owning native V2 adapter provides the versioned bridge
  while the original per-user INI remains the authoritative data store;
  migrating that INI to JSON is neither necessary nor authorized.

| Future section | Editable fields | Reason |
| --- | ---: | --- |
| General | 2 | Enable tracker; optional Phosphor coexistence |
| Tracker layout | 5 | Position, width, quest limit, completed, new quests |
| Tracker appearance | 7 | Theme, opacity, scale, four custom palette colors |
| Journal appearance | 7 | Independent theme, opacity, font scale, four palette colors |
| Journal layout | 3 | Density, two column widths |
| Controls | 7 | Keyboard shortcuts, controller shortcuts, Quick Data redirect |
| Maintenance | 1 action | Module-owned reset of native preferences |

**Current total:** 31 hub-visible preferences and one reset action.
All 39 native Preferences fields are accounted for: 31 hub-visible,
eight internal. Pip-Boy mirror stays in the Journal; five normalized live
layout values are owned by the adapter and are not duplicated as raw
settings fields in the hub.

## Duplicate, legacy and unsupported controls

- Native `opacity` and `fontSize` survive as older persisted fields.
  Their current visible equivalents are `trackerOpacity` and `trackerScale`.
  Mark them `internalKeys`: never delete, migrate or reset them merely
  because their controls are hidden. A separately requested full native
  preferences reset still uses the owner's established behavior.
- The Journal and tracker keyboard shortcuts (defaults Shift+G / G)
  are now hub-editable owner values, not native MCM bindings.
- Four owner-local controller select fields replace obsolete MCM dropdowns.
  Existing user MCM controller INI values are imported read-only when
  no matching owner preference exists; the original files are preserved.
- The live HUD layout editor is available through the canonical V2
  owner adapter, not new manifest fields. One supported element,
  quest-tracker, exposes proportional position/size. The owner stores
  trackerLayoutEnabled and integer viewport basis-point coordinates
  (trackerLayoutX/Y/Width/Height) only after Save, validates on load,
  and never modifies Fallout save data. These five keys are internal.
  Legacy anchor/width fields explicitly disable custom geometry when
  edited, so the Settings Hub numeric controls remain effective.
- Pip-Boy synchronization remains a direct Journal gameplay control.
  The native owner validates, saves and immediately applies the checkbox,
  with failure feedback. The hub explicitly rejects writing its internal
  trackerSyncPipboy preference; gameplay quest-tracking flags never change.
- The former per-mod color editor exposed color picker, hex input and RGB
  sliders for each color. These were alternate presentations of **one native
  color value**, not extra settings. One color field per value is now rendered
  centrally through K2040 Unified Settings V2.
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
  which resets hub-exposed preferences and gameplay shortcuts. The
  Journal-owned Pip-Boy mirror remains unchanged. Legacy MCM data and
  save-specific quest state remain outside its scope. Only the existing
  owner-controlled V2 adapter may invoke this action.

## Compatibility and next integration gates

The manifest is packaged with the mod and read by its native V2 adapter.
The existing shared K2040 renderer handles the integrated two-pane editor
for every participating mod; there is no separate per-mod settings renderer.
Live reads/writes, validation, preview/save/reset, native side effects and
thread-affinity must be handled by an owner-controlled, versioned adapter,
never by direct edits from the renderer. An incompatible adapter fails closed.

The retired settings UI is removed from this development branch. Windows
x64 ABI and build checks passed, but do not deploy, merge or release until
the shared editor and every participating mod pass independent Fallout 4
OG 1.10.163 game testing. Existing unmerged feature worktrees are independent;
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
