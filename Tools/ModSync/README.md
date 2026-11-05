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
