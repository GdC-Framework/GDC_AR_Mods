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
| OpsTrack | Enregistrement des parties. Réglé sur chaque serveur (`OpsTrackSettings.json` du profil, clé d'API) ; sans clé, il n'enregistre rien. |
| Realism Overhaul - Ballistics | Balistique des munitions et matériaux |
| WCS Armbands | Brassards |
| You can sit | S'asseoir sur les décors et dans les véhicules |

**Ordre des dépendances à garder** : dans `addon.gproj`, ACE All in One doit rester avant Coalition VON. C'est ce qui fait charger ACE avant CVON et Reforger Lobby :

- sinon, la version d'ACE Radio de `ActivateVON` passerait avant celle de CVON et réactiverait la voix du jeu de base ;
- et celle d'ACE Medical de `UpdateConsciousness` remplacerait celle de Reforger Lobby.

La supervision des mods le vérifie (`Tools/ModSync/mod_supervision.py order`).

## Réglages intégrés à GDC Core

- **Inconscience discrète** : les étiquettes de nom ne montrent jamais qu'un joueur est inconscient. Cela vaut aussi pour l'icône de rôle de COALITION Squad Interface. Le Game Master et les spectateurs voient toujours l'état réel. Script `GDC_SCR_NameTagData_NoUnconscious.c`, remplace *No Icon For Uncon*.
- **Arme conservée** : un joueur qui tombe inconscient range son arme au lieu de la lâcher. Une grenade dégoupillée tombe toujours. Script `GDC_SCR_CharacterControllerComponent_NoWeaponDrop.c`, remplace *No Weapon Drop Unconscious*.
- **Budgets du Game Master** à 100 000 (entités, IA, véhicules, systèmes). Surcharge de `Prefabs/Editor/EditorManager.et`, remplace *FNG GM Budgetless*.
- **Pas de supplies** : la gestion des ressources est désactivée pour tous les modes de jeu, et le kit de réparation répare jusqu'à 100 %. Surcharges de `Prefabs/MP/Modes/GameMode_Base.et` et `Prefabs/Items/Equipment/Kits/RepairKit_01/RepairKit_01_base.et`, remplacent *No Supply System*.
- **Consommation de carburant** à l'échelle normale (`GDC_FuelConsumption.c`).
- **Météo** : séchage du sol, des fissures et des flaques réglé sur le temps de jeu, avec des durées propres à GDC (8, 10 et 15). Voir `Terrains/Common/WeatherPresets/WeatherParameters.conf` ; le jeu de base ne fixe pas ces valeurs.
- **Écrans de chargement** des missions GDC (`UI/MissionLoadingScreens`).
