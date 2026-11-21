# Excluded addon loaded (GDC)

English | [Français](gdc-excluded-addons.fr.md)

Rule 2.5 · error · GDC convention

## What happens
Some addons break GDC missions. If one is loaded in the Workbench, the mission may depend on it, and then the server and the players load it too.

## Excluded addons
| Addon | Excluded | Why |
|---|---|---|
| Anarchy Map Markers & Drawings (69A510CE600D1126) | in Reforger Lobby missions | conflicts with Reforger Lobby: both handle the input of the quick marker menu (`OnInputQuickMarkerMenu`) |

The list is in the rule catalogue (`GDC_MMCheckCatalog.GetExcludedAddons`): keep both in step.

## What to do
Remove the addon from the dependencies of the mission addon, and from the server's mod list.

## When to remove an addon from the list
When the conflict is fixed, by the addon or by Reforger Lobby: check it in game before removing it.

## History
- 2026-11-18: Anarchy Map Markers & Drawings already excluded from the server configs of Reforger Lobby missions.
- 2026-11-21: rule written.
