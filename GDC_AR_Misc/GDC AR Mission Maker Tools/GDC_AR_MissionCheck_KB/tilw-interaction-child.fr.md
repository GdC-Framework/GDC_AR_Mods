# Interaction TILW placée en enfant

[English](tilw-interaction-child.md) | Français

Règle 5.1 · avertissement · comportement d'addon

## Ce qui se passe
Une interaction TILW (`TILW_SetFlagInteraction`, sans doute aussi `TILW_TeleportInteraction`) placée en enfant d'un objet peut ne pas afficher le libellé de son action sur un serveur dédié. Dans le Workbench et en jeu local, tout paraît normal.

## Preuve
Constaté et confirmé sur le serveur dédié GDC lors d'une mission précédente. Aucun autre effet connu.

## Que faire
Placez l'interaction en entité racine à la position de l'objet, avec « Display UI At Reference Point » coché sur son UserActionContext. Exemple : les interactions « Fin de mission » d'Entre Deux (calque framework).

## Quand retirer la règle
Quand un test sur serveur dédié, avec un TILW plus récent, montre que le libellé fonctionne sur une interaction placée en enfant.

## Historique
- Avant le 07/10/2026 : bug constaté sur le serveur dédié.
- 20/11/2026 : règle écrite (TILW 1.0.128).
