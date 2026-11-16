# Unified Settings visual styles, shortcuts and MCM (QA)

- Source branch is experimental, targeting original Fallout 4 1.10.163.
- Four independent shared visual themes now affect journal/tracker frames,
  corners and typography, without saving or overwriting local palettes or
  changing quest data, Pip-Boy mirror, or tracker placement.
- Journal and tracker gameplay shortcuts use native owner preferences;
  the shared hub's generic key recorder accepts mouse buttons 3–5, including
  Mouse 4 and Mouse 5 via a native fallback for Proton/CEF.
- Unmodified F3 is reserved as the Unified Settings provisional opener.
  Existing user-customized bindings remain intact; Ctrl+Shift+B is no
  longer reserved.
- This owner installs no MCM menu, keybinds, or shared parent. The hub
  separately installs the only top-level K2040 Settings MCM entry.
  Existing MCM user registries and old mod settings are neither deleted
  nor rewritten.
- Package Quest Journal with `scripts/stage-test-package.py --mcm-mode none`;
  install the independently supplied Unified Settings Hub separately. The
  Quest Journal ZIP must not contain `K2040_Unified_Settings.dll` or hub MCM.
- Source checks and native compilation are not in-game acceptance.
  Nothing has been deployed or published.

- The Journal Settings button routes to its own registered page in the updated hub through a validated optional native export. When the hub is absent or older, no substitute editor is launched; an in-Journal status message explains what is needed.
- No individual Journal MCM entry or hotkey is packaged; the shared hub remains a separate installation.
