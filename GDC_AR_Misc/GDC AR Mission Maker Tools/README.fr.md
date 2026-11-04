# GDC AR Mission Maker Tools

[English](README.md) | Français

Outils Workbench pour les mission makers de la communauté Grèce de canards.

N'ajoute jamais cet addon en dépendance d'une mission : charge-le seulement pour éditer, en le cochant dans « Open With Addons » au lancement du Workbench. Les missions construites avec ne dépendent pas de lui, et les joueurs ne le téléchargent jamais.

## Bibliothèques MM

Des raccourcis triés pour les mission makers dans le panneau **Prefab Library** du World Editor, construits à partir des addons que tu as chargés :

- **MM Forces :** groupes, armes statiques, véhicules terrestres et aériens, par faction. Ils viennent des catalogues de faction que Bohemia et les addons de faction (RHS…) tiennent pour le Game Master. Les personnages seuls sont exclus, car seuls les groupes reçoivent une IA.
- **MM Markers :** marqueurs de carte pour le briefing (Reforger Lobby, Coalition Lobby, TILW, GDC) et zones tracées sur la carte (TILW, GDC AR Lib), par addon.
- **MM Systems :** waypoints vanilla, triggers et utilitaires TILW, modules de GDC AR Lib (IA, appui, téléportation).

Ce qui n'est pas listé reste disponible dans le Resource Browser.

### Créer les bibliothèques

1. Ouvre ton projet en cochant GDC AR Mission Maker Tools dans « Open With Addons ».
2. Lance **Plugins > GDC > Mission Maker > Update MM Libraries**.
3. Choisis le dossier du nouvel addon local **GDC AR MM Libraries**. Par défaut, il se place à côté de tes autres projets Workbench. Clique sur Create.
4. Ferme le Workbench, puis rouvre ton projet depuis le Launcher avec « Open With Addons » en cochant **GDC AR MM Libraries**. Il entraîne GDC AR Mission Maker Tools avec lui.

Si tu as choisi un autre dossier que celui par défaut, ajoute-le d'abord au Launcher avec « Add Existing Project ».

GDC AR MM Libraries n'appartient qu'à toi : ne le publie jamais, et ne l'ajoute jamais en dépendance.

### Les utiliser

Fais glisser un prefab du panneau Prefab Library dans la vue. C'est le **prefab d'origine** qui est posé, pas le raccourci de la bibliothèque : la mission ne dépend donc ni des bibliothèques ni de cet addon.

Si l'addon du prefab n'est pas chargé, rien n'est posé et une boîte de dialogue indique l'addon à charger.

### Les mettre à jour

Relance **Update MM Libraries** après avoir changé tes addons chargés. Les bibliothèques en deviennent le reflet : les nouveaux prefabs sont ajoutés, ceux des addons qui ne sont plus chargés sont retirés.

## Tests

Tests automatisés : [Tools/MMLibrariesTests](../../Tools/MMLibrariesTests/README.md). Ils lancent le plugin en ligne de commande :

```
-wbModule=ResourceManager -plugin=GDC_MMCatalogGeneratorPlugin -run [-mmLibraryFolder="<dossier>"]
```
