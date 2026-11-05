# Tests des bibliothèques MM

Tests automatisés du plugin « Update MM Libraries » de GDC Mission Maker Tools. Ils tournent sans ouvrir le Workbench à la main et peuvent s'exécuter pendant qu'un autre Workbench est ouvert.

```
python run_tests.py          # scénario complet, environ 2 à 3 minutes
python run_tests.py --full   # ajoute une mise à jour avec tout le modset GDC (durée)
python run_tests.py --authorize-scripts   # sans le dialogue « Script Authorization Required »
python check_library.py "<dossier d'un addon GDC Mission Maker Libraries>"   # contrôle des fichiers seul
```

Sans `--authorize-scripts`, chaque lancement du Workbench affiche le dialogue « Script Authorization Required » (appels `FileIO` du plugin) : il faut cliquer « Yes to All », sinon l'étape reste bloquée. L'option passe `-scriptAuthorizeAll` au Workbench, qui accorde alors ces autorisations à tous les scripts chargés, y compris ceux des addons installés : à décider à chaque fois.

Le code de sortie vaut 0 quand toutes les étapes passent. Le résumé, les logs du Workbench et les rapports sont gardés dans `_work/results/`, qui n'est pas versionné.

## Fonctionnement

`run_tests.py` lance le Workbench en ligne de commande :

```
ArmaReforgerWorkbenchSteamDiag.exe -gproj "<MM Tools>/addon.gproj" -addonsDir "<dossiers>" -addons "<IDs>"
  -profile GDCMMTest -wbModule=ResourceManager -plugin=<classe> -run [paramètres du plugin]
```

- `-addonsDir` liste les dossiers où chercher les addons, `-addons` les ID des addons à charger (leurs dépendances suivent). Le dossier des addons vanilla doit figurer dans la liste.
- `-plugin` exécute la méthode `RunCommandline()` du plugin, puis le plugin ferme le Workbench avec son code de sortie.
- Le profil `GDCMMTest` sépare les logs : `Documents/My Games/GDCMMTest/logs/`.

La bibliothèque de test est créée dans `_work/GDC Mission Maker Libraries`. Quand deux addons portent le même ID, le Workbench charge celui du premier dossier de `-addonsDir` : `_work` passe donc avant `Documents/My Games/ArmaReforgerWorkbench/addons`, où se trouve d'habitude la bibliothèque du mission maker. Le script refuse de démarrer si une autre bibliothèque passerait avant, et chaque étape vérifie dans le log que la bibliothèque chargée est bien celle de test.

Deux plugins servent aux tests :

- `GDC_MMCatalogGeneratorPlugin` (l'outil lui-même) : sans boîte de dialogue en ligne de commande. Il met à jour la bibliothèque chargée, ou la crée dans `-mmLibraryFolder` si elle n'est pas chargée, et écrit `command line result: OK|FAILED` dans le log.
- `GDC_MMLibrariesCheckPlugin`, dans l'addon local `GDC MM Tools Tests` (ce dossier, jamais publié) : pour chaque prefab de bibliothèque que le Workbench a enregistré, il écrit dans `-mmTestReport` le parent lu comme au drop, l'interception par le plugin de drop et l'addon trouvé dans l'index.

## Ce qui est testé

| Étape | Addons chargés | Vérifications |
|---|---|---|
| 1. Création | TILW, GDC AR Lib (et leurs dépendances) | Code 0, uniquement des prefabs nouveaux ; contrôle des fichiers ; MM Forces, MM Markers et MM Systems non vides |
| 2. Enregistrement | idem + bibliothèque | Tous les prefabs sur disque sont enregistrés (sinon : panneau d'interdiction au drop), parents lisibles et identiques aux fichiers, interceptés, index lu comme dans le fichier |
| 3. Mise à jour à l'identique | idem | Tout est conservé ; GUID et chemins inchangés |
| 4. Ajout de RHS | + RHS | Nouveaux prefabs, aucun retiré ; anciens GUID inchangés ; prefabs RHS dans l'index |
| 5. Enregistrement avec RHS | idem | Comme l'étape 2 : les fichiers ajoutés bibliothèque chargée sont enregistrés aussitôt |
| 6. Ouverture sans RHS | sans RHS | Parents RHS illisibles ; l'index nomme RHS pour chacun (message au drop) ; les autres prefabs restent lisibles |
| 7. Mise à jour sans RHS | sans RHS | Seuls les prefabs RHS sont retirés ; la bibliothèque redevient identique à celle de l'étape 1 |
| 8. Création refusée | sans bibliothèque | Code 1 et message d'erreur quand le dossier contient déjà un addon ; bibliothèque intacte |
| 9. Modset complet (`--full`) | modset GDC : TILW, GDC AR Lib, GDC Core et GDC Equipment, qui chargent tous les autres mods comme dépendances | Code 0, contrôle des fichiers, durée |

Chaque étape vérifie aussi l'absence d'erreur de script dans le log du Workbench, donc la compilation de l'addon.

Le contrôle des fichiers (`check_library.py`) vérifie : `addon.gproj` (ID, titre, GUID, dépendance à MM Tools) ; arborescence `PrefabLibrary/MM .../<catégorie>/<faction ou addon>/` ; chaque `.et` est un prefab enfant vide dont le nom reprend celui du parent ; un `.meta` par fichier, au bon chemin, GUID uniques ; pas de dossier vide ni de `.meta` orphelin ; pas deux fois le même parent dans un dossier ; index trié, sans doublon, sans prefab vanilla, cohérent avec les dossiers par addon de MM Markers et MM Systems.

## Recette manuelle

Reste à faire dans le Workbench, ce que la ligne de commande ne couvre pas :

- Premier lancement depuis le menu : boîte de dialogue, dossier par défaut, message de fin ; dossier hors défaut avec « Add Existing Project ».
- Ouverture par le Launcher avec « Open with Addons » en cochant seulement GDC Mission Maker Libraries (MM Tools doit suivre).
- Panneau Prefab Library : les trois bibliothèques et leur ordre.
- Drop réel dans la vue : entité posée = prefab d'origine, orientation, sélection ; marqueurs (rotation des marqueurs PS) et zones (polyligne vide).
- Drop d'un prefab dont l'addon manque : texte de la boîte de dialogue.
- Drop d'un mélange de prefabs de bibliothèque et d'autres fichiers : comportement natif.
- Marqueurs Coalition Lobby (`COA_ManualMarker`) : l'addon n'est pas installé ici, non testé.
