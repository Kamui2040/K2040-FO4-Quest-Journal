# Changelog

## v0.0.3 — in development, not released

### Quest list and tracking

- Keep the last usable quest list when incoming data has the wrong structure. A normal save change still clears and reloads the list.
- Keep existing tracker settings if an incoming settings message is malformed.
- Check a quest instance once per update instead of repeating the lookup.
- Make large lists faster to search and sort, especially when a Misc quest has several entries.
- Optionally use Fallout 4's Quick Data action (usually J) to open the Journal. This follows your current key mapping and starts off.

### Other mods and settings

- Hide the mini tracker by default when Phosphor HUD's live HUD is active. **Show Quest Journal tracker with Phosphor HUD** lets you display it anyway; it remembers your choice. The full Journal is unchanged.
- Move Journal settings into the separately installed K2040 Unified Settings hub. The Journal's **Settings** button opens its page in the hub; it no longer ships its own MCM settings entry.
- Add four shared Framework frame styles to the full Journal and tracker, without overwriting saved colors.
- Add the hub's layout editor for the compact tracker. Preview position and size changes, then Save, Abort, or Retry.

### Controls

- Use the D-pad or left stick to move through Journal controls.
- Follow Fallout 4's mapped Confirm and Back buttons, including Switch Pro/Steam Input where supported, rather than fixed Xbox labels.
- Stop controller-generated arrow keys from accidentally changing an open category menu.

These changes have source and build checks; **full in-game acceptance is still pending**.

## v0.0.2 — first public release

- Full-screen journal with search, categories, favorites, and quest objectives.
- Compact tracker with adjustable position, size, and visible quest count.
- Quest tracking alongside the Pip-Boy.
- Separate themes and custom colors for Journal and tracker.
- Default keys: **Shift + G** for the Journal, **G** for the tracker.
- Journal settings, optional MCM, clearer dropdowns, and immediate tracker updates.

Requires original Fallout 4 **1.10.163**, F4SE, Address Library, and PrismaUI for Fallout 4.
