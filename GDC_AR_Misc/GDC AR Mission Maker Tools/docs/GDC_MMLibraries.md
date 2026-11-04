# MM entity libraries (GDC Mission Maker Tools)

English | [Français](GDC_MMLibraries.fr.md)

The **Update MM Libraries** plugin sorts the entities useful to mission makers into the World Editor's **Prefab Library** panel. The libraries are built from the addons you have loaded:

- **MM Forces:** groups, static weapons, land and air vehicles, per faction. They come from the faction catalogs that Bohemia and faction addons (RHS...) maintain for the Game Master. Single characters are left out, since only groups get an AI.
- **MM Markers:** briefing map markers (Reforger Lobby, Coalition Lobby, TILW, GDC) and areas drawn on the map (TILW, GDC AR Lib), per addon.
- **MM Systems:** vanilla waypoints, TILW triggers and utilities, GDC AR Lib modules (AI, support, teleport).

Anything not listed stays available in the Resource Browser.

## Create the libraries

1. Open your project with GDC Mission Maker Tools ticked in "Open With Addons".
2. Run **Plugins > GDC > Mission Maker > Update MM Libraries**.
3. Choose the folder of the new local addon **GDC Mission Maker Libraries**. By default, it goes next to your other Workbench projects. Click Create.
4. Close Workbench, then reopen your project from the Launcher with "Open With Addons", ticking **GDC Mission Maker Libraries**. It brings GDC Mission Maker Tools with it.

If you chose a folder other than the default one, first add it to the Launcher with "Add Existing Project".

GDC Mission Maker Libraries is yours alone: never publish it, and never add it as a dependency.

## Use them

Drag a prefab from the Prefab Library panel into the viewport. The **original prefab** is placed, not the library shortcut, so the mission depends neither on the libraries nor on this addon.

If the prefab's addon is not loaded, nothing is placed and a dialog names the addon to load.

## Update them

Run **Update MM Libraries** again after changing your loaded addons. The libraries then mirror them: new prefabs are added, and prefabs of addons no longer loaded are removed.

## Tests

Automated tests (maintainers): [Tools/MMLibrariesTests](../../../Tools/MMLibrariesTests/README.md) (in French).
