# Quest Journal settings catalog

**Development only.** These are the settings offered through K2040 Unified Settings V2, not the menu in the public v0.0.2 release. The Journal still owns its preferences; the hub only provides a common screen.

This work began on `feature/unified-editor-v2` and is included in the current R11 branch. Source and Windows build checks have passed, but the combined setup is **not yet accepted in-game**.

## Where settings come from

- `view/settings-v1-manifest.json` lists the controls and allowed values, under module ID `quest-journal`.
- `schemas/` contains the pinned Settings Schema V1 contract.
- `src/JournalSettings.h/.cpp` loads and saves preferences in the Journal's user-local INI, importing older game-relative settings when necessary.
- `K2040_GetSettingsAdapterV2` connects the Journal to the shared editor. Native `ChangeAndSave` and `ResetToDefaults` make the actual changes.

**Why:** The hub must not write directly into another mod's configuration. Quest Journal checks every change itself and rejects incompatible adapters. `nativeSettingsSchemaVersion: 1` identifies the settings format; it does not authorize switching the INI to JSON.

## What players can edit

| Category | Fields | Purpose |
| --- | ---: | --- |
| General | 2 | Enable tracker; allow it alongside Phosphor HUD |
| Tracker layout | 5 | Position, width, quest limit, completed/new quests |
| Tracker appearance | 7 | Theme, opacity, scale, four custom colors |
| Journal appearance | 7 | Separate theme, opacity, font size, four colors |
| Journal layout | 3 | Density and two column widths |
| Controls | 7 | Gameplay keyboard/controller shortcuts and Quick Data redirect |
| Maintenance | 1 action | Reset Journal preferences |

That is **31 visible preferences and one reset action**, from **39 native preference fields**. The other eight are internal. Pip-Boy synchronization and the layout editor's coordinates are handled separately, not duplicated as ordinary hub controls.

## Saved settings that must survive

- Older `opacity` and `fontSize` values are kept as `internalKeys`; the visible tracker fields are `trackerOpacity` and `trackerScale`. Hiding an old field must not delete its data.
- Gameplay shortcuts still default to **Shift+G** for the Journal and **G** for the tracker. The hub edits them, but they remain Journal-owned values rather than independent MCM keybinds.
- Four controller selection values replace old MCM dropdowns. If no newer value exists, old controller INI values can be read as a fallback without rewriting the original files.
- The native code normalizes `journalTheme=default`. Do not expose a duplicate Default choice or rewrite a user's saved theme just to rename it.
- Journal and tracker themes/colors stay separate. Shared frame styles are temporary overlays; they never replace saved palette data.
- `trackerOverridePhosphor` is not the same as `trackerEnabled`: it only decides whether Quest Journal's tracker can appear with Phosphor's HUD.
- `redirectQuestKey` follows Fallout 4's **Quick Data** mapping. It is not a new standalone keyboard shortcut.
- `hudColor`, `hudBackgroundColor`, and `pipboyColor` are read from the game for display, not writable settings.

The old color editor offered a picker, hex input, and RGB sliders for each color. Those were three ways to change the **same value**, not three settings. The hub shows one field for each color.

## Pip-Boy synchronization

The checkbox stays directly in the Journal. The plugin saves and applies that choice and refreshes the tracker. The hub must refuse writes to internal `trackerSyncPipboy`, and a hub Reset must leave it alone.

**Why:** Showing a quest in the tracker is a display choice. It must not silently change the quests tracked by Fallout 4.

## Tracker placement

The hub's layout editor works through a native V2 adapter for the single item `quest-tracker`. Quest Journal stores `trackerLayoutEnabled`, `trackerLayoutX`, `trackerLayoutY`, `trackerLayoutWidth`, and `trackerLayoutHeight` as internal fields. The four coordinates are integer **basis points** (10,000 = the full viewport).

- **Save** validates and writes the new position/size to the Journal INI.
- **Abort** restores saved placement without writing.
- **Retry** restores the starting layout and keeps editing.
- Changing older anchor/width controls returns to legacy placement.

These previews do not change quest saves or Pip-Boy sync.

## Conditional controls and reset

V1 `visibleWhen` conditions support toggle fields. Tracker settings can depend on `trackerEnabled`, but color fields cannot use a `theme=custom` select condition. Help text tells players when a color applies. Hidden fields retain their saved data.

`ResetToDefaults` resets hub-exposed preferences and gameplay shortcuts. It does not erase legacy MCM files, per-save quest state, or the Journal's Pip-Boy sync choice. Live reads, writes, native side effects, and required game-thread calls remain with the Journal adapter.

## Verification

```bash
npm ci --ignore-scripts --no-audit --no-fund
npm run validate:settings
npm test
bash scripts/verify-source.sh
```

The schema validator uses pinned **AJV 8.20.0**, a development-only dependency. Tests cover field names, types, ranges, choices, conditional visibility, value retention, schema hashes, and invalid manifest rejection.

Source and Windows x64 ABI/build checks are not enough to accept the mod. The hub interface, save/reload, controllers, themes, and tracker layout still require Fallout 4 1.10.163 testing. See [the QA checklist](UNIFIED_SETTINGS_VNEXT_QA.md).
