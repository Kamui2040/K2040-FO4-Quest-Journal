# Quest Journal + Unified Settings: test checklist

**Status:** Development QA, not released.

**Game:** Original Fallout 4 **1.10.163** with matching F4SE, PrismaUI, and the separately installed K2040 Unified Settings hub.

The current Journal candidate is **R11**, on `fix/framework-structural-journal-tracker`. It includes the earlier `fix/unified-visual-style-mcm-mouse` and Settings V2 work. Read `docs/VNEXT_MOD_ADOPTION.md` and `docs/EDITOR_V2_INTEGRATION.md` in the **hub** repository before combining versions.

## What is implemented

- **One shared settings screen:** The hub shows Quest Journal's 31 settings and reset action. Quest Journal has no separate settings MCM child or settings-only keybind. Gameplay shortcuts for the Journal and tracker remain independent and are saved by Quest Journal.
- **Direct Settings button:** The button inside the Journal opens its own page inside the hub.
- **Pip-Boy sync remains in the Journal:** The checkbox is not a hub setting. Changing it refreshes the tracker but must not alter Fallout 4's quest-tracking flags. Hub Reset leaves it unchanged.
- **Controller navigation:** D-pad, Confirm, and Back follow the game's remapped menu controls. Native shortcut chords are order-independent; optional pin/favorite shortcuts yield to normal menu actions. The latest Switch Pro and Steam Input behavior has not passed in-game testing.
- **Shared styles and colors:** The Journal reads the hub's appearance snapshot from an already loaded DLL. Native code validates the schema, theme and palette IDs, and seven `#RRGGBB` color values; JavaScript checks the result again. Invalid or missing hub data should restore the Journal's own appearance without overwriting saved preferences. The old theme callback remains for compatibility.
- **Mouse 3–5:** The hub owns recording mouse shortcuts. A native fallback is used for Mouse 4/5 where Proton/CEF does not send the browser event.
- **Legacy preferences:** Old MCM registries and user shortcuts remain on disk and may be imported read-only. No Journal MCM page is bundled with this test build.
- **Mini-tracker layout:** The Journal exports `K2040_GetLayoutAdapterV2` for one widget, `quest-tracker`. The hub draws the editing overlay. The Journal validates a proposed position/size against its PrismaUI viewport, including ultrawide displays.

The hub supports dragging, keyboard/controller nudging, wheel changes of 1%, and +/- resizing steps of 10%. **Save** writes valid coordinates together to the Journal's INI; **Abort** restores the saved layout without writing; **Retry** returns to the position from the start of editing and keeps the editor open. Save changes invalidate the preview. Changing an older anchor or width setting returns to the legacy layout.

**Why this separation matters:** The hub presents controls, but only Quest Journal can edit its preferences. Layout previews must never change quest saves or tracking state.

## Packaging

Build with `scripts/build-normal-tracking.sh` after passing `scripts/verify-source.sh`, then stage Quest Journal **without** the hub:

```bash
python3 scripts/stage-test-package.py   --mcm-mode none   --output "$HOME/Downloads/testing/K2040-Quest-Journal-v0.0.4.zip"
```

Keep **only the latest validated Quest Journal QA package** directly in `~/Downloads/testing/`. No nested directories. Name each package `K2040-Quest-Journal-vX.Y.Z.zip` and increment its version whenever the contents change. Record the development revision and test status in private QA notes, not the filename. The current `v0.0.4` is not a public release. The packaging script refuses an existing output path; remove or relocate a superseded package only after checking the replacement.

The ZIP contains the Journal DLL, its provider at `Data/K2040/Settings/Providers/quest-journal.json`, PrismaUI views, and licensing files. It does **not** include `K2040_Unified_Settings.dll`, hub UI, hub MCM, or user settings. Install the compatible hub separately. The script does not deploy into the game.

Technical note: The optional appearance-client header has a bounded `std::string` construction fix for MSVC/clang-cl. This does not change the ABI or hub source. The older `scripts/package-unified-vnext-qa.py` helper can reproduce previous combined packages when specifically needed, but is not the normal test workflow.

## In-game checks — not yet complete

1. Open the hub with its configurable shortcut. Confirm that Journal categories and settings work, and there is only **one hub MCM entry**.
2. Open the Journal's **Settings** button. Check that it lands on the Quest Journal page, not a separate renderer.
3. Toggle the Pip-Boy sync checkbox. Reopen the Journal and confirm the tracker changes without changing tracked quests.
4. Try all four frame styles, built-in/custom palettes including Hot Omni, and the Journal/tracker's saved colors.
5. With the **Switch Pro controller**, check the game's actual Confirm and Back buttons, D-pad, both sticks, focus, Escape, and return to normal gameplay.
6. Try Journal/tracker gameplay shortcuts and Mouse 4/5 settings through the hub.
7. With a visible tracker, edit its layout: drag, nudge, 1% wheel and 10% resize, Save, Abort, Retry, reopen, and reload. Try the old anchor/width controls too.
8. Check Phosphor HUD coexistence, missing or outdated hub behavior, invalid settings, save changes, and startup without the hub.
9. Recheck ordinary quest tracking and tracker updates against the Pip-Boy.

**Passing source tests and building the Windows DLL is not the same as passing these in-game checks.** Do not merge or release until the required runtime tests are accepted.
