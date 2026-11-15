# Tests automatisés des missions

Joue une mission sans intervention sur un serveur dédié local (exécutable Diag) avec 2 clients, à partir des sources du dépôt, puis donne un bilan `PASS` / `FAIL` / `WARN` et les erreurs des 3 logs, regroupées.

## Lancer

Le Workbench doit être fermé.

```powershell
powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 -Mission Davay
```

- Deux fenêtres de jeu s'ouvrent : n'y touche pas, le serveur pilote tout.
- `-Compile` : compile les scripts du jeu de tout le modset et s'arrête **avant le chargement du monde**, en 3 s. Résultat « COMPILED » ou « COMPILATION FAILED » avec les erreurs (fichier et ligne). Le serveur ne compile pas le module `WorkbenchGame` (plugins Workbench) : pour lui, il faut le Workbench.
- `-Smoke` : serveur seul sur un monde **Arland** (`_Tests/GDC_Teleport_Arland.ent`), le terrain le plus rapide à charger : compilation, chargement du monde et démarrage d'une partie, sans scénario (environ 30 s). Attrape en plus les prefabs ou configs illisibles et les erreurs à l'initialisation du mode de jeu. Résultat « COMPILED AND STARTED » ou la cause de l'échec.
- `-NoClients` : serveur seul sur le monde de la mission, pour vérifier que son scénario démarre. Il s'arrête alors sur « Two players connected ».
- `-TimeoutMinutes` : 60 par défaut.

Les logs sont dans `Documents/My Games/GDCAutoServer`, `GDCAutoClient1` et `GDCAutoClient2`.

## Fonctionnement

- L'addon `GDC_MissionTests` (ID `GDC Mission Tests`) n'est chargé que par ce script. **Il n'est jamais publié** et ne modifie ni les missions ni GDC AR Lib.
- Le serveur est lancé avec `-gdcTest <Mission>`. Le mode de jeu crée alors l'entité `GDC_MissionTestRunnerEntity`, qui déroule le scénario de la mission et mesure les FPS du serveur.
- Le serveur fait tout lui-même :
  - il attribue les slots du Reforger Lobby aux deux clients et fait avancer le lobby jusqu'à la partie ;
  - il rend les joueurs de test invulnérables ;
  - il les déplace avec la séquence de téléportation de GDC AR Lib ;
  - il appelle directement les modules (téléportation, appui feu) au nom des joueurs.

## Ajouter une mission

1. Écrire `Scripts/Game/GDC/MissionTests/Missions/GDC_MissionTest_<Mission>.c`, une classe dérivée de `GDC_MissionTest` qui implémente `RunStep()`. Les outils communs sont dans `GDC_MissionTest.c` : recensement des IA, inventaires, slots du lobby, drapeaux TILW, radios CVON…
2. Ajouter une ligne dans `GDC_CreateMissionTest()` (`GDC_SCR_BaseGameMode_MissionTest.c`).
3. Ajouter le monde dans `$worlds` de `run_mission_test.ps1`. Si la mission utilise d'autres mods, compléter `$addons`.

## Davay, duy na Gruwice (~40 min)

Le client 1 prend le PL de Granit, le client 2 le SL de Berkut. L'insertion se fait sur l'axe Bleu.

- **Lobby :**
  - 15 slots, rôles des 3 groupes ;
  - grades (PL CAPTAIN, SL LIEUTENANT) ;
  - R-148 pour tous, R-168 pour le PL, le RTO et les SL ;
  - Orion et chargeurs du PL, caisses PK de l'AMG, sac et kit du Medic ;
  - fréquences prévues par slot.
- **Mise en place :**
  - entités nommées présentes, aucun drapeau levé ;
  - 78 IA, limite d'IA à 256 ;
  - tourelles des véhicules armées ;
  - pas d'opposition sur les axes, intel dans l'hôtel ;
  - un module d'appui feu avec 5 missions.
- **Partie :**
  - fréquences réellement réglées sur les radios des 2 joueurs ;
  - insertion refusée au SL, acceptée au PL, et le SL est emmené ;
  - drapeau `Arrivee_Bleu` et 16 IA sur l'axe ouest, rien sur les autres axes ;
  - emplacement M2HB de PR3 armé.
- **Appui feu :**
  - accès selon la R-168 ;
  - refus sur la place Lénine ;
  - une mission consommée, refus tant que les mortiers tirent.
- **Renforts :**
  - le PL va à l'hôtel, `GO_REINF` doit se lever environ 300 s plus tard ;
  - les 3 groupes de « Bloc » vont à leurs points ;
  - 4 vagues de 3 QRF à l'heure prévue, plus rien après 1100 s et `GO_REINF` effacé ;
  - pour chaque groupe : arrivée à son point, ou position où il s'est bloqué ;
  - nombre maximal d'IA, FPS du serveur pendant la phase.
- **Comportement :**
  - les patrouilles bougent ;
  - les défenseurs tiennent leur position (avertissement au-delà de 50 m) ;
  - les tireurs aux fenêtres ne descendent pas (avertissement au-delà de 2,5 m).

Ce test ne couvre pas, et reste donc à faire à la main :
- l'audio des radios ;
- le visuel : briefing, marqueurs, interface de l'appui feu, fondu ;
- le jeu dans les bâtiments ;
- la difficulté ;
- les outils GM ;
- la version publiée sur un vrai serveur.
