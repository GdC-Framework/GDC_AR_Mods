# Vehicle crew that never spawns (TILW)

English | [Français](tilw-vehicle-crew.fr.md)

Rule 5.2 · error · addon behaviour

## What happens
A vehicle with a `TILW_VehicleCrewComponent` stays empty in a Reforger Lobby mission: no crew spawns, and nothing in the log says so.

## Cause
Since commit 0f2e423 (2026-10-20, "remove RL dependency"), TILW loads before PSCore and Reforger Lobby. Its lobby code sits in `#ifdef PS_LOBBY` blocks, and `PS_LOBBY` is only defined by Reforger Lobby, compiled after TILW: these blocks are compiled out. The "defines:" line of the log lists `PS_LOBBY` anyway, which is misleading.

## Evidence
Diagnostic of 2026-11-10: `TILW_Lobby.GetGameMode()` returns null under `PS_GameModeCoop`. Versions: TILW 1.0.128, Reforger Lobby 1.8.23, PSCore 1.6.10.

## Also affected
Same cause, just as silent: faction tickets, respawn of dead players, RemoveOpenSlots, per-faction marker visibility, AO limit faction listener, spawner state listener.

## Workaround
A `TILW_PrefabSpawner` (Prefabs/Utils/TILW_PrefabSpawner.et):
- m_prefab: the vehicle;
- m_crewConfig: the crew (waypoints allowed);
- m_pregameSpawn ticked;
- m_setEntityNames if the vehicle must keep its name.

Tested on Davay in the Workbench. Limits: the crew is active during the lobby; per-instance overrides need a derived prefab.

## When to remove the rule
When TILW compiles its lobby code again with Reforger Lobby (bridge addon or dependency restored): check TILW's repository at each update.

## History
- 2026-11-10: cause confirmed, workaround tested (TILW 1.0.128).
