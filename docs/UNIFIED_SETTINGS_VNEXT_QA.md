# Quest Journal + K2040 Unified Settings VNext — QA adoption

**Target:** Fallout 4 OG 1.10.163 / matching F4SE, PrismaUI_F4 and MCM.

**Source:** Quest Journal development branch `fix/unified-visual-style-mcm-mouse`
(descended from `feature/unified-editor-v2`), used with a separately installed
compatible Unified Settings hub. Read `docs/VNEXT_MOD_ADOPTION.md` and
`docs/EDITOR_V2_INTEGRATION.md` in the hub checkout first.
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
- The generic shared-hub shortcut recorder supports Mouse 3-5, including
  Mouse 4 and Mouse 5 through a native fallback when Proton/CEF does not
  forward the browser event. This is hub-owned, not a Journal key recorder.
- Old per-mod MCM registries and existing user shortcuts are not deleted
  or rewritten during this migration.
- The only settings-editor MCM keybind is configured by the shared hub, not by Quest Journal.
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

Run the regular source checks and `scripts/build-normal-tracking.sh`
with pinned Windows x64 dependencies, then package the Quest Journal alone:

    python3 scripts/stage-test-package.py \
      --mcm-mode none \
      --output "$HOME/Downloads/K2040-Quest-Journal-OG163-ModOnly-QA.zip"

The resulting ZIP includes only the Journal DLL, provider, PrismaUI view assets
and license/source notices. **It never includes `K2040_Unified_Settings.dll`,
shared hub UI or MCM.** Install/update the compatible hub separately.
The script rejects invalid Windows x64 binaries, incomplete owner assets
and existing output paths. No game deployment or installation takes place.

The historical `scripts/package-unified-vnext-qa.py` helper can reproduce a
combined QA archive when explicitly requested; do not use it for the current
mod-only installation workflow.

## In-game acceptance — pending

Verify the hub-configured shortcut and any remapping; no Journal MCM child;
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
