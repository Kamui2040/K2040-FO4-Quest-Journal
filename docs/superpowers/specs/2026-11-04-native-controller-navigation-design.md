# Native Controller Navigation Design

## Intent
Make the focused Quest Journal usable through normal controller menu navigation, while retaining its existing cursor and mouse controls.

## Goals
- D-pad and left stick move visible focus through enabled, interactive journal controls.
- A activates the focused control.
- B follows the existing Escape/back behavior: dismiss an open control popup first, then leave Settings while keeping the journal open.
- Keep the right-stick cursor and mouse interaction available.
- Handle navigation only while this journal owns focus, without sending gameplay inputs through.

## Non-goals and anti-goals
- Do not change journal/tracker hotkey bindings or game state.
- Do not replace the existing mouse/right-stick cursor model.
- Do not consume controller input while a vanilla menu or another Prisma view is focused.
- Do not add configurable controller mappings in this change.

## Constraints
The UI is a Prisma web view. Existing page code handles keyboard arrows in custom select menus, and native code already polls XInput for controller shortcuts. Controller input must cross the existing native-to-view boundary safely and be acted on by the focused view. Directional input needs a dead zone and repeat behavior so a held stick or D-pad does not skip unpredictably.

## Approaches considered
1. **D-pad focus navigation only:** familiar and simple, but leaves the left stick less useful for menu navigation.
2. **D-pad and left stick focus navigation (chosen):** both move the same visible focus; A activates and B uses Escape/back. This matches common controller menus and coexists with the current right-stick pointer.
3. **Move the cursor with the left stick:** minimal UI focus work, but remains pointer navigation and does not provide a directional focus path.

## Behavior
Directional movement selects the nearest eligible control in that direction, skipping hidden and disabled controls and bringing the result into view. A uses the control's normal activation behavior. B follows the same layered Escape behavior as keyboard input. Navigation remains within the currently visible Journal or Settings panel; custom select popups keep their existing arrow-key behavior. Pointer input remains usable, and mouse interaction is unchanged.

## Validation boundary
Implementation checks cover directional focus, selection, back behavior, and input gating. The user confirmed the controller-navigation build in Fallout 4 on 5 November 2026. Local development does not launch the game.
