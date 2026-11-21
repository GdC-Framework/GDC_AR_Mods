# AI or waypoint above the void

English | [Français](engine-ai-placement.fr.md)

Rule 6.5 · warning · engine behaviour

## What happens
An AI group, an AI character or a waypoint is placed with nothing (ground or object) just below it: one floor too high, beside a balcony, in the air, or under the terrain. The AI then falls, spawns in a wall, or never reaches the waypoint ("Failed move" in the log).

## Evidence
Check Mission looks for a surface from 0.5 m above the entity down to 1.5 m below it, against the terrain and the objects (buildings included). Measured by the engine probe of the automatic tests (`Tools/MissionCheckTests`) on Arland, Gulfcoast and Zimnitrita: the trace finds the ground and the roofs.

## What the check cannot see
Whether the AI can walk to a waypoint. In the World Editor, the navmesh answers on Arland but not on terrains whose navmesh is split in tiles: on Zimnitrita (Davay), where the AI move fine in game, no point of the soldiers navmesh is found. A waypoint on a raised platform with no way up, or a route blocked by a canal, is therefore found only in Play: the mission tests (`Tools/MissionTests`) follow the groups and report the ones that stop.

## What to do
Lower the entity onto the floor or the ground. For a marksman at a window, check the floor in the World Editor before saving.

## When to remove the rule
If the World Editor gives a reliable navmesh on every terrain: the rule could then check the navmesh itself (the engine probe test will tell).

## History
- 2026-11-21: rule written; navmesh checks dropped after the probes on Arland, Gulfcoast and Zimnitrita.
