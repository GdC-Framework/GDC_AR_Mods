# GDC Mission Maker Tools

[English](README.md) | Français

Des outils pour le Workbench Enfusion ("Arma Reforger Tools") pour faciliter la vie des créateurs de mission.

/!\ Inutile d'ajouter cet addon aux dépendances de votre mission. Chargez le avec la fonction "Open With Addons" accessible par le lanceur Enfusion (clic droit sur votre mod de mission).

Que propose cet addon ?

  * Une bibliothèque d'entités alla 3den
  * Un éditeur de loadouts 
  * Un squelette de mission en un clic
  * Une vérification de mission


## Une bibliothèque d'entités alla 3den

Le resource browser du World Editor donne accès à toutes les entités chargées : il y a en a littéralement des milliers, dont seule une petite partie est utile au créateur de missions. GDC Mission Maker tools propose un plugin permettant de créer une bibliothèque des entités utiles au créateurs de mission, directement accessible par le panneau natif du World Editor **Prefab Library**.

Mode d'emploi : [docs/GDC_MMLibraries.fr.md](docs/GDC_MMLibraries.fr.md).


## Un éditeur de loadouts (un peu) mieux

Un outil du World Editor (icône valise de la barre d'outils) pour modifier le loadout d'un prefab de personnage sur un seul panneau : tenue, armes, équipements et inventaire, avec la capacité réelle de chaque conteneur. 

Mode d'emploi : [docs/GDC_LoadoutEditor.fr.md](docs/GDC_LoadoutEditor.fr.md).


## Un squelette de mission en un clic

Un plugin du World Editor qui ajoute à un nouveau monde de mission tout ce par quoi commence une mission GDC : calques, managers, mode de jeu Reforger Lobby avec les réglages habituels, framework TILW, sections du briefing et config de fréquences Coalition VON. Il n'ajoute que ce qui manque : on peut le relancer sans risque.

Mode d'emploi : [docs/GDC_MissionSkeleton.fr.md](docs/GDC_MissionSkeleton.fr.md).


## Une vérification de mission

Un plugin du World Editor (Plugins > GDC > Check Mission, Ctrl+Alt+C) qui vérifie le monde de mission ouvert sans le modifier : ressources introuvables, proxys de la bibliothèque MM, entités qui ont perdu leur prefab, pièges de TILW… Les blocs à lancer et la langue (anglais ou français) se règlent dans Plugins > Settings > Check Mission. Le rapport est dans la Log Console (filtre `[GDC_CHECK]`) : chaque constat dit où, ce qui ne va pas et que faire, avec un lien qui sélectionne l'entité.

Mode d'emploi et liste de tous les contrôles : [docs/GDC_CheckMission.fr.md](docs/GDC_CheckMission.fr.md). Pourquoi une règle existe, la preuve et que faire : [base de connaissance de la vérification de mission](GDC_AR_MissionCheck_KB/README.fr.md).

