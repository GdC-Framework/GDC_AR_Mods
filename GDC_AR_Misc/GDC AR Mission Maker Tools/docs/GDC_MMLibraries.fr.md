# Bibliothèques d'entités MM (GDC Mission Maker Tools)

[English](GDC_MMLibraries.md) | Français

Le plugin **Update MM Libraries** range les entités utiles aux créateurs de mission dans le panneau **Prefab Library** du World Editor. Les bibliothèques sont construites à partir des addons que vous avez chargés :

- **MM Forces :** groupes, armes statiques, véhicules terrestres et aériens, par faction. Ils viennent des catalogues de faction que Bohemia et les addons de faction (RHS…) tiennent pour le Game Master. Les personnages seuls sont exclus, car seuls les groupes reçoivent une IA.
- **MM Markers :** marqueurs de carte pour le briefing (Reforger Lobby, Coalition Lobby, TILW, GDC) et zones tracées sur la carte (TILW, GDC AR Lib), par addon.
- **MM Systems :** waypoints vanilla, triggers et utilitaires TILW, modules de GDC AR Lib (IA, appui, téléportation).

Ce qui n'est pas listé reste disponible dans le Resource Browser.

## Créer les bibliothèques

1. Ouvrez votre projet en cochant GDC Mission Maker Tools dans « Open With Addons ».
2. Lancez **Plugins > GDC > Mission Maker > Update MM Libraries**.
3. Choisissez le dossier du nouvel addon local **GDC Mission Maker Libraries**. Par défaut, il se place à côté de vos autres projets Workbench. Cliquez sur Create.
4. Fermez le Workbench, puis rouvrez votre projet depuis le Launcher avec « Open With Addons » en cochant **GDC Mission Maker Libraries**. Il entraîne GDC Mission Maker Tools avec lui.

Si vous avez choisi un autre dossier que celui par défaut, ajoutez-le d'abord au Launcher avec « Add Existing Project ».

GDC Mission Maker Libraries n'appartient qu'à vous : ne le publiez jamais, et ne l'ajoutez jamais en dépendance.

## Les utiliser

Faites glisser un prefab du panneau Prefab Library dans la vue. C'est le **prefab d'origine** qui est posé, pas le raccourci de la bibliothèque : la mission ne dépend donc ni des bibliothèques ni de cet addon.

Si l'addon du prefab n'est pas chargé, rien n'est posé et une boîte de dialogue indique l'addon à charger.

## Les mettre à jour

Relancez **Update MM Libraries** après avoir changé vos addons chargés. Les bibliothèques en deviennent le reflet : les nouveaux prefabs sont ajoutés, ceux des addons qui ne sont plus chargés sont retirés.

## Tests

Tests automatisés (mainteneurs) : [Tools/MMLibrariesTests](../../../Tools/MMLibrariesTests/README.md).
