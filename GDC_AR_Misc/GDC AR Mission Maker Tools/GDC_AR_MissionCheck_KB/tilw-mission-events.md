# TILW mission events badly set

English | [Français](tilw-mission-events.fr.md)

Rules 5.6 to 5.14 · warning or error · addon behaviour

## What happens
The mission events of the TILW Mission Framework entity run their instructions when their condition becomes true. TILW evaluates every event again each time a flag changes. Some settings make an event silent, early, or inoperative, with no message to the mission maker:

| Rule | Setting | Effect |
|---|---|---|
| 5.6 | event without condition, meta flag without definition | never true; a script error ("NULL pointer to instance") at each evaluation. A Run Mission Events instruction still runs the event |
| 5.7 | operator (Conjunction, Disjunction...) without operand | its result never changes: an empty Conjunction is true, so the event happens as soon as the game starts; an empty Disjunction is never true |
| 5.8 | condition waiting for a flag that nothing sets | the event (or the interaction, spawner, GDC AI artillery) never happens. Not reported for an event run by name |
| 5.9 | Run, Reactivate or Deactivate Events naming no event | the instruction does nothing |
| 5.10 | name aimed at, held by several events | the instruction acts on all of them |
| 5.11 | place, group, waypoint, entity, map item, vehicle that no entity holds | the instruction (or spawner, vehicle crew) does nothing for that name |
| 5.12 | several framework entities | TILW keeps one; flag changes no longer evaluate the events of the others |
| 5.13 | event allowed before the end of the briefing (`m_pregameEvent`) reading flags that are not random | these flags are mostly not set yet |
| 5.14 | `m_suppressGameEnd` ticked while an event ends the game | End Game shows a message instead of ending the game |

Flags count as set when a trigger, an interaction, a Set Flag or random flag instruction, a meta flag, a casualty or random flag, or a damage or control mode flag component sets them, in the mission or in a prefab it makes appear (spawner, instruction, group member, vehicle crew).

## Evidence
Read in the TILW scripts (`TILW_MissionFrameworkEntity`, `TILW_Instructions`, `TILW_Terms`), version 1.0.128. Engine behaviour measured by the automatic tests (`Tools/MissionCheckTests`, engine probe): a method called on null logs the error and the script goes on, the test then counts as false; an operator without operand gives the result above.

## What to do
Follow the fix given in the report. A flag set only by a mission script cannot be seen: ignore 5.8 in that case.

## When to remove the rules
If TILW changes its events, terms or instructions: check its scripts at each update. If the engine stops a script on a null call, 5.6 becomes worse (the following events are no longer evaluated): the engine probe test will tell.

## History
- 2026-11-21: rules written (TILW 1.0.128).
