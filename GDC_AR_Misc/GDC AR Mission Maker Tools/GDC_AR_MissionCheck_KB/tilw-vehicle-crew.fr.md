# Équipage de véhicule qui n'apparaît jamais (TILW)

[English](tilw-vehicle-crew.md) | Français

Règle 5.2 · erreur · comportement d'addon

## Ce qui se passe
Un véhicule doté d'un `TILW_VehicleCrewComponent` reste vide dans une mission Reforger Lobby : aucun équipage n'apparaît, et rien ne le signale dans le log.

## Cause
Depuis le commit 0f2e423 (20/10/2026, « remove RL dependency »), TILW se charge avant PSCore et Reforger Lobby. Son code lobby est dans des blocs `#ifdef PS_LOBBY`, or `PS_LOBBY` n'est défini que par Reforger Lobby, compilé après TILW : ces blocs ne sont pas compilés. La ligne « defines: » du log affiche quand même `PS_LOBBY`, ce qui trompe.

## Preuve
Diagnostic du 10/11/2026 : `TILW_Lobby.GetGameMode()` renvoie null sous `PS_GameModeCoop`. Versions : TILW 1.0.128, Reforger Lobby 1.8.23, PSCore 1.6.10.

## Aussi touchés
Même cause, tout aussi silencieux : tickets de faction, réapparition des joueurs morts, RemoveOpenSlots, visibilité des marqueurs par faction, écoute de faction de la limite d'AO, écoute d'état des spawners.

## Contournement
Un `TILW_PrefabSpawner` (Prefabs/Utils/TILW_PrefabSpawner.et) :
- m_prefab : le véhicule ;
- m_crewConfig : l'équipage (waypoints possibles) ;
- m_pregameSpawn coché ;
- m_setEntityNames si le véhicule doit garder son nom.

Testé sur Davay dans le Workbench. Limites : l'équipage est actif pendant le lobby ; régler une instance à part demande un prefab dérivé.

## Quand retirer la règle
Quand TILW compile de nouveau son code lobby avec Reforger Lobby (addon de pont ou dépendance rétablie) : regarder le dépôt de TILW à chaque mise à jour.

## Historique
- 10/11/2026 : cause confirmée, contournement testé (TILW 1.0.128).
