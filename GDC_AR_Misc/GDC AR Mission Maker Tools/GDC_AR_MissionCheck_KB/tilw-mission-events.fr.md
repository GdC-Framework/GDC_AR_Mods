# Événements de mission TILW mal réglés

[English](tilw-mission-events.md) | Français

Règles 5.6 à 5.14 · avertissement ou erreur · comportement d'addon

## Ce qui se passe
Les événements de mission de l'entité TILW Mission Framework exécutent leurs instructions quand leur condition devient vraie. TILW réévalue chaque événement à chaque changement de flag. Certains réglages rendent un événement muet, précoce ou inopérant, sans aucun message pour le mission maker :

| Règle | Réglage | Effet |
|---|---|---|
| 5.6 | événement sans condition, meta flag sans définition | jamais vrai ; une erreur de script (« NULL pointer to instance ») à chaque évaluation. Une instruction Run Mission Events lance quand même l'événement |
| 5.7 | opérateur (Conjunction, Disjunction...) sans opérande | son résultat ne change jamais : une Conjunction vide est vraie, l'événement se produit donc dès le début de la partie ; une Disjunction vide n'est jamais vraie |
| 5.8 | condition qui attend un flag que rien ne lève | l'événement (ou l'interaction, le spawner, l'artillerie IA GDC) ne se produit jamais. Non signalé pour un événement lancé par son nom |
| 5.9 | Run, Reactivate ou Deactivate Events qui ne nomme aucun événement | l'instruction ne fait rien |
| 5.10 | nom visé, porté par plusieurs événements | l'instruction agit sur tous |
| 5.11 | lieu, groupe, waypoint, entité, élément de carte, véhicule que rien ne porte | l'instruction (ou le spawner, l'équipage) ne fait rien pour ce nom |
| 5.12 | plusieurs entités Mission Framework | TILW en garde une ; les changements de flag ne réévaluent plus les événements des autres |
| 5.13 | événement autorisé avant la fin du briefing (`m_pregameEvent`) qui lit des flags non aléatoires | ces flags ne sont en général pas encore levés |
| 5.14 | `m_suppressGameEnd` coché alors qu'un événement termine la partie | End Game affiche un message au lieu de terminer la partie |

Un flag compte comme levé quand un trigger, une interaction, une instruction Set Flag ou de flag aléatoire, un meta flag, un flag de pertes ou aléatoire, ou un composant de flag de dégâts ou de mode de contrôle le lève, dans la mission ou dans un prefab qu'elle fait apparaître (spawner, instruction, membre de groupe, équipage).

## Preuve
Lu dans les scripts de TILW (`TILW_MissionFrameworkEntity`, `TILW_Instructions`, `TILW_Terms`), version 1.0.128. Comportement du moteur mesuré par les tests automatiques (`Tools/MissionCheckTests`, sonde moteur) : une méthode appelée sur nul écrit l'erreur et le script continue, le test vaut alors faux ; un opérateur sans opérande donne le résultat ci-dessus.

## Que faire
Suivez la correction indiquée dans le rapport. Un flag levé seulement par un script de mission est invisible pour l'outil : ignorez alors la 5.8.

## Quand retirer les règles
Si TILW change ses événements, termes ou instructions : regarder ses scripts à chaque mise à jour. Si le moteur arrête un script sur un appel nul, la 5.6 s'aggrave (les événements suivants ne sont plus évalués) : le test de la sonde moteur le dira.

## Historique
- 21/11/2026 : règles écrites (TILW 1.0.128).
