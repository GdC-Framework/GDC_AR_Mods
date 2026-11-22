# GDC Mission Maker Tools

English | [Français](README.fr.md)

Tools for the Enfusion Workbench ("Arma Reforger Tools") that make life easier for mission makers.

/!\ No need to add this addon to the dependencies of your mission. Load it with "Open With Addons" in the Enfusion Launcher (right-click on your mission mod).

What does this addon offer?

  * An entity library, 3den style
  * A loadout editor
  * A mission skeleton in one click
  * A mission check


## An entity library, 3den style

The World Editor's Resource Browser gives access to every loaded entity: there are literally thousands of them, and only a small part is useful to mission makers. GDC Mission Maker Tools provides a plugin that builds a library of the entities useful to mission makers, directly available in the World Editor's native **Prefab Library** panel.

User guide: [docs/GDC_MMLibraries.md](docs/GDC_MMLibraries.md).


## A (slightly) better loadout editor

A World Editor tool (suitcase icon in the toolbar) to edit the loadout of a character prefab on a single panel: clothing, weapons, equipment and inventory, with the real capacity of each container.

User guide: [docs/GDC_LoadoutEditor.md](docs/GDC_LoadoutEditor.md).


## A mission skeleton in one click

A World Editor plugin that adds to a new mission world everything a GDC mission starts with: layers, managers, Reforger Lobby game mode with the usual settings, TILW framework, briefing sections and Coalition VON frequency config. It only adds what is missing, so it can be run again safely.

User guide: [docs/GDC_MissionSkeleton.md](docs/GDC_MissionSkeleton.md).


## A mission check

A World Editor plugin (Plugins > GDC > Check Mission, Ctrl+Alt+C) that checks the open mission world without changing it: missing resources, MM library proxies, entities that lost their prefab, TILW pitfalls... The blocks to run and the language (English or French) are set in Plugins > Settings > Check Mission. The report is in the Log Console (filter `[GDC_CHECK]`): each finding says where, what goes wrong and what to do, with a link that selects the entity.

User guide and list of every check: [docs/GDC_CheckMission.md](docs/GDC_CheckMission.md). Why a rule exists, the evidence and what to do: [mission check knowledge base](GDC_AR_MissionCheck_KB/README.md).
