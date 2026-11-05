# Changelog

Versions publiées sur le Workshop Bohemia, par mod, de la plus récente à la plus ancienne. Le texte de chaque version reprend celui publié sur le Workshop, en anglais : `ADDED`, `FIXED`, `CHANGED`. Les dates sont en UTC.

À compléter à chaque publication. Sont suivis GDC Core, GDC Equipment, GDC AR Lib et GDC Mission Maker Tools ; un autre mod s'ajoute ici à sa prochaine publication.

## GDC Core

[Historique sur le Workshop](https://reforger.armaplatform.com/workshop/663710CD8BEB01D4/changelog)

### 2.3.1 - 2026-11-05

- REMOVED : GameMasterFX (5994AD5A9F33BE57), until fix for 1.9 version

## GDC AR Lib

[Historique sur le Workshop](https://reforger.armaplatform.com/workshop/690756F509F253EB-GDCARLib/changelog)

### 1.3.0 - 2026-11-04

- REMOVED: GDC Loadout Editor. It moves to the new GDC Mission Maker Tools addon, a Workbench-only addon that is never a mission dependency. Missions and prefabs already edited with it are not affected.

### 1.2.1 - 2026-11-01

- ADDED: Teleport module, ALL_PLAYERS mode: a player of the minimum rank set on the module takes along all the players within the radius, whatever their group.
- ADDED: Teleport module, optional change of the game clock during the time skip. The set hour and minutes apply to all players in the world, at the first use of the module only. Players who stay behind don't get the black screen.
- CHANGED: Teleport module attribute m_fGroupRadius renamed m_fRadius. A value customised on an existing module returns to the default 30 m.

### 1.2.0 - 2026-11-01

- ADDED: Teleport module with time skip (Prefabs/Teleport/GDC_Teleport.et). Individual use, or group use where only the group leader triggers it and takes along the players gathered nearby. Options: fade and black screen durations, text, background image, game sound fade.
- ADDED: Map areas (Prefabs/Markers/GDC_Marker_Area.et), a polyline shown on the in-game map with outline and fill. Concave shapes supported. Also used to define fire support target areas.
- CHANGED: Virtual artillery module is now only available during the game phase: hidden from players in preview, briefing and debriefing, and requests are refused by the server.
- CHANGED: Prefab GDC_SupportZone renamed and moved to Prefabs/Markers/GDC_Marker_Area.et (same GUID).

### 1.1.0 - 2026-10-30

- ADDED: virtual artillery module

### 1.0.3 - 2026-10-14

- ADDED: per-slot radio frequencies (CVON) now accept a comma-separated list, one frequency per radio of the same type (e.g. "32000, 31000": 1st radio on the team net, 2nd on the squad net). A single value works as before.
- ADDED: Workbench plugin "GDC Import Loading Screens" (Resource Manager > Plugins), for mission makers.

### 1.0.2 - 2026-10-08

- ADDED: possibility to set radio frequences per slot (CVON)

### 1.0.1 - 2026-09-27

- Fixed another bug

### 1.0.0 - 2026-09-26

- ADDED: GDC Loadout Editor, first version

### 0.0.8 - 2026-09-19

- Added a scripted entity that follows the player closest to a reference.

### 0.0.7 - 2026-09-19

- Artillery fire support component: added a retry attempt if the crewman is not yet in the mortar.

### 0.0.6 - 2026-09-15

- ADDED: Improvements to GDC_ArtilleryFireSupport. Randomized firing order (each shot lands at a random position on the ring, with an increased delay between shots) and the ability to loop fire missions.

### 0.0.4 - 2026-07-15

- Added dependencies to TILW Mission Framework, PS Core and Reforger Lobby CE

### 0.0.3 - 2026-04-06

- Fixes GDC_ArtilleryFireSupport

### 0.0.2 - 2026-04-03

- Small fixes on default values for GDC_ArtilleryFireSupport prefab

### 0.0.1 - 2026-04-03

- Première publication, sans texte de version.

## GDC Mission Maker Tools

[Page sur le Workshop](https://reforger.armaplatform.com/workshop/6AC540B09694CF5F-GDCMissionMakerTools)

### 0.0.1 - 2026-11-04

- Première publication, sans texte de version : bibliothèques MM (« Update MM Libraries ») et GDC Loadout Editor, venu de GDC AR Lib.
