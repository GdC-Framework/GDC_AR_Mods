# GDC Core

Socle commun des parties Arma Reforger de la communauté Grèce de Canards : les mods que tout le monde charge, et les réglages de jeu propres à GDC.

- Workshop : https://reforger.armaplatform.com/workshop/663710CD8BEB01D4-GDCARCore
- Historique des versions : [CHANGELOG.md](../../CHANGELOG.md), section GDC Core.

## Mods embarqués (dépendances)

| Mod | Rôle |
|---|---|
| ACE All-in-One | Médical avancé, explosifs, captifs, porter/traîner, etc. |
| Coalition VON | Radios (réseaux CP et LP) |
| COALITION Squad Interface | Étiquettes et couleurs d'équipe au-dessus des joueurs. Le radar n'est pas utilisé. |
| Fixed Load Casualty Vehicles | Charger un blessé dans un véhicule |
| Gray's Adjustable Doors | Portes entrouvertes, ouverture par les IA |
| Improved Blood Effect | Effets de sang |
| Realism Overhaul - Ballistics | Balistique des munitions et matériaux |
| WCS Armbands | Brassards |
| You can sit | S'asseoir sur les décors et dans les véhicules |

## Réglages intégrés à GDC Core

Ces réglages remplacent des mods qui étaient auparavant des dépendances (version 2.5.0) :

- **Inconscience discrète** : les étiquettes de nom ne montrent jamais qu'un joueur est inconscient. Cela vaut aussi pour l'icône de rôle de COALITION Squad Interface. Le Game Master et les spectateurs voient toujours l'état réel. Script `GDC_SCR_NameTagData_NoUnconscious.c`, remplace *No Icon For Uncon*.
- **Arme conservée** : un joueur qui tombe inconscient range son arme au lieu de la lâcher. Une grenade dégoupillée tombe toujours. Script `GDC_SCR_CharacterControllerComponent_NoWeaponDrop.c`, remplace *No Weapon Drop Unconscious*.
- **Budgets du Game Master** à 100 000 (entités, IA, véhicules, systèmes). Surcharge de `Prefabs/Editor/EditorManager.et`, remplace *FNG GM Budgetless*.
- **Pas de supplies** : la ressource est désactivée pour tous les modes de jeu, et le kit de réparation répare jusqu'à 100 %. Surcharges de `Prefabs/MP/Modes/GameMode_Base.et` et `Prefabs/Items/Equipment/Kits/RepairKit_01/RepairKit_01_base.et`, remplacent *No Supply System*.
- **Consommation de carburant** à l'échelle normale (`GDC_FuelConsumption.c`).
- **Météo** : séchage du sol, des fissures et des flaques réglé sur le temps de jeu, avec des durées propres à GDC (8, 10 et 15). Voir `Terrains/Common/WeatherPresets/WeatherParameters.conf` ; le jeu de base ne fixe pas ces valeurs.
- **Écrans de chargement** des missions GDC (`UI/MissionLoadingScreens`).

## Tester

Depuis la racine du dépôt, Workbench fermé :

```powershell
# Compilation des scripts de tout le modset (3 s)
powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 -Compile
# Réglages de GDC Core en jeu, serveur dédié + 2 clients sur Arland (~3 min)
powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 -Mission Uncon
```

Voir [Tools/MissionTests/README.md](../../Tools/MissionTests/README.md).

## Avant de publier

- **Ouvrir GDC Core dans le Workbench** après tout ajout ou retrait de ressource (prefab, config, avec son `.meta`) fait hors du Workbench. Il reconstruit l'index du projet (`resourceDatabase.rdb`, non suivi par git). Sans cet index, une surcharge écrite à la main n'est pas appliquée par le jeu.
- **Retirer les scripts `GDC_EnfusionMCP`** s'ils ont été installés (`wb_handlers_remove`).
- **Mettre à jour** la liste des mods du serveur (`GDC_AR_Missions/_ServerConfigs/addons.json`) quand une dépendance change, et ajouter la version à [CHANGELOG.md](../../CHANGELOG.md).
