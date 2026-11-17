# Compatibility

## Game version

Quest Journal supports **Fallout 4 (1.10.163)**, the original PC version. It requires the matching F4SE and Address Library, along with PrismaUI for Fallout 4.

The next-gen/Anniversary Edition versions and Fallout 4 VR are not supported.

## Controller menus

The upcoming v0.0.3 development build adds D-pad and left-stick menu navigation. Confirm/Back follow Fallout 4's effective menu bindings rather than Xbox-specific letters, including Switch Pro/Steam Input mappings where provided by the game. The right stick can still move the cursor. An earlier controller flow was confirmed in-game on 5 November 2026; the newly mapped input and live HUD layout V2 integration still require independent game testing. The published v0.0.2 package does not include them.

## Quest tracking

You can track supported quests in the Journal, just as you would in the Pip-Boy. The mini tracker can also show quests tracked through the Pip-Boy or quests you've pinned yourself.

Some repeatable quests and quests from other mods handle objectives differently. In those cases, not every tracking option may be available. Quest Journal does not change quest progress or completion.

## Other mods

When [Phosphor HUD](https://www.nexusmods.com/fallout4/mods/109983) is running with its live HUD on, Quest Journal hides only its own mini tracker. The full Journal remains available. To show the mini tracker too, turn on **Show Quest Journal tracker with Phosphor HUD** in K2040 Unified Settings V2 under Quest Journal's tracker options. This option is off by default, saves between sessions, and takes effect right away. Quest Journal does not change Phosphor's settings. If both mods show a tracker, you may see two. These changes are in v0.0.3 development and are not part of the published v0.0.2 release.

In the VNext QA build, only the shared Unified Settings hub installs a top-level MCM entry, with the shortcut configured in the hub's MCM entry. Journal/tracker gameplay shortcuts are edited through the hub Controls page and saved by Quest Journal itself. The hub's live layout action is available for a visible mini tracker; it can be moved, resized, saved, aborted or reset to the start-of-session layout without modifying any quest state.

If you run into a problem with another mod, [report it on GitHub](https://github.com/Kamui2040/K2040-FO4-Quest-Journal/issues) and include your game and mod versions and a short description of what happened.
