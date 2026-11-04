# GDC AR Mission Maker Tools

English | [Français](README.fr.md)

Workbench tools for the mission makers of the Grèce de canards community.

Never add this addon as a dependency of a mission: load it only to edit, by ticking it in "Open With Addons" when starting Workbench. Missions built with it don't depend on it, and players never download it.

## MM libraries

Shortcuts sorted for mission makers in the World Editor's **Prefab Library** panel, built from the addons you have loaded:

- **MM Forces:** groups, static weapons, land and air vehicles, per faction. They come from the faction catalogs that Bohemia and faction addons (RHS...) maintain for the Game Master. Single characters are left out, since only groups get an AI.
- **MM Markers:** briefing map markers (Reforger Lobby, Coalition Lobby, TILW, GDC) and areas drawn on the map (TILW, GDC AR Lib), per addon.
- **MM Systems:** vanilla waypoints, TILW triggers and utilities, GDC AR Lib modules (AI, support, teleport).

Anything not listed stays available in the Resource Browser.

### Create the libraries

1. Open your project with GDC AR Mission Maker Tools ticked in "Open With Addons".
2. Run **Plugins > GDC > Mission Maker > Update MM Libraries**.
3. Choose the folder of the new local addon **GDC AR MM Libraries**. The default one is next to your other Workbench projects. Click Create.
4. Close Workbench, then open your project from the Launcher with "Open With Addons", ticking **GDC AR MM Libraries**. It brings GDC AR Mission Maker Tools with it.

If you chose a folder other than the default one, first add it to the Launcher with "Add Existing Project".

GDC AR MM Libraries is yours alone: never publish it, and never add it as a dependency.

### Use them

Drag a prefab from the Prefab Library panel into the viewport. The **original prefab** is placed, not the library shortcut, so the mission depends neither on the libraries nor on this addon.

If the prefab's addon is not loaded, nothing is placed and a dialog names the addon to load.

### Update them

Run **Update MM Libraries** again after changing your loaded addons. The libraries then mirror them: new prefabs are added, and prefabs of addons no longer loaded are removed.

## Tests

Automated tests: [Tools/MMLibrariesTests](../../Tools/MMLibrariesTests/README.md) (in French). They run the plugin from the command line:

```
-wbModule=ResourceManager -plugin=GDC_MMCatalogGeneratorPlugin -run [-mmLibraryFolder="<folder>"]
```
