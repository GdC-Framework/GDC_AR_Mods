# Backlog

Travaux à faire sur les mods et les missions, sans ordre de priorité imposé. Un point terminé est retiré d'ici ; s'il est publié, il passe dans le [changelog](CHANGELOG.md).

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

## GDC Mission Maker Tools

### Bibliothèques MM (« Update MM Libraries »)

- Proposer un nom à la pose d'un marqueur ou d'une zone servant de repère à un module.
- Évaluer une bibliothèque dédiée au Scenario Framework.
- Signaler à Bacon que l'addon ZU-23 ne déclare `BaconZU23_Turret_Deployed` et `Ural4320_ZU23` dans aucun catalogue de faction, seulement dans un ancien registre Game Master.

## Noms des mods sans « AR »

Les TITLE des addons ont perdu « AR » (GDC Core, GDC Equipment, GDC Terrains, GDC Loadouts, GDC Mission Maker Tools) ; seul GDC AR Lib le garde, pour le distinguer de GDC_Lib d'Arma 3. Les ID de Core, Equipment, Terrains et Loadouts restent inchangés, ainsi que les dossiers. À la prochaine publication de chaque mod :

- Vérifier que le dialogue de publication reprend le nouveau nom, et que les anciens liens Workshop (`…-GDCARCore`, `…-GDCARTerrains` dans les READMEs) mènent toujours à la page.
- Prévenir les joueurs que l'ancien nom devient le nouveau, et mettre à jour le wiki.

## Coalition VON : fréquence de départ des radios de véhicules

L'évolution demandée dans l'[issue #42](https://github.com/CoalitionArma/Coalition-VON/issues/42) est annoncée par l'équipe CVON pour une version postérieure à la 3.0.3. D'après son commentaire, les radios de véhicules démarrent sur la première fréquence longue portée de la configuration et y reviennent à chaque key fill. Trois réglages sont annoncés : `m_sVehicleFrequency` sur `CVON_FreqConfig` (mission) ou sur les infos d'indicatif d'une faction, et `m_sStartFrequency` sur `CVON_VehicleRadioComponent` (véhicule).

La copie locale de CVON est en 3.0.3 et ne contient pas encore ces attributs. Une fois CVON mis à jour :

- Vérifier le comportement et le nom des attributs.
- Murabat : retirer le contournement de `Configs/Murabat_Frequencies.conf` (48000 placé en tête de `m_aSRFrequencies`) et remettre 31000 en premier.

## Réécrire le pont Reforger Lobby – Coalition VON

Le pont *AFI CVON-RL COMMUNITY Compat* (`66E2C8C95CD0EF22`, 2.1.12) n'est plus maintenu : son auteur ne répond plus à nos demandes. Il faut écrire le nôtre.

- **Constat du 18/11/2026** : il remplace `SCR_VONController.EOnFixedFrame` par une copie d'une ancienne version de CVON, sans appeler celle de CVON. Les ajouts de CVON 3.2.3 qui n'y figurent pas ne s'exécutent donc jamais sur nos serveurs, par exemple `WriteHeartbeat` et `RefreshListenedRadios`. L'effet en jeu n'est pas vérifié.
- **Fait le 19/11/2026** : addon séparé *GDC RL-CVON* (`GDC_AR_ModPack/GDC_RL_CVON`, GUID `BCA23F9DDF701E02`, non publié). Il passe par les points d'extension de CVON (`IsSpectatorTalker`, `HearOverChannel`, `ComputeSpectatorLR`) au lieu de copier `EOnFixedFrame` et `InitializeRadios`. Il coupe aussi `LobbyVoNAdminEnable`, qu'AFI oubliait, et laisse le VON de Reforger Lobby fonctionner dans une mission sans CVON. Compilation OK (`-Compile`). `Tools/MissionTests` l'utilise à la place d'AFI.
- **Défaut du lobby corrigé et vérifié le 19/11/2026** (test `LobbyVoice` de `Tools/MissionTests`). Quand un joueur prenait un slot, il n'entendait plus les autres, et le contournement était de prendre un slot puis de le quitter. Cause : prendre un slot téléporte l'entité de lobby du joueur sur le terrain, l'entité de l'autre joueur ne lui est plus transmise, et AFI coupait la voix d'un joueur sans entité. Avec *GDC RL-CVON*, les deux clients gardent l'autre joueur à plein volume. Avec AFI, le même test montre que chaque client retire l'autre joueur de ses voix après le slot.
- Tester à plusieurs sur le serveur dédié, avec TeamSpeak : parole dans le lobby et au briefing, salons de spectateurs, passage en spectateur à la mort, joueur coupé dans le jeu, radios et préréglages par slot, radio de véhicule, combiné d'un sac radio, respawn, reconnexion.
- Ensuite : publier l'addon, le mettre à la place d'AFI dans `_ServerConfigs/addons.json` et dans les listes des missions, et mettre à jour la règle d'ordre des listes de mods (skill `gdc-mod-supervision`, README de `Tools/ModSync`).

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

- Remplacer les `TILW_TeleportInteraction` des drapeaux par le module `GDC_Teleport`, avec le déclenchement par le CdO.
- Régler la durée du noir sur Zimnitrita, pour le trajet d'environ 7 km depuis la base.
- Construire la garnison ennemie, les positions renforcées, les véhicules et les occupants de l'hôtel.
- Construire l'arrivée de renforts par le port.
- Ajouter l'objet de renseignement et son interaction dans l'hôtel.
- Vérifier le nombre de slots pour 10 à 15 joueurs : un seul groupe jouable (`RifleSquad`) pour l'instant.
- Vérifier que les IA utilisent les passerelles et échelles ajoutées, probablement absentes du navmesh de Zimnitrita.

### Maintenance après le retrait de GameMasterFX et de RHIB

En 1.9, GameMasterFX 1.7.0 et RHIB 1.0.20 cassent la compilation des scripts. Ils ont été retirés des dépendances de GDC Core et de GDC Equipment (novembre 2026). Les missions qui les utilisent ne s'ouvrent plus correctement sans eux : les charger à part dans le Workbench pour les reprendre.

- **Entre Deux** : les RHIB `RHIB_LZ_*` (`framework.layer`) portent l'interaction « Fin de mission ». Les remplacer par un autre objet et corriger le briefing (« sur l'avant du RHIB », `briefing.layer`).
- **Gulf Coast Mission 06 et 06b** : 10 feux et 6 fumées de GameMasterFX (`build.layer`). Le terrain reste de toute façon bloqué par le navmesh de Gulfcoast.
- **Return To Babaorum** : 2 feux et 1 fumée de GameMasterFX (`items.layer`).
- Pour chaque mission, choisir entre retirer les éléments, les remplacer par des éléments du jeu de base, ou déclarer le mod comme dépendance de la mission (`addons.json`) une fois corrigé par son auteur.
- Attention : enregistrer un monde sans ces mods chargés supprime leurs prefabs des layers.

### Liberate ZPR 1 et 2

- Les deux missions posent `TILW_BaseInteraction.et` (`{584EB34600E9698D}`), absent de TILW actuel et des autres addons installés : vérifier ces interactions et les remplacer.
