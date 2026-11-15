# Rapport de log serveur

Transforme le `console.log` d'un serveur dédié Arma Reforger en une page HTML autonome : FPS du serveur dans le temps, joueurs et IA présents, mods chargés. La page s'ouvre dans n'importe quel navigateur, hors ligne, et peut s'envoyer telle quelle.

## Utilisation

Glisser le `console.log` (ou son dossier `logs_AAAA-MM-JJ_...`) sur `server_log_report.bat`. Le rapport est écrit à côté du log, sous le même nom en `.html`, puis ouvert.

En ligne de commande :

```
python server_log_report.py "console.log"                    # écrit console.html à côté
python server_log_report.py logs_2026-11-08_19-42-47 --open  # dossier de logs, ouvre le rapport
python server_log_report.py "console.log" --out rapport.html
```

Python 3.10 ou plus, sans paquet à installer.

## Contenu du rapport

- **En-tête** : mission, date, heures de début et de fin, version du jeu, fichier de config du serveur.
- **Chiffres clés** : FPS moyen et minimum pendant la partie (à partir du passage en phase `GAME`, ou sur toute la session si la partie n'a pas commencé), nombre maximal de personnages IA et de joueurs, gel le plus long.
- **FPS du serveur** : un point toutes les 5 s (lignes `FPS:` du log, écrites grâce à `-logStats 5000`). La zone grisée précède le début de la partie, et des pointillés marquent chaque changement de phase (slots, briefing, partie). Le survol affiche l'heure, les FPS, les joueurs, les IA et les gels.
- **Joueurs et IA présents**, sur la même échelle de temps, avec la liste des joueurs vus.
- **Morts de joueurs** : repérées en rouge sur le graphique des joueurs, puis un récapitulatif par joueur (morts, tué par l'IA, tir ami subi ou commis, suicides, joueurs ennemis tués) et une chronologie : heure, victime, type (ennemi, tir ami, Game Master, suicide), tireur, distance, dernier dégât et zone touchée, positions de la victime et du tireur, remarques.
- **Mods chargés** : dans l'ordre de chargement, avec leur ID de projet, leur GUID et un lien vers leur page Workshop. Un champ permet de filtrer la liste.

## Limites

- Les mods viennent du dernier bloc `Loaded addons` du log : ce sont ceux que le serveur a réellement chargés, pas tous ceux présents dans son dossier. Le log ne donne pas leurs versions.
- Sans `-logStats` dans la ligne de commande du serveur, le log ne contient aucun relevé de FPS et les graphiques restent vides.
- Les morts viennent des lignes `KILL` que le jeu de base écrit sur le serveur (`SCR_BaseGameMode.LogKillEvent`), seulement quand un joueur meurt : les morts d'IA n'y sont pas. Un tireur IA n'est connu que par sa faction et sa position, et l'arme n'est jamais donnée.
- Le « dernier dégât » est le dernier dégât infligé par le tireur. Un bug du jeu de base (1.9) laisse passer les soins : une cause « soin » ou « régénération » est donc fausse, mais le tireur reste juste. Avec ACE, une mort par hypoxie est une mort différée (saignement, arrêt cardiaque) : le tireur est celui qui a blessé. Le rapport signale ces cas dans la colonne Remarques.
- Le nom de la mission est celui de la liste des scénarios (`-listScenarios`). À défaut, c'est le nom du fichier `.conf`.
