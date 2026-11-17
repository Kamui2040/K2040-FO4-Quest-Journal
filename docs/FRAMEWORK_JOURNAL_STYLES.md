# K2040 Framework — Quest Journal structural theme adoption

Source QA only; Fallout 4 OG 1.10.163 in-game acceptance pending.

The Journal and its passive mini tracker now use the same active structural
theme received through the optional Framework appearance snapshot.
Palette roles and owner-saved text, color, position, visibility and quest
tracking preferences remain independent and are never rewritten.

Both Journal and tracker use explicit DOMTokenList add/remove (with a
compatible toggle fallback) to apply exactly one structural theme.
This avoids a browser-specific dependency on DOMTokenList.toggle(force),
which can silently leave frame geometry unchanged while CSS variables
recolor the interface.

Four approved structural treatments:
- fallout4 — square industrial outlines, engraved headers and rails;
- fallout76 — flat rectangular panels, angled selected ribbons and compact
  uppercase headings (not the former rounded brochure styling);
- doom-eternal — clipped/faceted selections, heavy technical dividers,
  emphasized selection and tracker typography;
- fallout3-newvegas — thin terminal grids, chevron selected-row marker,
  bracketed tracker headings, monospaced labels and dashed separators.

Both the full-screen quest log and compact mini tracker receive
distinct frames, separators and selection treatments. No new graphics,
gameplay settings, hub DLLs, user INIs or MCM entries are bundled.

Regression checks: the new theme-state/geometry tests plus the existing
JavaScript and Python suites. In-game validation must verify each theme
with at least Default, Radioactive Green and Hot Omni palettes, compare
Journal and tracker simultaneously, and retest direct Settings navigation,
Pip-Boy synchronization, focus, controller and load/reload.

Safety QA: the optional shared appearance entry now rejects invalid,
missing, circular and non-serializable inputs without crashing the Journal
or replacing the last valid theme or palette. This change does not alter
saved preferences, tracker data, direct Hub navigation or the native DLL.

Owner review (2026-11-17): compared against the Framework-owned
design/framework-visual-language.v1.json,
docs/VISUAL_THEME_STYLE_GUIDE.md and
docs/FRAMEWORK_VISUAL_AUTHORITY.md. Consolidated the conflicting
legacy/R6 CSS declarations into one shared structural language with
four theme sections; all colors continue to come from semantic palette
roles, not from fixed signature colors. The full Journal and tracker
remain owned by Quest Journal; the Framework/UI hub is a dependency,
never bundled. Controller regression work also corrects XInput
thumbstick vertical direction and signed magnitude overflow.

This correction is a *source-reviewed proposed owner integration*;
do not call it owner-accepted or in-game tested before the corresponding
QA package passes Fallout 4 OG163 runtime checks.
