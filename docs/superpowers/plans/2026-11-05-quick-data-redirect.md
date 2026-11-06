# Quick Data Redirect Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax.

**Goal:** Let players optionally open Quest Journal with Fallout 4's mapped Quick Data action.

**Architecture:** Carry an off-by-default boolean through existing native preferences and Prisma settings UI. Match `QuickData` on the native input event and set its handled state to stop vanilla processing only when gameplay owns input.

**Tech Stack:** C++ F4SE/CommonLibF4, existing JavaScript settings module and Node tests.

**Spec:** `docs/superpowers/specs/2026-11-05-quick-data-key-redirect-design.md`

## Global Constraints

- The option is off for new and existing users.
- Match the game action identity, not a physical key.
- Preserve normal Quick Data while another menu or Prisma view owns input.
- Do not alter the control map or use global keyboard hooks.

## Review Focus

- Rebound Quick Data follows the game's logical action name.
- Disabled or competing-menu input reaches vanilla unchanged.
- Settings survive save/load and settings reset returns the option to off.

### Task 1: Implement and package the Quick Data redirect

**Files:** `src/JournalSettings.{h,cpp}`, `src/main.cpp`, `view/{journal.html,journal-settings.js,app.js}`, existing relevant tests, `docs/CHANGELOG.md`, mirrored MCM config text if relevant.

**Interfaces:** Use the existing `updateJournalSetting` command bridge and `Preferences` persistence. The event action is `QuickData`; consume with `InputEvent::HANDLED_RESULT::kStop`.

- [x] Add failing tests for settings normalization/persistence and the mapped-event redirect; focused tests failed at the expected missing behavior.
- [x] Implement the native action interception, off-by-default saved setting, UI checkbox and help text.
- [x] Run source verification and build the OG 1.10.163 Windows x64 plugin; source and PE checks passed.
- [x] Stage and inspect a non-overwriting QA archive; the user confirmed the usual and rebound Quick Data in-game checks both passed.
- [x] Confirm Quick Data keeps normal behavior while another menu owns input; the user confirmed this in-game check passed.
