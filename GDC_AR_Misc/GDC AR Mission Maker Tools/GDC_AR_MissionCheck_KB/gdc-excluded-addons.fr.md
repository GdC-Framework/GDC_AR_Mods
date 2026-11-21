# Addon exclu chargé (GDC)

[English](gdc-excluded-addons.md) | Français

Règle 2.5 · erreur · convention GDC

## Ce qui se passe
Certains addons cassent les missions GDC. S'il est chargé dans le Workbench, la mission risque d'en dépendre, et le serveur et les joueurs le chargent alors aussi.

## Addons exclus
| Addon | Exclu | Pourquoi |
|---|---|---|
| Anarchy Map Markers & Drawings (69A510CE600D1126) | des missions Reforger Lobby | conflit avec Reforger Lobby : les deux gèrent l'entrée du menu rapide des marqueurs (`OnInputQuickMarkerMenu`) |

La liste est dans le catalogue des règles (`GDC_MMCheckCatalog.GetExcludedAddons`) : tenez les deux à jour ensemble.

## Que faire
Retirez l'addon des dépendances de l'addon de la mission, et de la liste des mods du serveur.

## Quand retirer un addon de la liste
Quand le conflit est corrigé, par l'addon ou par Reforger Lobby : vérifiez-le en jeu avant de le retirer.

## Historique
- 18/11/2026 : Anarchy Map Markers & Drawings déjà exclu des configs serveur des missions Reforger Lobby.
- 21/11/2026 : règle écrite.
