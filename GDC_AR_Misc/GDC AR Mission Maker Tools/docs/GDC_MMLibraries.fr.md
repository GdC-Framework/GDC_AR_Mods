# Bibliothèques d'entités MM (GDC Mission Maker Tools)

[English](GDC_MMLibraries.md) | Français

Le plugin **Update MM Libraries** range les entités utiles aux créateurs de mission dans le panneau **Prefab Library** du World Editor. Les bibliothèques sont construites à partir des addons que vous avez chargés :

- **MM Forces :** groupes, armes statiques, véhicules terrestres et aériens, par faction. Ils viennent des catalogues de faction que Bohemia et les addons de faction (RHS…) tiennent pour le Game Master.
- **MM Markers :** marqueurs de carte pour le briefing (Reforger Lobby, Coalition Lobby, TILW, GDC) et zones tracées sur la carte (TILW, GDC AR Lib), par addon.
- **MM Systems :** waypoints vanilla, triggers et utilitaires TILW, modules de GDC AR Lib (IA, appui, téléportation).

Ce qui n'est pas listé reste disponible dans le Resource Browser.

## Premier lancement : créer les bibliothèques

1. Ouvrez votre projet en cochant GDC Mission Maker Tools dans « Open With Addons ».
2. Lancez **Plugins > GDC > Mission Maker > Update MM Libraries**.
3. Choisissez le dossier du nouvel addon local **GDC Mission Maker Libraries**. Par défaut, il se place à côté de vos autres projets Workbench. Cliquez sur Create.
4. Fermez le Workbench, puis rouvrez votre projet depuis le Launcher avec « Open With Addons » en cochant **GDC Mission Maker Libraries** (plus besoin de cocher GDC Mission Maker Tools, votre addon local de bibliothèque l'entraîne avec lui).

Si vous avez choisi un autre dossier que celui par défaut, ajoutez-le d'abord au Launcher avec « Add Existing Project ».

<details>
<summary>Mais pourquoi je dois faire ça, bordel de noeud de chiotte ?</summary>

GDC Mission Maker Tools est un mod que vous téléchargez. Il est donc en lecture seule, comme tous les autres addons que vous chargez.

Par contre, votre bibliothèque est locale et varie en fonction de votre modset : on est donc obligé de créer un petit mod local, rien qu'à vous, qui stocke les données de votre bibliothèque.

</details>

⚠️ **N'ajoutez jamais GDC Mission Maker <ins>Libraries</ins> en dépendance. Votre mod de bibliothèque étant local et jamais publié, aucun autre joueur ne pourrait jouer la mission ! Donc, on fait gentiment un clic droit dans le launcher du workbench pour l'ajouter avec "Open with addons".**

## Utiliser la bibliothèque

1. Sélectionnez le panneau Prefab Library.
2. Choisissez une des bibliothèques MM (Forces, Markers, etc.)
3. Glissez et déposez dans la vue 3D

PS : Si l'addon du prefab n'est pas chargé, rien n'est posé et une boîte de dialogue indique l'addon à charger.

## J'ai chargé d'autres addons ou supprimé certains addons de ma mission : mettre à jour la bibliothèque

Relancez **Update MM Libraries** : les nouveaux prefabs sont ajoutés, ceux des addons qui ne sont plus chargés sont retirés. Aucun risque, vous pouvez faire cela autant de fois que vous voulez.

## Tests pour les mainteneurs de l'addon (AI food, arf !)

Tests automatisés (mainteneurs) : [Tools/MMLibrariesTests](../../../Tools/MMLibrariesTests/README.md).
