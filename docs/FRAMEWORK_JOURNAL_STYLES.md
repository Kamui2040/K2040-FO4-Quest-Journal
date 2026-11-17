# Framework styles for Quest Journal

**Status:** Source-reviewed development work. The four styles are **not yet accepted in-game** on Fallout 4 1.10.163.

Quest Journal and its small HUD tracker can follow the appearance style selected in the separate K2040 Framework/Unified Settings hub. The hub supplies a style and palette; Quest Journal still owns its quests, colors, positions, and saved preferences.

## The four styles

| Style ID | What changes |
| --- | --- |
| `fallout4` | Squared industrial frames, engraved headings, and rails |
| `fallout76` | Flat rectangular panels, angled selection strips, and tighter uppercase headings |
| `doom-eternal` | Cut-corner selections, heavier dividers, and stronger text emphasis |
| `fallout3-newvegas` | Terminal-like grids, a chevron selection marker, bracketed tracker headings, and dashed lines |

These affect **frame shapes, separators, headings, and selections**, not just colors. Both the full Journal and the mini tracker get the style. Colors still come from the active palette or the user's saved settings; switching frames should not erase them.

The source was checked against the Framework design references, including `design/framework-visual-language.v1.json`, `docs/VISUAL_THEME_STYLE_GUIDE.md`, and `docs/FRAMEWORK_VISUAL_AUTHORITY.md` in the Framework project.

## Implementation notes

The Journal applies one CSS theme class at a time. Its JavaScript explicitly adds the new class and removes the others, with a compatible fallback for browsers that cannot reliably handle `DOMTokenList.toggle(name, force)`.

**Why:** On some browser builds, colors could change while the actual frames stayed stuck on the previous style. Explicit class updates avoid that mismatch.

Legacy and R6 CSS frame rules were consolidated so they do not fight the newer styles. The appearance input also checks for missing, invalid, circular, or non-serializable values. Invalid input should leave the last usable style intact rather than crashing the view.

No new user INI, graphics archive, Quest Journal MCM entry, or Framework DLL is bundled here. The separately installed hub supplies appearance settings. A related controller fix corrects left-stick vertical direction and signed magnitude overflow.

## What still needs to work in-game

- View each style in **both** Journal and mini tracker.
- Try at least Default, Radioactive Green, and Hot Omni palettes.
- Check saved custom colors, size, placement, and quest visibility after changing styles.
- Retest Settings navigation, Pip-Boy synchronization, controller focus, and closing/reopening the Journal.
- Change saves and reload to make sure the tracker and selected appearance recover correctly.

JavaScript/Python tests and source review alone do **not** complete this checklist.
