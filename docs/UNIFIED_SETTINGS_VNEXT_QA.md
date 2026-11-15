# Quest Journal + K2040 Unified Settings VNext — QA adoption

**Target:** Fallout 4 OG 1.10.163 / matching F4SE, PrismaUI_F4 and MCM.

**Source:** this repository branch feature/unified-editor-v2, paired with
an authorized local checkout of the Unified Settings hub at
feature/unified-settings-editor-v2. Read docs/VNEXT_MOD_ADOPTION.md and
docs/EDITOR_V2_INTEGRATION.md in the hub checkout first.
No private source checkout or internal build artifacts are bundled.

## Implemented in Quest Journal

- The validated V2 adapter exposes 31 owner settings plus reset.
  Pip-Boy mirroring is not a hub field: an on-screen Journal checkbox
  saves via the native owner, updates both views and only changes what
  the HUD tracker displays. Hub reset preserves this Journal-only value.
  No in-game quest-tracking flags are changed.
- Ported the controller-fix branch's native menu mapping, order-independent
  controller shortcut chords and search reset. The game-thread snapshot
  follows Fallout 4's remapped Confirm/Back/D-pad; optional pin/favorite
  shortcuts yield to menu actions. Switch Pro/Steam Input runtime acceptance
  remains pending.
- providers/quest-journal.json is staged to
  Data/K2040/Settings/Providers/quest-journal.json, declaring the already
  loaded K2040_Quest_Journal.dll and V2 getter.
- The VNext K2040_CopyAppearanceV1 snapshot is read from an already-loaded
  hub DLL. C++ validates schema, theme IDs, palette IDs and seven #RRGGBB
  semantic colors; JavaScript validates again before applying temporary
  colors and frame classes to Journal/tracker. Without a hub, the native
  appearance is restored and the saved owner INI is never overwritten.
  The legacy theme callback remains for compatibility.
- The optional official appearance-client header has one necessary owner-side
  MSVC/clang-cl compile fix: construct std::string from a bounded length,
  rather than mismatched char* and const char* iterator types. No ABI,
  callback or hub-source code was changed.
- The only MCM keybind is the hub's configurable Ctrl+Shift+B.
  Quest Journal has no MCM child, registry actions, gamepad dropdowns or
  native MCM callback. Its independent gameplay keyboard and gamepad
  controls are native owner preferences shown on the hub Controls page.
  Legacy controller user files are read only, never overwritten.
- Live HUD layout adapter V2 exports K2040_GetLayoutAdapterV2 with the
  canonical byte-matched hub ABI. Its single widget ID is quest-tracker.
  The hub's Appearance category exposes Edit this mod's HUD layout only
  when the mini tracker is visible and initialized. The owner reports its
  current Prisma viewport rectangle (including ultrawide screens);
  the hub owns the transparent overlay, dragging, keyboard/controller
  nudges, wheel 1% and +/- 10% proportional resizing, and confirmation UI.
  The Journal accepts only bounded, proportional, in-viewport geometry.
  Save atomically persists normalized position and size in the existing
  owner INI; Abort restores the saved layout without writing; Retry
  restores the original in-memory snapshot and continues editing.
  Loading or changing saves invalidates the live session without writing.
  Older anchor/width settings work until Save, and a later change to either
  restores the legacy placement behavior. The old on-screen Pip-Boy sync
  toggle and other gameplay features are unaffected.
  **Runtime confirmation of overlay positioning and visual scaling is
  still pending**; source/build tests do not establish actual FO4 behavior.

## Packaging and QA

Run the regular source checks and scripts/build-normal-tracking.sh with
pinned Windows x64 dependencies, then package the built Quest Journal
together with the official, source-matched Unified Settings hub checkout:

    python3 scripts/package-unified-vnext-qa.py \
      --hub-root "/path/to/K2040-Unified-Settings-editor-v2" \
      --output "$HOME/Downloads/K2040-FO4-Quest-Journal/Quest-Journal-Unified-VNext-OG163-QA.zip"

The script checks clean correct source branches, identical settings AND
layout V2 ABI headers, provider, Windows x64 F4SE and adapter exports,
hub appearance
exports, source/staging asset consistency and identical shared MCM parent.
It rejects existing ZIP outputs, unexpected files and private MCM registry
or user settings. No game deployment or installation takes place.

## In-game acceptance — pending

Verify Ctrl+Shift+B and remapped hub binding; no Journal MCM child;
Journal-only Pip-Boy sync toggle, persistence and immediate tracker
refresh; module and category visibility; live tracker layout Edit button
for a visible tracker, drag/nudge, proportional 1% wheel and 10% key
resize, Esc Save/Abort/Retry, save/reload persistence, rollback and
legacy layout recovery;
typed edits with saved values after reload and owner reset; all themes,
built-in and named custom palettes including Hot Omni; per-mod appearance
override; controller native Confirm/Back; Journal/tracker gameplay shortcuts;
Phosphor HUD coexistence; corrupt/absent owner fail-closed behavior;
save transitions, focus and cursor ownership; startup without hub;
and tracking regression.

Build/static tests do not imply gameplay verification. No release promotion
is authorized until owner acceptance.
