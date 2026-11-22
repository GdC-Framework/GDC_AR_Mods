# Synchronisation des mods du Workbench

Le launcher du jeu met à jour ses mods (`Documents/My Games/ArmaReforger/addons`), mais le Workbench garde ses propres copies (`Documents/My Games/ArmaReforgerWorkbench/addons`), mises à jour seulement à la main. Ce script recopie dans le Workbench les versions du jeu.

## Utilisation

Fermer le jeu et le Workbench, puis double-cliquer sur `sync_workbench_mods.bat`. Le script affiche le plan et demande confirmation avant de copier.

En ligne de commande :

```
python sync_workbench_mods.py --dry-run   # plan seul
python sync_workbench_mods.py --yes       # sans confirmation
```

## Règles

- Le jeu fait référence. Quand un mod est présent des deux côtés et que sa version est plus récente côté jeu, le dossier du Workbench est entièrement remplacé : il ne reste aucun ancien `.pak`.
- Une dépendance absente du Workbench (par exemple une nouvelle dépendance d'un mod mis à jour) est copiée aussi.
- Rien d'autre n'est copié ni supprimé : les mods présents seulement côté jeu et les projets locaux ne sont pas touchés.
- Ne sont signalés qu'à titre d'information : un mod plus récent côté Workbench, une copie de jeu incomplète (téléchargement en cours), une dépendance absente des deux côtés, un `.pak` obsolète resté dans le Workbench.

Un mod qui change d'ID de projet en se mettant à jour (par exemple WCS_JLTV, dont l'ID est passé de `JLTV` à `WCS_JLTV`) reste synchronisé, car la comparaison se fait sur le GUID Workshop. En revanche, les scripts ou les lignes de commande qui l'appellent par son ancien ID doivent être corrigés.

La copie passe par un dossier temporaire (`.sync_new`), puis l'ancien dossier est remplacé : un échec en cours de copie laisse l'ancienne version en place.

## Supervision des mods

`mod_supervision.py` encadre la synchro pour savoir ce qui a changé dans les mods dont dépend GDC et si rien n'a cassé. Claude s'en sert via le skill `gdc-mod-supervision` (`.claude/skills/`), qui publie ensuite un rapport pour les admins GDC. Il se lance avec l'interpréteur d'enfusion-mcp-py, qui fournit le lecteur de `.pak` :

```
C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe mod_supervision.py scope    # mods supervisés
C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe mod_supervision.py before   # AVANT la synchro
C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe mod_supervision.py after    # APRÈS la synchro
C:/Dev/Enfusion/enfusion-mcp-py/.venv/Scripts/python.exe mod_supervision.py save     # versions vues, une fois le rapport publié
```

- **Périmètre** : les projets GDC listés dans `supervision/config.json` et toutes leurs dépendances.
- **`before`** garde une copie des scripts, configs, prefabs et layouts des mods que la synchro va remplacer, dans `Documents/My Games/GDC_ModSupervision/snapshots`. Sans elle, on ne peut plus comparer : la synchro remplace tout le dossier.
- **`after`** compare avec les versions de la dernière supervision (`supervision/state.json`) et écrit `report.json` : fichiers et classes changés, fichiers GDC qui modifient ou héritent d'une classe changée, ressources référencées par GDC et supprimées par le mod, `.pak` obsolètes, versions de dépendances insuffisantes, mods absents de la liste de `Tools/MissionTests`.
- **`keep <dossier>`** garde comme copie de comparaison un mod téléchargé ailleurs, par exemple une ancienne version récupérée sur le Workshop, qui en garde les 50 dernières. `after` indique aussi, pour chaque mod mis à jour, les classes et ressources dont GDC dépend chez lui : c'est ce qui décide s'il vaut la peine de télécharger l'ancienne version.
- **`conflicts`**, à lancer après le test `-Compile` de MissionTests, dont le journal donne l'ordre de chargement réel, cherche les conflits entre mods du périmètre, addons GDC compris. Il en relève deux sortes, et dit chaque fois quel mod l'emporte :
  - une même fonction modifiée par plusieurs mods, dont un n'appelle pas la version précédente : celles chargées avant lui ne s'exécutent jamais ;
  - un même fichier modifié par plusieurs mods : propriété réglée différemment dans un prefab ou une config, ou fichier remplacé en bloc (matériau, particules, animation).

  Les mods d'une même famille (ACE, RHS, WCS, PSCore et Reforger Lobby, GDC : `families` dans `supervision/config.json`) se coordonnent entre eux : leurs conflits internes ne sont pas signalés. La détection de l'appel à la version précédente est textuelle : un appel sous condition compte comme un appel.
- **`candidate <dossier>`** : supervision préventive d'un mod qui n'est pas encore dans le modset (dossier téléchargé, ses dépendances à côté). Elle liste ses dépendances (déjà dans le modset, nouvelles, manquantes), ce qu'il modifie dans le jeu (scripts, classes modifiées, fichiers du jeu de base) et les conflits qu'il créerait avec le modset. Elle affiche ensuite la commande du test de compilation avec le candidat (`-ExtraAddonsDir`, `-ExtraAddons` de `run_mission_test.ps1`) ; relancée après ce test, elle dit qui l'emporte dans chaque conflit. Usage local, rien n'est publié.
- **`order [listes]`** vérifie des listes de mods de serveur par rapport à la règle d'ordre de chargement (`load_order` dans `supervision/config.json`). Sans argument, elle vérifie les listes `_ServerConfigs` du dépôt ; avec `--apply`, elle les réécrit d'abord selon la règle. La règle :
  1. le terrain ;
  2. les mods propres à la mission ;
  3. GDC Core, GDC AR Lib, GDC Equipment ;
  4. le pont GDC RL-CVON (Reforger Lobby – Coalition VON) ;
  5. le mod de la mission en dernier.

  Coalition VON, Reforger Lobby et PSCore doivent se charger après ACE, et GDC Core doit garder ACE avant Coalition VON dans ses dépendances. `after` fait aussi ces vérifications. `conflicts` prend l'ordre de la liste globale du dépôt, rangée selon cette règle, ou celui d'une liste donnée avec `--server-config`.
