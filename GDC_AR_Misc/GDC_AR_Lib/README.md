# GDC AR Lib

English | [Français](README.fr.md)

A library of features for the missions of the Grèce de canards community, intended for mission makers.

To use it, add GDC AR Lib as a dependency of your addon, or tick it in "Open With Addons" when starting Workbench.

It depends on TILW, PSCore, Reforger Lobby and Coalition VON, see the [backlog](../../BACKLOG.md) (in French).

## New features

### Teleport with time skip

Prefab: `Prefabs/Teleport/GDC_Teleport.et`. Place it as a root entity next to the object used as the starting point (a flag, for example), never as a child of that object.

- **Destination:** the name of an entity in the world. Players arrive spread out around it.
- **`INDIVIDUAL` mode:** each player uses the action and travels alone.
- **`GROUP` mode:** only the group leader sees the action. The leader takes along the group's players within an adjustable radius around the module. AI, dead players and players in a vehicle stay behind.
- **Time skip:** durations of the fade, the black screen and the return of the picture. If all three are 0, the teleport is immediate.
- **Text:** optional, shown on a light veil, for example "Two hours later...".
- **Image:** optional, centred on the black screen, limited to a share of the screen (60% by default) and never enlarged beyond its native resolution.
- **Sound:** game sounds follow the fade, and this can be turned off. Voice and radio are not cut.

### Virtual mortar fire support

Prefab: `Prefabs/Support/GDC_FireSupport_Mortar.et`. Shells are simulated, with no mortar or crew. The module position stands for the position of the tubes, for flight time and range.

Players request fire from a panel opened through the map's radial menu. They choose the ammunition, the number of rounds and the dispersion, within the limits set on the module.

- **Target designation:** `ARCADE` by clicking on the map, with a marker and the impact area shown, or `MILSIM` by coordinates typed by hand, with nothing shown on the map.
- **Access:** minimum rank and/or an item to carry. If both are set, both are required. Without them, the player doesn't see the support.
- **Limits:** number of missions, delay between two missions, allowed or forbidden target areas.
- **Range:** the range rings of each ammunition are visible in the World Editor. A mortar symbol can also be shown on the in-game map.

### Areas shown on the map

Prefab: `Prefabs/Markers/GDC_Marker_Area.et`, a polyline that can be shown on the in-game map with an outline and, if it is closed, a fill. Colours, width and opacity are adjustable. It is also used to define the target areas of the mortar support.

### Per-slot radios

Short-range and long-range radio frequencies, set per slot on playable groups (`SCR_AIGroup`), with Coalition VON. A slot can have several radios. A common long-range frequency can apply to group members with no frequency of their own.

## Other features

- **GDC Loadout Editor:** World Editor tool to edit the loadout of character prefabs, see [docs/GDC_LoadoutEditor.md](docs/GDC_LoadoutEditor.md) (in French).
- **AI artillery:** `Prefabs/AI/Artillery/GDC_ArtilleryFireSupport.et`, mortar fire served by AI, triggered by a TILW mission flag.
- **`GDC_FollowNearestPlayerEntity`:** places an entity on the player closest to a reference, at an adjustable interval.
- **`GDC_ForceGroupActiveLODComponent`:** keeps an AI group in full simulation, regardless of its distance from players.
- **GDC Import Loading Screens:** Resource Manager plugin that imports the loading screens prepared in `_Media` into GDC AR Core.
