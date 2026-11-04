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

## GDC AR Mission Maker Tools

### Bibliothèques MM (« Update MM Libraries »)

- Dérouler la recette manuelle listée dans [Tools/MMLibrariesTests/README.md](Tools/MMLibrariesTests/README.md) (dialogues, Launcher, panneau Prefab Library, drop réel). La partie automatisable passe avec `python run_tests.py`.
- Proposer un nom à la pose d'un marqueur ou d'une zone servant de repère à un module.
- Évaluer une bibliothèque dédiée au Scenario Framework.
- Signaler à Bacon que l'addon ZU-23 ne déclare `BaconZU23_Turret_Deployed` et `Ural4320_ZU23` dans aucun catalogue de faction, seulement dans un ancien registre Game Master.

## Coalition VON : fréquence de départ des radios de véhicules

L'évolution demandée dans l'[issue #42](https://github.com/CoalitionArma/Coalition-VON/issues/42) est annoncée par l'équipe CVON pour une version postérieure à la 3.0.3. D'après son commentaire, les radios de véhicules démarrent sur la première fréquence longue portée de la configuration et y reviennent à chaque key fill. Trois réglages sont annoncés : `m_sVehicleFrequency` sur `CVON_FreqConfig` (mission) ou sur les infos d'indicatif d'une faction, et `m_sStartFrequency` sur `CVON_VehicleRadioComponent` (véhicule).

La copie locale de CVON est en 3.0.3 et ne contient pas encore ces attributs. Une fois CVON mis à jour :

- Vérifier le comportement et le nom des attributs.
- Murabat : retirer le contournement de `Configs/Murabat_Frequencies.conf` (48000 placé en tête de `m_aSRFrequencies`) et remettre 31000 en premier.

## Missions

### Davay, duy na Gruwice

- Remplacer les `TILW_TeleportInteraction` des drapeaux par le module `GDC_Teleport`, avec le déclenchement par le CdO.
- Régler la durée du noir sur Zimnitrita, pour le trajet d'environ 7 km depuis la base.
- Construire la garnison ennemie, les positions renforcées, les véhicules et les occupants de l'hôtel.
- Construire l'arrivée de renforts par le port.
- Ajouter l'objet de renseignement et son interaction dans l'hôtel.
- Vérifier le nombre de slots pour 10 à 15 joueurs : un seul groupe jouable (`RifleSquad`) pour l'instant.
- Vérifier que les IA utilisent les passerelles et échelles ajoutées, probablement absentes du navmesh de Zimnitrita.

### Liberate ZPR 1 et 2

- Les deux missions posent `TILW_BaseInteraction.et` (`{584EB34600E9698D}`), absent de TILW actuel et des autres addons installés : vérifier ces interactions et les remplacer.
