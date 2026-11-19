# GDC RL-CVON : plan de test

Le pont *GDC RL-CVON* remplace *AFI CVON-RL COMMUNITY Compat* (`66E2C8C95CD0EF22`). Il faut le publier avant de pouvoir le tester à plusieurs, puis le tester sur le serveur avant de le mettre dans toutes les missions.

## Déjà vérifié par le test automatique

Le test `RLCVON` de `Tools/MissionTests` (serveur dédié et 2 clients sur Arland, environ 6 min, sans TeamSpeak) lit ce que CVON enverrait au plugin. Il passe avec CVON 3.2.4 (19/11/2026) :

- preview, choix des slots, puis un joueur dans un slot : les deux s'entendent. C'était le défaut d'AFI : on n'entendait plus personne dès qu'on prenait un slot ;
- salons du lobby : un joueur seul dans son salon local n'est plus entendu, les deux dans le salon global s'entendent ;
- les touches de voix de Reforger Lobby (directe, faction, admin G) ne déclenchent pas le VON du jeu ;
- briefing : on s'entend si et seulement si on est dans le même salon de Reforger Lobby ;
- partie : deux joueurs vivants et proches s'entendent en voix directe ;
- un joueur tué n'est plus entendu des vivants.

Pour le relancer, Workbench fermé :

```powershell
powershell -ExecutionPolicy Bypass -File Tools/MissionTests/run_mission_test.ps1 -Mission RLCVON
```

Restent aux joueurs : le vrai son dans TeamSpeak, les radios, la radio de véhicule, le combiné, et les reconnexions, qu'un client de test ne sait pas faire.

## 1. Publier

1. Dans le Workbench : ajouter le projet `GDC_AR_ModPack/GDC_RL_CVON/addon.gproj`, l'ouvrir et vérifier que la compilation des scripts passe.
2. *Publish Project* :
   - nom : **GDC RL-CVON** ;
   - visibilité : non listée si le dialogue le propose, sinon publique avec « en test » dans la description ;
   - dépendances : Reforger Lobby Community et Coalition VON.
3. Noter l'ID Workshop affiché. On attend `BCA23F9DDF701E02`, le GUID du projet. S'il est différent, utiliser le bon dans les étapes suivantes.

## 2. Préparer le serveur

Mission de test : **Murabat, dernier bastion** (radios par slot, longue portée du CdO, véhicules).

Dans sa liste de mods (`_ServerConfigs/tanin69/Murabat_Dernier_Bastion/addons.json`, reportée dans la config du serveur), remplacer l'entrée d'AFI **à la même place** :

```json
{
    "modId": "BCA23F9DDF701E02",
    "name": "GDC RL-CVON"
}
```

AFI ne doit plus figurer dans la liste : les deux ponts ensemble se contrediraient.

**Retour arrière** : remettre l'entrée d'AFI (`66E2C8C95CD0EF22`, « AFI CVON-RL COMMUNITY Compat ») et redémarrer.

## 3. Séance (environ 20 min)

| Joueur | Slot à Murabat | Fréquences attendues |
|---|---|---|
| **tanin** (admin, note les résultats) | CdO | longue portée 48000, PRC-152 38000 |
| **Warzen** | SL d'Alpha | longue portée 48000, groupe 31000 |
| **Seegan** | Chef d'équipe (TL) d'Alpha | groupe 31000, équipe 32000 ou 33000 |

Tous les trois sur TeamSpeak avec le plugin CVON de la même version que le mod (3.2.4). Pour un KO, noter qui parlait, qui n'entendait pas, et l'heure.

| # | Action | Résultat attendu |
|---|---|---|
| 1 | Lobby : Warzen prend le SL d'Alpha, tanin et Seegan restent sans slot. Chacun parle | Les trois s'entendent |
| 2 | Lobby : Seegan, avec son slot, quitte le serveur et revient aussitôt. Chacun parle | Les trois s'entendent. Seegan doit se remettre prêt ou reprendre son slot : sinon Reforger Lobby le lui retire 2 minutes après son départ |
| 3 | Partie : Seegan appelle sur 31000, puis Warzen sur 48000 | Fréquences du tableau. Warzen entend Seegan, tanin non ; tanin entend Warzen, Seegan non |
| 4 | Warzen monte dans un véhicule avec radio, fait le key fill et appelle tanin | La radio du véhicule apparaît dans les radios de Warzen, et tanin reçoit l'appel |
| 5 | Si un slot porte un sac radio avec la longue portée (sinon, sauter) : Seegan prend le combiné de ce porteur (*Grab Handset*) et appelle tanin | tanin reçoit l'appel. Cette fonction ne marchait pas avec AFI |
| 6 | Warzen, à pied à côté de Seegan, quitte le serveur et revient. Warzen parle, puis appelle tanin sur 48000 | Warzen reprend son personnage. Seegan l'entend en direct, tanin reçoit l'appel. Volume et oreille des radios de Warzen revenus aux valeurs par défaut de CVON (volume 9, les deux oreilles) |

Les lignes 2 et 6 testent ce que le test automatique ne peut pas faire : le pont doit enregistrer de nouveau dans CVON un joueur qui revient, sinon personne ne l'entend plus.

## 4. Après la séance

Récupérer les logs du serveur, et ceux d'un joueur concerné par chaque KO :

- client : `Documents/My Games/ArmaReforger/logs/<date>/console.log` ;
- serveur : dossier `logs` du profil du serveur dédié.

Les donner à Claude, qui vérifie qu'aucune erreur de script ne vient de `GDC_RLCVON_*`, en particulier autour des reconnexions.

## 5. Si tout est OK

1. Publier en public, si la version de test était non listée.
2. Remplacer AFI par GDC RL-CVON dans `_ServerConfigs/addons.json` et dans les listes des missions (`_ServerConfigs/tanin69/*/addons.json`), à la même place.
3. Mettre à jour la règle d'ordre des listes de mods : skill `gdc-mod-supervision` et README de `Tools/ModSync`.
4. Retirer la rubrique du pont dans `BACKLOG.md` et noter la publication dans `CHANGELOG.md`.
