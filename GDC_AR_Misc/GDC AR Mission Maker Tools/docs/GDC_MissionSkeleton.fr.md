# Squelette de mission (GDC Mission Maker Tools)

[English](GDC_MissionSkeleton.md) | Français

Le plugin **Create Mission Skeleton** ajoute à votre monde de mission tout ce par quoi commence une mission GDC : calques, managers, mode de jeu, framework de mission, sections du briefing et fréquences radio. Il n'ajoute que ce qui manque : vous pouvez le relancer à tout moment, il ne modifie jamais ce qui existe déjà.

## Utilisation

1. Créez votre monde de mission comme sous-scène du terrain et **enregistrez-le** (le plugin refuse un monde jamais enregistré).
2. Lancez **Plugins > GDC > Create Mission Skeleton**.
3. Lisez le récapitulatif : ce qui a été ajouté, ce qui existait déjà, ce qu'il faut vérifier.
4. Enregistrez le monde (Ctrl+S).

Vous changez d'avis ? Ctrl+Z annule toute l'exécution d'un coup (la config de fréquences écrite à côté du monde reste).

## Ce qu'il ajoute

| Élément | Détail |
|---|---|
| Calques | `default`, `briefing`, `players`, `systems`, et les dossiers `ia` et `build` avec chacun un premier calque (`ia/opfor`, `build/decor`) : git ne conserve pas les dossiers vides. Un calque existant est repris quelle que soit sa casse (`Players`). |
| AI world | Celui du terrain quand son mod en fournit un (il porte les navmeshes du terrain), sinon le générique. Limite d'IA actives : 128. Pour les terrains du jeu de base (Arland, Eden, Cain), les navmeshes manquants sont remplis avec ceux de Conflict. |
| Perception manager | |
| Mode de jeu | Reforger Lobby (`PS_GameMode_Lobby`), avec les réglages habituels chez GDC : pas de marqueurs de chefs de groupe, ramasse-miettes désactivé, lobby accessible pendant la partie, briefing commun à tous, Arma Vision autorisée, pas de temps de gel. |
| Framework de mission | TILW (`TILW_MF`), dans `systems`. |
| Briefing | Sections Contexte, Situation, Mission, Renseignements, Insertion et extraction, dans cet ordre, visibles par tous, chacune avec un court texte de départ à remplacer. |
| Fréquences | Une config de fréquences Coalition VON vide à côté du monde (`Configs/<monde>_Frequencies.conf`), liée au mode de jeu. |

Un AI world, un perception manager ou un framework déjà fourni par le terrain n'est pas doublé. Le mode de jeu et le briefing ne sont cherchés que dans votre mission.

## À vérifier après une exécution

- **« AI world navmeshes without a file »** : sans eux, les IA ne peuvent pas se déplacer. Renseignez les fichiers dans l'AI world (NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig). Ça arrive avec un terrain dont le mod ne fournit pas de prefab d'AI world.
- **« prefabs of the following addons cannot be loaded »** : rien n'a été modifié. Chargez les addons cités (GDC AR Lib amène Reforger Lobby et TILW), puis relancez le plugin.

## Votre propre squelette

Le squelette est décrit par une config : celle de GDC est `Configs/GDC MM/MissionSkeleton.conf`, en lecture seule comme tout l'addon. Pour le modifier :

1. Dans le Resource Browser, clic droit sur `MissionSkeleton.conf` > **Create Inherited File**, dans votre addon **GDC Mission Maker Local** (l'addon local créé par [Update MM Libraries](GDC_MMLibraries.fr.md) : il n'est jamais publié).
2. Dans la nouvelle config, ne changez que ce dont vous avez besoin : calques, entités et leurs valeurs, sections du briefing, navmeshes par terrain. Chaque champ est décrit dans l'éditeur. Tout ce que vous ne touchez pas suit la config de GDC, mises à jour comprises.
3. Sélectionnez-la dans **Plugins > Settings > Create Mission Skeleton** (Skeleton Config). Le Workbench conserve ce réglage d'une session à l'autre.

Ne mettez jamais cette config dans votre addon de mission : elle serait publiée avec la mission, et elle fait référence à GDC Mission Maker Tools, qui ne doit jamais être une dépendance de mission.
