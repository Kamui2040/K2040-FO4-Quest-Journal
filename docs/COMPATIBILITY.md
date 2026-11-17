# Compatibility

## Fallout 4 versions

Quest Journal supports the **original PC version 1.10.163**, with matching F4SE, Address Library, and PrismaUI for Fallout 4.

**Not supported:** next-gen Fallout 4, Anniversary Edition, and Fallout 4 VR.

## Tracking quests

The Journal can track supported quests and display quests you have tracked through the Pip-Boy. You can also pin quests to the mini tracker.

Some mod-added or repeatable quests handle objectives differently. Not every tracking action will be available for every quest. The mod does **not** change quest stages or completion. If a particular quest behaves unexpectedly, include the quest and mod name in a bug report.

Two separate mod-added quests have been confirmed in-game; that does not mean every mod-added or repeatable quest has been tested.

## Controllers

The unreleased v0.0.3 development build adds D-pad and left-stick menu navigation. Confirm and Back follow Fallout 4's current mappings, including Steam Input mappings where the game provides them, rather than assuming Xbox A and B. The right stick can still move the cursor.

An earlier controller flow worked in-game on **5 November 2026**. The newest mapping, Switch Pro behavior, and live layout editor **still need game testing**. They are not in public v0.0.2.

## Phosphor HUD

When [Phosphor HUD](https://www.nexusmods.com/fallout4/mods/109983) has its live HUD enabled, Quest Journal can hide its **mini tracker** to avoid showing two quest trackers. The full Journal stays available.

In the development build, enable **Show Quest Journal tracker with Phosphor HUD** under Quest Journal's tracker settings in the shared hub if you want both. This choice is saved and takes effect right away. It does not change Phosphor's own settings.

**Release note:** This compatibility behavior is in unreleased v0.0.3 work, **not** the published v0.0.2 package.

## Unified Settings (development only)

The separate hub provides the one settings shortcut and MCM entry. Quest Journal's gameplay shortcuts are changed on the hub's Controls page but are saved by Quest Journal itself.

If the tracker is visible and initialized, the hub can open a layout editor to move or resize it. Save keeps the new position, Abort returns to the saved one, and Retry returns to the layout from the start of editing. These actions should not affect quests.

The latest combined setup still needs testing with a real controller, save/reload, and missing-hub scenarios. The Journal should not silently replace your settings if the hub cannot be found.

## Report a problem

[Open a GitHub issue](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues) with your Fallout 4 version, mod versions, and a short description of what you tried and what happened.
