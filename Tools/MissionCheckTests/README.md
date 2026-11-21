# Tests de Check Mission

Tests automatisés du plugin *Check Mission* de GDC Mission Maker Tools, et de l'analyse de loadout qu'il partage avec l'éditeur de loadouts. Ils lancent le Workbench sans interface et peuvent tourner pendant qu'un autre Workbench est ouvert.

```
python run_tests.py                 # environ 1 minute
python run_tests.py --gdc-missions  # ajoute le monde skeleton de GDC Missions (vrais fichiers de mission)
python run_tests.py --keep-logs     # garde les logs du Workbench dans _work/results/
python make_fixtures.py             # régénère l'addon de test (run_tests.py le fait à chaque lancement)
```

Le code de sortie vaut 0 quand tout passe. Le résumé de chaque lancement est gardé dans `_work/results/`, qui n'est pas versionné.

## Ce qui est testé

| Étape | Contenu |
|---|---|
| Contrôles statiques | `check_kb.py` (fiches de la base de connaissance) ; fins de ligne LF ; textes français au vouvoiement ; textes français différents des anglais |
| Tests unitaires | plugin `GDC_MMCheckUnitTestsPlugin` : catalogue (chaque règle complète dans les deux langues, mêmes `%1 %2 %3`, numéros et titres uniques, règles « comportement d'addon » vérifiées et dotées d'une fiche) ; aides de texte (accents, majuscules, accords) ; résolution des ressources (règle 2.1) ; addons exclus (2.4, avec des listes simulées) ; analyse de loadout (arme principale, chargeurs de rechange, débordement, prefabs introuvables, poids, rangements absents) |
| `T_Clean` | monde complet et propre : aucun constat |
| `T_Faulty`, en anglais puis en français | un cas piégé par règle : chaque règle attendue sort le bon nombre de fois, avec la bonne gravité, sur la bonne entité ; le rapport est dans la bonne langue |
| `T_NoPlayable` | Reforger Lobby sans groupe jouable (3.1), sans aucune entité TILW : les règles TILW ne tournent pas |
| skeleton (`--gdc-missions`) | le monde de test de GDC Missions, avec ses défauts connus |

Chaque lancement vérifie aussi l'absence d'erreur de script dans le log, donc la compilation de MM Tools.

## Fonctionnement

- **L'addon de test** `GDC Check Mission Tests` (ce dossier, jamais publié) dépend de MM Tools et de GDC AR Lib, qui amène TILW, Reforger Lobby et Coalition VON. Il contient les mondes témoins (sous-scènes d'Arland, le terrain le plus rapide à charger), des prefabs de personnages piégés et le plugin de tests unitaires. Il est **généré** par `make_fixtures.py` : on modifie le générateur, pas les fichiers.
- **Check Mission en ligne de commande** : `-wbModule=WorldEditor -plugin=GDC_MMMissionCheckPlugin -run -checkWorld="<monde .ent>" [-checkLanguage=fr]`. Le plugin ouvre le monde, lance tous les blocs et écrit chaque constat sur une ligne `[GDC_CHECK_DATA] règle|gravité|où`, que `run_tests.py` compare aux résultats attendus (`FAULTY`, `NO_PLAYABLE`, `SKELETON`).
- **Ajouter une règle** : piéger son cas dans `T_Faulty` (`make_fixtures.py`) et ajouter la ligne attendue dans `FAULTY` ; si la règle dépend d'un calcul isolable, un test unitaire.

## Ce que les tests ont appris (comportements du moteur)

- `Workbench.GetResourceName(chemin)` d'un chemin sans fichier renvoie le chemin lui-même, sans GUID.
- `Resource.Load` d'une référence au GUID inconnu se rabat sur le chemin : ce n'est pas un test d'existence.
- À l'ouverture d'un monde, le Workbench répare un GUID inconnu dont le chemin existe : une règle « GUID inconnu » ne se déclencherait jamais, elle a été retirée (le monde T_Faulty garde le cas, qui ne doit rien donner).
- `SCR_ECharacterRank.INVALID` est la **dernière** valeur de l'énumération, pas la plus basse.
- Charger un addon introuvable (`-addons`) ouvre un dialogue modal « Missing Addon » : ne jamais faire charger aux tests un addon qui peut manquer.

## Recette manuelle

Reste à faire dans le Workbench, ce que la ligne de commande ne couvre pas : les fenêtres de Check Mission (réglages, résultat, boutons), le clic sur les liens de la Log Console, l'éditeur de loadouts (affichage, *Refresh Containers*, répartition, *Apply*).
