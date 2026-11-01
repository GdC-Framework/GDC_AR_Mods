# GDC AR Lib

[English](README.md) | Français

Bibliothèque de fonctions pour les missions de la communauté Grèce de canards, destinée aux créateurs de missions.

Pour l'utiliser, il suffit de mettre GDC AR Lib en dépendance de ton addon, ou de le cocher dans « Open With Addons » au lancement de Workbench.

Elle dépend de TILW, PSCore, Reforger Lobby et Coalition VON, voir le [backlog](../../BACKLOG.md).

## Nouvelles fonctionnalités

### Téléportation avec ellipse temporelle

Prefab : `Prefabs/Teleport/GDC_Teleport.et`, à poser en entité racine à côté de l'objet servant de point de départ (par exemple un drapeau), jamais en enfant de celui-ci.

- **Destination :** le nom d'une entité du monde. Les joueurs arrivent répartis autour d'elle.
- **Mode `INDIVIDUAL` :** chaque joueur utilise l'action et part seul.
- **Mode `GROUP` :** seul le chef de groupe voit l'action. Il emmène les joueurs de son groupe présents dans un rayon réglable autour du module. Les IA, les joueurs morts ou en véhicule ne partent pas.
- **Ellipse :** durées du fondu, de l'écran noir et du retour de l'image. Si les trois sont à 0, la téléportation est immédiate.
- **Texte :** optionnel, affiché sur un léger voile, par exemple « Deux heures plus tard... ».
- **Image :** optionnelle, centrée sur le noir, limitée à une part de l'écran (60 % par défaut) et jamais agrandie au-delà de sa résolution native.
- **Son :** les sons du jeu suivent le fondu, ce qui se désactive. La voix et la radio ne sont pas coupées.

### Appui feu virtuel au mortier

Prefab : `Prefabs/Support/GDC_FireSupport_Mortar.et`. Les obus sont simulés, sans pièce ni servant. La position du module tient lieu de position des tubes pour le temps de vol et la portée.

Les joueurs demandent le tir depuis un panneau ouvert par le menu radial de la carte. Ils y choisissent la munition, le nombre d'obus et la dispersion, dans les limites fixées par le module.

- **Désignation de la cible :** `ARCADE` par clic sur la carte, avec marqueur et zone d'impact affichés, ou `MILSIM` par coordonnées saisies à la main, sans rien sur la carte.
- **Accès :** grade minimum et/ou objet à porter. Si les deux sont définis, les deux sont exigés. Sans eux, le joueur ne voit pas l'appui.
- **Limites :** nombre de missions, délai entre deux missions, zones de tir autorisées ou interdites.
- **Portée :** les anneaux de portée de chaque munition sont visibles dans le World Editor. Un symbole de mortier peut aussi être affiché sur la carte du jeu.

### Zones affichées sur la carte

Prefab : `Prefabs/Markers/GDC_Marker_Area.et`, une polyligne affichable sur la carte du jeu avec un contour et, si elle est fermée, un remplissage. Couleurs, épaisseur et opacité sont réglables. Elle sert aussi à définir les zones de tir de l'appui au mortier.

### Radios par slot

Fréquences des radios courte et longue portée, définies par slot sur les groupes jouables (`SCR_AIGroup`), avec Coalition VON. Un slot peut avoir plusieurs radios. Une fréquence longue portée commune peut s'appliquer aux membres du groupe sans fréquence propre.

## Autres fonctionnalités

- **GDC Loadout Editor :** outil du World Editor pour modifier le loadout des prefabs de personnages, voir [docs/GDC_LoadoutEditor.md](docs/GDC_LoadoutEditor.md).
- **Artillerie IA :** `Prefabs/AI/Artillery/GDC_ArtilleryFireSupport.et`, tirs de mortier servi par des IA, déclenchés par un flag de mission TILW.
- **`GDC_FollowNearestPlayerEntity` :** place une entité sur le joueur le plus proche d'une référence, à intervalle réglable.
- **`GDC_ForceGroupActiveLODComponent` :** garde un groupe d'IA en simulation complète, quelle que soit la distance aux joueurs.
- **GDC Import Loading Screens :** plugin du Resource Manager qui importe les écrans de chargement préparés dans `_Media` vers GDC AR Core.
