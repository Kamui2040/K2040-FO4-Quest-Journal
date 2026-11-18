# How Quest Journal works

Quest Journal reads Fallout 4's quest information, shows it in PrismaUI, and lets the player request supported tracking changes. It does **not** change quest stages or complete objectives.

## Where things live

| Path | Purpose |
| --- | --- |
| `src/` | F4SE plugin: quest collection, tracking checks, settings, input, and the bridge to the UI |
| `view/` | Journal and mini tracker shown through PrismaUI |
| `view/settings-v1-manifest.json` | List of settings made available to the separate Unified Settings V2 editor |
| `research/production_candidate/` | Native tracking adapter used by the normal tracking build |
| `research/native_one_shot/` | Repeatable/Misc tracking research and test cases; experimental paths are disabled in normal builds |
| `scripts/` | Dependency checks, builds, package creation, and DLL inspection |
| `tests/` | JavaScript, Python, and synthetic C++ tests; these do not run inside Fallout 4 |

## Quest tracking

Build the normal Fallout 4 1.10.163 plugin with `scripts/build-normal-tracking.sh`.

Before requesting a tracking change, the plugin checks the quest instance, the Pip-Boy entry, and the current save/session. It associates each request with the Journal view that issued it, and it can cancel a pending request before touching tracking state. Afterward, it reads back the result for the *same quest*. If that result cannot be trusted, further tracking changes are blocked until the game restarts.

**Why:** A stale quest selection or a result from the wrong save should never cause the plugin to change a different quest. Quest stages and completion are never edited.

The old Papyrus `GameVM` dispatch path is disabled. Running `build-linux.sh` with no tracking option produces a **read-only fallback**, not the normal gameplay build. External SDKs are pinned to known revisions.

## Journal, tracker, and controls

The Journal and tracker keep separate theme and color settings. Old saved theme values still work: for example, the tracker's `commonwealth` and Journal's `dark` values both map to Commonwealth Blue. Changing a display setting redraws the tracker immediately.

The default gameplay keys are **Shift+G** (Journal) and **G** (tracker). Saved custom shortcuts survive updates. The Journal's menus use the game's current ControlMap for Confirm and Back, including Steam Input where Fallout 4 exposes it. The D-pad and left stick navigate; the right stick or mouse can move the cursor. For XInput, moving the left stick up gives a positive Y value.

The old Journal-only settings editor and its MCM entry have been removed **in the development build**. The separate hub handles settings; it does not take ownership of the Journal's stored preferences.

## Shared settings: technical details

The `K2040_GetSettingsAdapterV2` export connects the Journal's native settings to the common editor. Its controls come from the validated V1 JSON manifest. The hub asks the adapter to read or change values; it does not write the Journal's INI directly.

**Why:** Each mod keeps control of its own settings and can reject an invalid change. The adapter and query ABI headers must match the versions in the hub exactly.

The development QA package has **no Journal MCM entries** and no Journal-only settings shortcut. The separately installed hub owns the settings shortcut and its MCM entry. The Journal's gameplay keys remain editable under that hub's Controls page.

## Moving the mini tracker

The optional `K2040LayoutAdapterV2` export exposes only the compact tracker to the shared layout editor. The tracker view reports its current position and size relative to the viewport. The Journal plugin checks each proposed rectangle and previews valid changes. **Save** writes four integer position/size values to the existing per-user INI together; **Abort** restores the saved placement; **Retry** returns to the position from the start of editing so the player can try again.

These values use *basis points*: 10,000 units represent the full viewport. This keeps placement proportional across screen resolutions.

Changing the older anchor or width settings switches back to the old placement method. Layout editing does not change Pip-Boy synchronization, quest saves, or gameplay shortcuts. The hub handles input while the layout overlay is open.

## Testing limits

Source checks verify parsing, settings, ABI use, and synthetic tracking cases. A Windows DLL build checks compatibility at build time. **Neither proves that controller focus, positioning, or quest tracking works in-game.** Those features still need Fallout 4 1.10.163 testing before release.
