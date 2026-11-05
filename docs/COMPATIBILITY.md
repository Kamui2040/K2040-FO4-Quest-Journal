# Compatibility

## Game version

Quest Journal supports **Fallout 4 (1.10.163)**, the original PC version. It requires the matching F4SE and Address Library, along with PrismaUI for Fallout 4.

The next-gen/Anniversary Edition versions and Fallout 4 VR are not supported.

## Controller menus

The Journal menus support D-pad and left-stick focus navigation, **A** to activate, and **B** to go back. Right-stick cursor navigation remains available. The user confirmed this behavior in-game with the native controller-navigation release candidate on 5 November 2026. Mouse and keyboard input remains supported.

## Quest tracking

You can track supported quests in the Journal, just as you would in the Pip-Boy. The mini tracker can also show quests tracked through the Pip-Boy or quests you've pinned yourself.

Some repeatable quests and quests from other mods handle objectives differently. In those cases, not every tracking option may be available. Quest Journal does not change quest progress or completion.

## Other mods

In the unreleased development source after v0.0.2, Quest Journal detects a loaded [Phosphor HUD](https://www.nexusmods.com/fallout4/mods/109983) with its live HUD enabled and suppresses only its own mini tracker by default. The **Compact tracker** settings include **Show Quest Journal tracker with Phosphor HUD** (off by default), which lets users opt into Quest Journal's tracker even if Phosphor is active. If Phosphor also displays its tracker, both may appear until Phosphor provides and the user enables a separate disable option. The override persists and can be changed immediately without restarting the game. The full-screen Journal is not affected, and neither mod writes to the other mod's settings. This is not part of the published v0.0.2 package.

MCM is optional, though useful for changing your keyboard shortcuts.

If you run into a problem with another mod, [report it on GitHub](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues) and include your game and mod versions and a short description of what happened.
