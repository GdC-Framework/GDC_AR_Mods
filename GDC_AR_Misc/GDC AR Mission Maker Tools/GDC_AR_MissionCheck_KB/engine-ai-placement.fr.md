# IA ou waypoint au-dessus du vide

[English](engine-ai-placement.md) | Français

Règle 6.5 · avertissement · comportement du moteur

## Ce qui se passe
Un groupe IA, un personnage IA ou un waypoint est posé sans rien (sol ou objet) juste en dessous : un étage trop haut, à côté d'un balcon, en l'air ou sous le terrain. L'IA tombe alors, apparaît dans un mur, ou n'atteint jamais le waypoint (« Failed move » dans le log).

## Preuve
Check Mission cherche une surface de 0,5 m au-dessus de l'entité jusqu'à 1,5 m en dessous, sur le terrain et les objets (bâtiments compris). Mesuré par la sonde moteur des tests automatiques (`Tools/MissionCheckTests`) sur Arland, Gulfcoast et Zimnitrita : le tracé trouve le sol et les toits.

## Ce que le contrôle ne voit pas
Si l'IA peut marcher jusqu'à un waypoint. Dans le World Editor, le navmesh répond sur Arland mais pas sur les terrains dont le navmesh est découpé en tuiles : sur Zimnitrita (Davay), où les IA se déplacent bien en jeu, aucun point du navmesh fantassins n'est trouvé. Un waypoint sur un terre-plein sans accès, ou un trajet coupé par un canal, ne se voit donc qu'en Play : les tests de mission (`Tools/MissionTests`) suivent les groupes et signalent ceux qui s'arrêtent.

## Que faire
Descendez l'entité sur le plancher ou sur le sol. Pour un tireur à une fenêtre, vérifiez l'étage dans le World Editor avant d'enregistrer.

## Quand retirer la règle
Si le World Editor donne un navmesh fiable sur tous les terrains : la règle pourrait alors vérifier le navmesh lui-même (le test de la sonde moteur le dira).

## Historique
- 21/11/2026 : règle écrite ; contrôles de navmesh abandonnés après les sondes sur Arland, Gulfcoast et Zimnitrita.
