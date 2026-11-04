# MM entity libraries (GDC Mission Maker Tools)

English | [Français](GDC_MMLibraries.fr.md)

The **Update MM Libraries** plugin sorts the entities useful to mission makers into the World Editor's **Prefab Library** panel. The libraries are built from the addons you have loaded:

- **MM Forces:** groups, static weapons, land and air vehicles, per faction. They come from the faction catalogs that Bohemia and faction addons (RHS...) maintain for the Game Master.
- **MM Markers:** briefing map markers (Reforger Lobby, Coalition Lobby, TILW, GDC) and areas drawn on the map (TILW, GDC AR Lib), per addon.
- **MM Systems:** vanilla waypoints, TILW triggers and utilities, GDC AR Lib modules (AI, support, teleport).

Anything not listed stays available in the Resource Browser.

## First run: create the libraries

1. Open your project with GDC Mission Maker Tools ticked in "Open With Addons".
2. Run **Plugins > GDC > Mission Maker > Update MM Libraries**.
3. Choose the folder of the new local addon **GDC Mission Maker Libraries**. By default, it goes next to your other Workbench projects. Click Create.
4. Close Workbench, then reopen your project from the Launcher with "Open With Addons", ticking **GDC Mission Maker Libraries** (no need to tick GDC Mission Maker Tools anymore: your local library addon brings it along).

If you chose a folder other than the default one, first add it to the Launcher with "Add Existing Project".

<details>
<summary>But why on earth do I have to do this?</summary>

> GDC Mission Maker Tools is a mod you download. It is therefore read-only: nothing can be written into it... not even the library of the mods you use.
>
> To write your library, we therefore have to create a small local mod, yours alone, that stores the data of your own modset (yours alone, but don't go crazy with the modset all the same).

</details>

⚠️ **Never add GDC Mission Maker <ins>Libraries</ins> as a dependency. Your library addon is local and never published, so no other player could play the mission! So be nice: right-click in the Workbench Launcher and add it with "Open With Addons".**

## Use the library

1. Select the Prefab Library panel.
2. Choose one of the MM libraries (Forces, Markers, etc.).
3. Drag and drop into the 3D view.

PS: if the prefab's addon is not loaded, nothing is placed and a dialog names the addon to load.

## I loaded other addons or removed some addons from my mission: update the library

Run **Update MM Libraries** again: new prefabs are added, and prefabs of addons no longer loaded are removed. No risk, you can do it as many times as you like.

## Tests for the addon maintainers (AI food, ha!)

Automated tests (maintainers): [Tools/MMLibrariesTests](../../../Tools/MMLibrariesTests/README.md) (in French).
