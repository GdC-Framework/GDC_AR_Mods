# Backlog

Travaux à faire sur les mods et les missions, sans ordre de priorité imposé. Un point terminé est retiré d'ici ; s'il est publié, il passe dans le [changelog](CHANGELOG.md).

## GDC Core

- Réintégrer **Fixed Load Casualty Vehicles** : surcharge de `Vehicle_base.et` (tout véhicule peut recevoir un blessé) et script à réécrire. Le sien charge le blessé dans une place patient puis aussi dans une place passager, en validant l'action deux fois.
- Non testé automatiquement : la grenade dégoupillée qui tombe quand un joueur perd connaissance.

## GDC AR Lib

### Réduire les dépendances de l'addon

`addon.gproj` déclare TILW, PSCore, Reforger Lobby et Coalition VON : toute mission qui charge GDC AR Lib entraîne ces mods, même si elle n'utilise pas les fonctions concernées.

| Mod | Origine relevée dans le code |
|---|---|
| TILW | `GDC_ArtilleryFireSupportComponent` (module d'artillerie IA) : déclenchement par flag de mission |
| Reforger Lobby, PSCore | Aucune référence `PS_` dans les scripts, prefabs et layouts de la bibliothèque : à confirmer, puis retirer si rien ne les utilise |
| Coalition VON | `GDC_SCR_GroupsManagerComponent_SlotRadios` (radios par slot) |

### Appui feu

- Regrouper les appuis d'une mission dans une seule entité portant la liste de ses appuis. Une première tentative a été annulée le 2026-10-30 : à refaire avec Workbench fermé, code, prefab et layer changés d'un seul coup.
- Étendre le module à d'autres appuis : aérien, CAS, logistique.

### Radios par slot

- Corriger la description de `m_sGDC_LongRangeFrequency` : elle dit que les radios de véhicules sont réglées sur cette fréquence, or aucun script GDC ne le fait plus.
- Dérouler les tests sur serveur dédié (préréglages par slot, reconnexion, respawn). Ils étaient encore à faire en octobre : à confirmer.

### Sac radio avec rangement pour la longue portée CVON

Le combiné partagé de CVON (*Grab Handset* sur le dos du porteur, cordon de 3 m) exige un manpack : une radio longue portée CVON portée **et** un sac radio sur le dos, c'est-à-dire un sac portant un composant radio, même désactivé (`CVON_Manpack.IsWearingRadioBackpack`). Les sacs radio du jeu de base (AN/PRC-77, R-107M) n'ont aucun rangement : leur seul composant d'objet est `SCR_DeployableInventoryItemInventoryComponent` (point de réapparition déployable). Le FILBE radio de RHS non plus : RHS y a désactivé le rangement hérité du FILBE. La radio CVON (PRC-117, R-168 : 1,3 kg, volume 300, 10×10×10) doit donc aller ailleurs.

Prefab enfant dans GDC AR Lib, par exemple `GDC_Radio_ANPRC77_Backpack.et` (puis R-107M au besoin) :

- Couper la radio vanilla, comme les sacs « inert » de CRF : `BaseRadioComponent` et `SCR_RadioComponent` désactivés. Le sac reste reconnu par CVON.
- Remplacer le composant d'objet, sans en ajouter un second (les deux héritent d'`InventoryItemComponent`) : désactiver `SCR_DeployableInventoryItemInventoryComponent`, ajouter `SCR_UniversalInventoryStorageComponent` avec les attributs de la PRC-77 (nom, icône, poids, 2×2, `SLOT_BACKPACK`) et un `m_fMaxWeight`. Variante : `SCR_FilteredInventoryStorageComponent` limité aux radios.
- Retirer le déploiement : `SCR_RestrictedDeployableSpawnPointComponent` désactivé, actions de déploiement remplacées par celles d'un sac (ouvrir, s'équiper, ramasser).
- Garder `BaseLoadoutClothComponent`, le maillage et les sons.
- Choix à faire : rangement libre ou réservé aux radios, poids maximum.

Tests, d'abord sur Arland :

- Inventaire : s'équiper, ranger et sortir la radio, poids.
- Sac posé puis ramassé, mort du porteur : le contenu reste-t-il ?
- Radios par slot : la radio rangée dans ce sac est-elle bien réglée ?
- Serveur dédié avec deux clients : *Grab Handset*, haut-parleur du porteur.

Repli sans prefab : `m_bManpackNeedsRadioBackpack` à 0 sur `CVON_VONGameModeComponent`. Toute radio longue portée portée compte alors comme manpack, rangée dans un sac ordinaire, sans le visuel du poste dorsal.

### Filtre de notifications

Remplacer `m_bDisableNotifications` de Reforger Lobby, qui bloque toutes les notifications des joueurs vivants sauf les pings GM (400-499), dont les constantes ACE. Cette cause reste à confirmer en jeu : passer le réglage à 0 et faire prendre le pouls par un joueur non admin.

- D'abord, regarder en jeu ce qui s'affiche avec ce réglage à 0. `PS_GameMode_Lobby.et` règle déjà `m_iKillFeedType` à `DISABLED`, et `SCR_NotificationSenderComponent` peut couper les départs (`m_bShowPlayerLeftNotification`) : le filtre n'est peut-être plus nécessaire, ou seulement pour le reste (arrivées, factions, messages du GM).
- Si le filtre reste utile, ajouter un prefab à poser dans la mission : présent, il active le filtre ; absent, rien ne change.
- Lister les notifications masquées dans un fichier de configuration (`.conf`) : un fichier par défaut dans GDC AR Lib, remplaçable par mission. Le filtre ne bloque que ce qui est listé, donc les notifications des mods passent par défaut. `Configs/Notifications/Notifications.conf` ne convient pas : il ne décrit que l'affichage et n'a pas de champ pour masquer.
- Ne pas dépendre de Reforger Lobby. Toute mission qui utilise le filtre doit garder `m_bDisableNotifications` à 0, à indiquer dans la documentation.
- Laisser tout voir aux admins et aux joueurs morts ou spectateurs, avec une détection vanilla à vérifier au lobby, au briefing et en spectateur.
- Décider du sort des messages du GM (700 et plus) et de `PLAYER_TELEPORTED_SELF` (117) avec le module `GDC_Teleport`.
- Tester sur serveur dédié avec un client non admin.

## GDC Mission Maker Tools

### Bibliothèques MM (« Update MM Libraries »)

- Proposer un nom à la pose d'un marqueur ou d'une zone servant de repère à un module.
- Évaluer une bibliothèque dédiée au Scenario Framework.
- Signaler à Bacon que l'addon ZU-23 ne déclare `BaconZU23_Turret_Deployed` et `Ural4320_ZU23` dans aucun catalogue de faction, seulement dans un ancien registre Game Master.

### Vérification de mission (« Check Mission »)

- Écrire sur le wiki GDC une page qui présente Check Mission dans les grandes lignes et renvoie aux fiches de la base de connaissance, maintenues seulement sur GitHub (`GDC AR Mission Maker Tools/GDC_AR_MissionCheck_KB/`).
- Faire signaler par la supervision des mods les règles « comportement d'addon » dont l'addon a changé de version depuis leur vérification.

## Noms des mods sans « AR »

Les TITLE des addons ont perdu « AR » (GDC Core, GDC Equipment, GDC Terrains, GDC Loadouts, GDC Mission Maker Tools) ; seul GDC AR Lib le garde, pour le distinguer de GDC_Lib d'Arma 3. Les ID de Core, Equipment, Terrains et Loadouts restent inchangés, ainsi que les dossiers. À la prochaine publication de chaque mod :

- Vérifier le champ du nom dans le dialogue de publication du Workbench (*Publish Project*) et y saisir le nouveau nom s'il affiche encore l'ancien. Core 2.3.1 et Equipment 1.1.4, publiés après le changement du `TITLE`, s'affichent toujours « GDC AR Core » et « GDC AR Equipment » sur le Workshop : le champ semble prérempli avec le nom déjà enregistré sur le Workshop, pas avec le `TITLE`.
- Prévenir les joueurs que l'ancien nom devient le nouveau, et mettre à jour le wiki.

Changer aussi l'ID d'un mod (par exemple « GDC AR Equipment » → « GDC Equipment ») est possible : le GUID, le Workshop, les dépendances, les listes de mods des serveurs et les références `{GUID}chemin` ne bougent pas. À faire au moment de la publication du mod, avec :

- **Passer les lanceurs aux GUID** dans `-addons`, qui accepte les deux (recommandé par le wiki), pour ne plus dépendre des ID : `Tools/MissionTests/run_mission_test.ps1`, `GDC_AR_Lib/tools/run_support_autotest.ps1` et `run_teleport_autotest.ps1`, `Tools/MMLibrariesTests/run_tests.py`.
- **Préfixe `$<ID>:`** : le plugin des écrans de chargement utilise `$GDC AR Core:`, à reprendre si l'ID de GDC Core change. Aucune occurrence trouvée pour Equipment.
- **Relancer *Update MM Libraries*** : l'index `Configs/MM Libraries/LibraryAddons.conf` de l'addon local des bibliothèques enregistre l'ID de chaque mod.
- **Resynchroniser le Workbench après publication** (`Tools/ModSync`) : jusque-là, la copie téléchargée garde l'ancien ID avec le même GUID que le projet local, et le Workbench peut charger l'un ou l'autre.
- Désigner le projet par son nouvel ID dans enfusion-py (ou par le chemin du `.gproj`).

## Coalition VON : fréquence de départ des radios de véhicules

L'évolution demandée dans l'[issue #42](https://github.com/CoalitionArma/Coalition-VON/issues/42) est dans CVON 3.2.4 (vérifié dans son code le 19/11/2026). La radio d'un véhicule démarre sur une fréquence choisie, et y revient à chaque key fill. Le premier réglage rempli l'emporte :

1. `m_sStartFrequency` sur `CVON_VehicleRadioComponent` (le véhicule, par exemple un réseau air pour les hélicoptères) ;
2. `m_sVehicleFrequency` sur `SCR_FactionCallsignInfo` (la faction) ;
3. `m_sVehicleFrequency` sur `CVON_FreqConfig` (la mission) ;
4. sinon, la première fréquence longue portée des canaux du poste.

À faire pour Murabat :

- Dans `Configs/Murabat_Frequencies.conf`, régler `m_sVehicleFrequency` à 48000, retirer le contournement (48000 en tête de `m_aSRFrequencies`) et remettre 31000 en premier.
- Vérifier en jeu qu'après le key fill, la radio d'un véhicule est sur 48000.

## Pont Reforger Lobby – Coalition VON (*GDC RL-CVON*)

Le pont *AFI CVON-RL COMMUNITY Compat* (`66E2C8C95CD0EF22`, 2.1.12) n'est plus maintenu : son auteur ne répond plus à nos demandes. Notre pont *GDC RL-CVON* le remplace : il est écrit, et il ne reste que le test avec les joueurs, puis la mise en service.

- **Constat du 18/11/2026** : il remplace `SCR_VONController.EOnFixedFrame` par une copie d'une ancienne version de CVON, sans appeler celle de CVON. Les ajouts de CVON 3.2.3 qui n'y figurent pas ne s'exécutent donc jamais sur nos serveurs, par exemple `WriteHeartbeat` et `RefreshListenedRadios`. L'effet en jeu n'est pas vérifié.
- **Fait le 19/11/2026** : addon séparé *GDC RL-CVON* (`GDC_AR_ModPack/GDC_RL_CVON`, ID Workshop `BCA23F9DDF701E02`). Publié le 19/11/2026 en 0.0.1, non listé, et chargé sur le serveur GDC à la place d'AFI pour Murabat. Il passe par les points d'extension de CVON (`IsSpectatorTalker`, `HearOverChannel`, `ComputeSpectatorLR`) au lieu de copier `EOnFixedFrame` et `InitializeRadios`. Il coupe aussi `LobbyVoNAdminEnable`, qu'AFI oubliait, et laisse le VON de Reforger Lobby fonctionner dans une mission sans CVON. Compilation OK (`-Compile`). `Tools/MissionTests` l'utilise à la place d'AFI.
- **Défaut du lobby corrigé et vérifié le 19/11/2026** (test `RLCVON` de `Tools/MissionTests`, ex-`LobbyVoice`). Quand un joueur prenait un slot, il n'entendait plus les autres, et le contournement était de prendre un slot puis de le quitter. Cause : prendre un slot téléporte l'entité de lobby du joueur sur le terrain, l'entité de l'autre joueur ne lui est plus transmise, et AFI coupait la voix d'un joueur sans entité. Avec *GDC RL-CVON*, les deux clients gardent l'autre joueur à plein volume. Avec AFI, le même test montre que chaque client retire l'autre joueur de ses voix après le slot.
- **Test en direct avec les joueurs prévu le dimanche 22/11/2026 à 17h00** : la séance à trois joueurs de `GDC_AR_ModPack/GDC_RL_CVON/PLAN_DE_TEST.md` (6 cas : son réel, radios, radio de véhicule, combiné, reconnexions). Le reste est couvert par le test automatique `RLCVON`.
- Si tout est OK : publier en public (1.0.0), mettre le pont à la place d'AFI dans `_ServerConfigs/addons.json` et dans les listes des missions, et mettre à jour la règle d'ordre des listes de mods (skill `gdc-mod-supervision`, README de `Tools/ModSync`).

## Supervision des mods sur le serveur GDC

But : que la supervision des mods (`Tools/ModSync`, skill `gdc-mod-supervision`) ne dépende plus du PC de l'admin qui a Claude Code. Le serveur GDC fait le travail mécanique, et l'analyse finale se fait avec Claude Code ou, sans abonnement, en déposant les résultats dans claude.ai.

Le serveur dédié Linux est déjà installé, c'est notre serveur de jeu. Il sait télécharger un mod dans une version précise (champ `version` de la config), dans un dossier séparé (`-addonDownloadDir`).

- Faire tourner la supervision avec une instance à part du serveur dédié (profil, port et dossier de mods distincts), sans toucher au serveur de jeu ni à ses mods.
- Suivre les mises à jour des mods du modset et garder l'ancienne version avant chaque mise à jour, comme `before` le fait sur le PC.
- Porter sous Linux ce qui en dépend : `mod_supervision.py` et `mod_conflicts.py` (chemins `My Games`, lecteur de `.pak` d'enfusion-mcp-py), et le test de compilation (`run_mission_test.ps1 -Compile`, aujourd'hui en PowerShell avec l'exécutable Windows).
- Ne publier que des résultats compacts : `report.json`, `conflicts.json`, comparaisons des fichiers qui touchent GDC. Les mods eux-mêmes ne quittent pas le serveur.
- Écrire un skill claude.ai « analyse de supervision » qui prend ces fichiers en entrée, pour un admin sans Claude Code (plan gratuit : skills et exécution de code disponibles, sans accès au PC ni, par défaut, au réseau hors domaines approuvés).
- À vérifier avant de copier des fichiers de mods ailleurs que sur le serveur : les conditions du Workshop et les licences des mods.

## Missions

### Davay, duy na Gruwice

- Régler la durée du noir sur Zimnitrita, pour le trajet d'environ 7 km depuis la base.
- Vérifier que les IA utilisent les passerelles et échelles ajoutées, probablement absentes du navmesh de Zimnitrita.

Suite du test automatique du 15/11/2026 (`Tools/MissionTests`, 124 réussis, 7 échecs) :

- **M2HB de PR3 jamais servie** : `PR3_grp` (point `PR3_WP_GetIn`, GetInNearest) ne monte pas dans `PR3_Emplacement`. Ce n'est ni la faction `US` de la composition ni le LOD des IA (les patrouilles bougent). Ajouter au test un diagnostic de PR3 (positions des 2 hommes, distance à la M2HB, point de passage courant, LOD des agents), puis corriger.
- **Groupe « Bloc » vers `Wp_Def1` perdu** : parti de Spawn_1 (port), il finit en 9810 / 4719 au bord de l'eau, à 920 m de son point au nord. Le chemin vers la bande nord ne passe sans doute pas par le canal : changer son point d'apparition ou lui donner un point intermédiaire.
- **QRF arrêtés en 10136 / 5365**, à 110 m de la place Lénine (3 groupes), et un en 9913 / 5236 près de Spawn_3. Peut-être un combat contre le PL invulnérable à l'hôtel (biais du test) : ajouter l'état de combat des groupes au test pour trancher.
- **Tireur de `Pat_Ouest`** absent de sa tourelle en fin de partie : à surveiller.
- Relancer le test après corrections, puis commiter la mission (renforts commités en a273edc).
- À la main (passe 3, avec des joueurs) : audio des radios, visuel (briefing, marqueurs, interface d'appui feu, fondu), passage au sous-sol de l'hôtel, difficulté, outils GM, version publiée sur un vrai serveur.

### Maintenance après le retrait de GameMasterFX et de RHIB

En 1.9, GameMasterFX 1.7.0 et RHIB 1.0.20 cassent la compilation des scripts. Ils ont été retirés des dépendances de GDC Core et de GDC Equipment (novembre 2026). Les missions qui les utilisent ne s'ouvrent plus correctement sans eux : les charger à part dans le Workbench pour les reprendre.

- **Entre Deux** : les RHIB `RHIB_LZ_*` (`framework.layer`) portent l'interaction « Fin de mission ». Les remplacer par un autre objet et corriger le briefing (« sur l'avant du RHIB », `briefing.layer`).
- **Gulf Coast Mission 06 et 06b** : 10 feux et 6 fumées de GameMasterFX (`build.layer`). Le terrain reste de toute façon bloqué par le navmesh de Gulfcoast.
- **Return To Babaorum** : 2 feux et 1 fumée de GameMasterFX (`items.layer`).
- Pour chaque mission, choisir entre retirer les éléments, les remplacer par des éléments du jeu de base, ou déclarer le mod comme dépendance de la mission (`addons.json`) une fois corrigé par son auteur.
- Attention : enregistrer un monde sans ces mods chargés supprime leurs prefabs des layers.
