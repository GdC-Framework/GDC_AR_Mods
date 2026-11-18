# GDC Core

English | [Français](README.fr.md)

The common base of every Arma Reforger game of the Grèce de canards community: the addons everybody loads, and the game settings specific to GDC.

- Workshop: https://reforger.armaplatform.com/workshop/663710CD8BEB01D4-GDCARCore
- Version history: [CHANGELOG.md](../../CHANGELOG.md), GDC Core section (in French, release notes in English).

## Bundled addons (dependencies)

| Addon | Purpose |
|---|---|
| ACE All-in-One | Advanced medical, explosives, captives, carrying and dragging, etc. |
| Coalition VON | Radios (short range and long range nets) |
| COALITION Squad Interface | Name tags and team colours above players. The radar is not used. |
| Fixed Load Casualty Vehicles | Loading a casualty into a vehicle |
| Gray's Adjustable Doors | Half-open doors, doors opened by AI |
| Improved Blood Effect | Blood effects |
| OpsTrack | Game recording. Set up on each server (`OpsTrackSettings.json` in the server profile, API key); without a key it records nothing. |
| Realism Overhaul - Ballistics | Ammunition and material ballistics |
| WCS Armbands | Armbands |
| You can sit | Sitting on props and in vehicles |

**Keep the dependency order**: in `addon.gproj`, ACE All in One must stay before Coalition VON. This is what loads ACE before Coalition VON and Reforger Lobby. Otherwise:

- ACE Radio's `ActivateVON` would run before Coalition VON's and turn the vanilla voice chat back on;
- ACE Medical's `UpdateConsciousness` would replace Reforger Lobby's.

The addon supervision checks it (`Tools/ModSync/mod_supervision.py order`).

## Settings built into GDC Core

- **Hidden unconsciousness**: name tags never show that a player is unconscious, including the role icon of COALITION Squad Interface. The Game Master and spectators still see the real state. Script `GDC_SCR_NameTagData_NoUnconscious.c`, replaces *No Icon For Uncon*.
- **Weapon kept**: a player who falls unconscious puts his weapon away instead of dropping it. A live grenade in hand is still dropped. Script `GDC_SCR_CharacterControllerComponent_NoWeaponDrop.c`, replaces *No Weapon Drop Unconscious*.
- **Game Master budgets** raised to 100 000 (entities, AI, vehicles, systems). Override of `Prefabs/Editor/EditorManager.et`, replaces *FNG GM Budgetless*.
- **No supplies**: resource management is disabled for every game mode, so repairing, healing and rearming cost nothing; the repair kit repairs up to 100 %. Overrides of `Prefabs/MP/Modes/GameMode_Base.et` and `Prefabs/Items/Equipment/Kits/RepairKit_01/RepairKit_01_base.et`, replace *No Supply System*.
- **Fuel consumption** at the normal scale (`GDC_FuelConsumption.c`).
- **Weather**: drying of the ground, cracks and puddles follows game time, with GDC durations (8, 10 and 15). See `Terrains/Common/WeatherPresets/WeatherParameters.conf`; the base game does not set these values.
- **Loading screens** of the GDC missions (`UI/MissionLoadingScreens`).
