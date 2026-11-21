# Entity deleter without a filter (TILW)

English | [Français](tilw-entity-deleter.fr.md)

Rule 4.1 · warning · addon behaviour

## What happens
A `PK_EntityDeleter` whose prefab filter list is empty deletes every root entity within its radius when it fires: props, but also managers, game mode entities and AI. Depending on what lies in the sphere, the mission breaks or the server crashes.

## Evidence
Crash on Davay (2026-11-14) caused by a deleter without a filter. It does not crash every time: it depends on what lies in the sphere.

## What to do
Fill m_prefabFilterList with the prefabs to delete. An empty filter is fine only if the sphere holds nothing but what must go: then ignore the warning.

## When to remove the rule
If TILW changes what an empty filter does (for example, deleting nothing).

## History
- 2026-11-14: crash on Davay.
- 2026-11-20: rule written as a warning (TILW 1.0.128).
