# Mission skeleton (GDC Mission Maker Tools)

English | [Français](GDC_MissionSkeleton.fr.md)

The **Create Mission Skeleton** plugin adds to your mission world everything a GDC mission starts with: layers, managers, game mode, mission framework, briefing sections and radio frequencies. It only adds what is missing: you can run it again at any time, it never changes what already exists.

## Use

1. Create your mission world as a sub-scene of the terrain and **save it** (the plugin refuses a world that was never saved).
2. Run **Plugins > GDC > Create Mission Skeleton**.
3. Read the summary: what was added, what was already there, what to check.
4. Save the world (Ctrl+S).

Changed your mind? Ctrl+Z undoes the whole run in one go (the frequency config written next to the world stays).

## What it adds

| Item | Details |
|---|---|
| Layers | `default`, `briefing`, `players`, `systems`, and the folders `ia` and `build` with a first layer each (`ia/opfor`, `build/decor`): git does not keep empty folders. An existing layer is reused whatever its case (`Players`). |
| AI world | The terrain's own when it ships one (it holds the terrain's navmeshes), else the generic one. Active AI limit 128. For the base game terrains (Arland, Eden, Cain), the missing navmeshes are filled with the Conflict ones. |
| Perception manager | |
| Game mode | Reforger Lobby (`PS_GameMode_Lobby`), with the usual GDC settings: no squad leader markers, garbage system off, lobby available during the game, common briefing for everyone, Arma Vision allowed, no freeze time. |
| Mission framework | TILW (`TILW_MF`), in `systems`. |
| Briefing | Sections Contexte, Situation, Mission, Renseignements, Insertion et extraction, in this order, visible to everyone, each with a short starting text to replace. |
| Frequencies | An empty Coalition VON frequency config next to the world (`Configs/<world>_Frequencies.conf`), linked on the game mode. |

An AI world, perception manager or framework already provided by the terrain is not duplicated. The game mode and the briefing are only looked for in your mission.

## To check after a run

- **"AI world navmeshes without a file"**: AI cannot move without them. Set the files in the AI world (NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig). This happens with a terrain whose addon ships no AI world prefab.
- **"prefabs of the following addons cannot be loaded"**: nothing was changed. Load the addons named (GDC AR Lib brings Reforger Lobby and TILW), then run the plugin again.

## Your own skeleton

The skeleton is described by a config: GDC's is `Configs/GDC MM/MissionSkeleton.conf`, read-only like the whole addon. To change it:

1. In the Resource Browser, right-click on `MissionSkeleton.conf` > **Create Inherited File**, in your **GDC Mission Maker Local** addon (the local addon created by [Update MM Libraries](GDC_MMLibraries.md): it is never published).
2. In the new config, change only what you need: layers, entities and their values, briefing sections, navmeshes per terrain. Each field is described in the editor. Everything you leave untouched follows GDC's config, updates included.
3. Select it in **Plugins > Settings > Create Mission Skeleton** (Skeleton Config). The Workbench keeps this setting between sessions.

Never put this config in your mission addon: it would be published with the mission, and it refers to GDC Mission Maker Tools, which must never be a mission dependency.
