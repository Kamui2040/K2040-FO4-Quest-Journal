# Changelog

## Unreleased — v0.0.3

- Preserve the last valid quest list and tracker preferences when a native JSON payload has an invalid structure; save transitions still clear the list normally.
- Reuse the same native quest-instance assessment for display status and instance identity instead of resolving it twice.
- Improve responsiveness on large quest lists: search without temporary objective arrays, cache category ranks during sorting and index duplicate Misc rows instead of scanning all quests.

- Optionally open Quest Journal with Fallout 4's Quick Data action (usually J). The redirect follows your current key binding and is off by default.
- When Phosphor HUD is active, hide the Quest Journal mini tracker by default. Turn on **Show Quest Journal tracker with Phosphor HUD** in Compact tracker settings to show it anyway. This setting is off by default and saved between sessions. The full Journal stays available, and Quest Journal does not change Phosphor's settings.
- Retire Quest Journal's separate settings editor and MCM entry. The Journal Settings button opens its registered page directly in the separately installed K2040 Framework / Unified Settings Hub.
- Add four Framework structural themes for both the full Journal and mini tracker, independently of saved colors. The live tracker layout editor remains owned by the hub.
- Use the D-pad or left stick to navigate. Confirm/Back use Fallout 4's mapped menu assignments, including Switch Pro / Steam Input, rather than fixed Xbox labels.
- Prevent controller-synthesized arrows from interfering with an open category menu.

## 0.0.2 — First release

- Full-screen quest journal with search, categories, favorites and quest objectives.
- Compact HUD tracker with adjustable position, size and visible quest count.
- Quest tracking with Pip-Boy synchronization.
- Separate appearance settings for the Journal and mini tracker, including custom colors.
- Keyboard and controller support, with **Shift + G** and **G** as the default shortcuts.
- Journal settings and optional MCM support.
- Darker, more consistent dropdown menus and instant tracker updates when settings change.

Requires Fallout 4 (1.10.163), F4SE, Address Library and PrismaUI for Fallout 4.
