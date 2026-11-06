# Quick Data Action Redirect Design

## Intent

Let players optionally open Quest Journal with Fallout 4's Quick Data action, which is usually bound to J. When enabled, that action opens Quest Journal instead of the vanilla quest/data screen.

## Goals

- Add an off-by-default setting to redirect Quick Data to Quest Journal.
- Match the game's Quick Data action, not the physical J key, so the feature follows changes to the player's key binding.
- When enabled in gameplay, open Quest Journal and prevent the same action from also opening the vanilla screen.
- Preserve normal Quick Data behavior while the option is off or when a game menu or another Prisma view owns input.
- Keep current Journal and tracker shortcuts unchanged.

## Non-goals

- Change Fallout 4's saved control map or any other mod's bindings.
- Install a Windows keyboard hook or intercept input outside the game.
- Change behavior for Quick Data while another menu is already open.

## Approaches

1. **Intercept the mapped Quick Data action (chosen).** Use the game's action identity from its input event and stop that event before the vanilla Quick Data handler receives it. This follows the player's current binding. The implementation must verify that this event path can reliably stop the vanilla action before enabling the setting.
2. **Intercept the physical J key.** This is simpler but stops working if the player rebinds Quick Data, so it does not meet the requirement.
3. **Rewrite the game's control map or use a global keyboard hook.** These methods affect other input behavior or operate outside the game, so they are out of scope.

## Behavior and constraints

The setting is off for new and existing users. When enabled, the mapped Quick Data action opens Quest Journal only during gameplay and only when no competing menu or Prisma view owns input. The event must be consumed so Fallout does not also open the vanilla screen. When disabled, the event is left untouched.

The implementation matches `InputEvent::QUserEvent()` to `QuickData` and sets `handled` to `kStop` before dispatching the Journal action. The pinned CommonLibF4 headers expose this handled state, and the local OG 1.10.163 control map names the action `QuickData`. The user confirmed both in-game checks passed: the usual binding opens the Journal without the vanilla Data screen, and the redirect follows a changed Quick Data binding. If a future game/runtime update breaks this behavior, investigate a supported F4SE input route before public release.

## Validation

- Source checks cover the off-by-default setting, persistence, mapped-action matching, input ownership gates, and event consumption.
- In Fallout 4, verify that disabled Quick Data behaves normally; enabled Quick Data opens Quest Journal without opening the vanilla screen; changing Quick Data from J to another key still redirects; and pressing it while another menu owns input retains vanilla behavior.
- Keep the feature in unreleased v0.0.3 until the in-game checks pass.
