# Deleter TILW sans filtre

[English](tilw-entity-deleter.md) | Français

Règle 4.1 · avertissement · comportement d'addon

## Ce qui se passe
Un `PK_EntityDeleter` dont la liste de filtres de prefabs est vide supprime, quand il se déclenche, toutes les entités racines dans son rayon : les objets, mais aussi les managers, les entités du mode de jeu et les IA. Selon ce qui se trouve dans la sphère, la mission casse ou le serveur plante.

## Preuve
Plantage sur Davay (14/11/2026) causé par un deleter sans filtre. Il ne plante pas à chaque fois : tout dépend de ce qui se trouve dans la sphère.

## Que faire
Remplissez m_prefabFilterList avec les prefabs à supprimer. Un filtre vide ne convient que si la sphère ne contient que ce qui doit disparaître : ignorez alors l'avertissement.

## Quand retirer la règle
Si TILW change l'effet d'un filtre vide (par exemple, ne plus rien supprimer).

## Historique
- 14/11/2026 : plantage sur Davay.
- 20/11/2026 : règle écrite en avertissement (TILW 1.0.128).
