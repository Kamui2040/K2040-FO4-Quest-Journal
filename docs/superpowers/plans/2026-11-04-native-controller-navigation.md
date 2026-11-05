# Native Controller Navigation Implementation Plan

**Status: Complete.** The changes are on public `main`, the source checks passed, and the user confirmed the release candidate in-game on 5 November 2026.

**Goal:** Add predictable D-pad and left-stick focus navigation to the focused Quest Journal, with A to activate and B/Escape to back out, while retaining pointer input.

**Architecture:** A small engine-independent native sampler converts XInput state into debounced navigation actions. The existing native task queue forwards those actions only to the focused Journal view. `view/app.js` moves visible focus, activates controls, and routes back through the existing Escape path.

**Tech Stack:** C++23, XInput, existing F4SE task queue and Prisma interop, JavaScript, Node built-in test runner. No new dependencies.

**Spec:** `docs/superpowers/specs/2026-11-04-native-controller-navigation-design.md`

## Global Constraints
- D-pad and left stick move focus; A activates; B follows Escape behavior.
- Preserve right-stick cursor control and mouse use.
- Send navigation only while the Journal view is focused; never consume input for a vanilla menu or another Prisma view.
- Preserve Journal/Tracker shortcut assignments; while the Journal is focused, D-pad/A/B navigation takes priority over a colliding toggle binding.
- Do not add configurable mappings or dependencies.

## Review Focus
- No valid active element: begin at the first visible, enabled control in DOM order. Test initial focus selection.
- Hidden or disabled controls: skip them; scroll a newly focused offscreen control into view. Test both cases.
- Opposing D-pad/left-stick directions or diagonal stick input: D-pad wins; otherwise choose the dominant axis, with vertical winning an exact tie. Test deterministic selection.
- Held directions versus A/B: repeat directions after an initial delay; activate/back only once per press. Test hold timing and button edges.
- Journal loses focus while a control is held or an action is queued: reset the sampler and drop the queued action. Test loss-of-focus gating.

---

### Task 1: Add directional focus and activation to the Journal UI

**Files:**
- Modify: `view/app.js`
- Modify: `view/styles.css`
- Modify: `view/journal.html`
- Modify: `tests/journal_escape_hotkeys.test.cjs`

**Interface:** `window.receiveJournalControllerAction(action)` accepts `up`, `down`, `left`, `right`, `activate`, and `back`. It is a no-op when Settings/Journal is not the active view.

- [x] Add tests to `tests/journal_escape_hotkeys.test.cjs` for initial focus, nearest directional focus, hidden/disabled skipping, scroll-into-view, activation, Escape/back dispatch, left/right range adjustment, and the Controls-page navigation guidance.
- [x] Run `node --test tests/journal_escape_hotkeys.test.cjs`; confirm the new behavior assertions fail for the missing receiver.
- [x] Implement the receiver in `view/app.js`. Move focus among visible enabled controls in the active panel; when a custom select popup is open, pass directions through its existing arrow-key handler. Activate with the focused control's normal click behavior; route back through the existing Escape handler.
- [x] Add a visible controller-focus outline in `view/styles.css`; clear controller focus styling when pointer or keyboard interaction resumes. Explain the D-pad/left-stick, A, B, and right-stick controls in the Settings Controls page.
- [x] Run the focused Node test and `node --check view/app.js`.

### Task 2: Sample and forward controller actions safely

**Files:**
- Create: `src/ControllerNavigation.h`
- Modify: `src/main.cpp`
- Modify: `scripts/verify-source.sh`
- Test: `tests/native_controller_navigation.cpp`
- Modify: `docs/CHANGELOG.md`

**Interface:** `K2040::ControllerNavigation::Sampler::Update(Input, focused, now)` returns one action (`None`, direction, `Activate`, or `Back`). `Input` carries XInput button bits and left-stick axes. Direction repeats after 350 ms and then every 120 ms; use the XInput left-stick dead zone.

- [x] Write synthetic tests for D-pad priority, dominant-axis handling, dead zone, repeat cadence, A/B rising edges, focus loss, and menu-button hotkey collisions (navigation takes priority only while the Journal is focused).
- [x] Compile and run the new native test directly; confirm it fails on the unimplemented sampler behavior.
- [x] Implement the sampler in the engine-independent header and add its compile/run command to `scripts/verify-source.sh`.
- [x] Extend the existing XInput sample in `src/main.cpp` to include left-stick axes and D-pad/A/B bits.
- [x] Feed sampler actions from `PollHubPads`; marshal each action through the existing game-thread task queue, recheck focused Journal ownership in the task, and call `receiveJournalControllerAction` through Prisma interop. Do not call Prisma interop from the polling worker.
- [x] Update the Unreleased changelog entry.
- [x] Run `bash scripts/verify-source.sh`; build the Windows x64 DLL if the pinned toolchain is available.

### Task 3: Prepare and manually accept the controller candidate

**Files:** existing test package output and release notes only.

- [x] Stage a private RC ZIP from the fresh DLL and current UI assets; keep prior accepted ZIPs unchanged.
- [x] Verify ZIP contents, DLL x64/F4SE exports, and SHA-256.
- [x] Ask the user to check D-pad and left-stick focus movement, A activation, B dismissing a popup then leaving Settings, and continued right-stick/mouse operation in Fallout 4. Do not launch the game in this environment.

**Completed:** The implementation was committed after its checks passed and is now on public `main`.
