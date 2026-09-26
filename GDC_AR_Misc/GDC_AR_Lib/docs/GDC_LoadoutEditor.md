# Éditeur de loadout des joueurs (GDC AR Lib)

Le **GDC Loadout Editor** est un outil du World Editor qui permet de modifier le loadout d'un prefab de personnage sans ouvrir ses composants un par un : tenue, armes, équipements (jumelles, montre) et inventaire. Il est pensé pour préparer rapidement les rôles joueurs d'une mission. L'outil :

- affiche tout le loadout sur un seul panneau, avec les noms des slots et des prefabs ;
- montre chaque poche, pochette ou sac avec sa capacité réelle (volume, emplacements, poids) et signale ce qui ne rentrera pas en jeu ;
- sait répartir automatiquement une liste d'objets (chargeurs, grenades, soins…) dans l'équipement porté ;
- fonctionne avec les prefabs vanilla, RHS ou GDC, pourvu qu'ils soient dans un projet modifiable.

L'outil est fourni par le mod **GDC AR Lib**. Il n'ajoute rien au jeu : il modifie uniquement des prefabs dans le Workbench.

## Avant de commencer

- Le mod **GDC AR Lib** doit être chargé dans le Workbench, ainsi que **tous les mods dont provient l'équipement** du personnage (RHS par exemple). Sans eux, l'outil ne peut pas lire l'équipement : il le signale et refuse d'appliquer des modifications.
- Un **monde doit être ouvert** dans le World Editor (n'importe lequel, un monde de test suffit).
- Le prefab à modifier doit se trouver **dans un de vos projets** (votre mod de mission par exemple). Les prefabs du jeu de base et des mods compilés (RHS, mods du Workshop) sont en lecture seule.

> **Astuce** — Pour modifier un soldat RHS ou vanilla, créez d'abord **votre propre prefab** à partir de celui-ci dans votre mod de mission (depuis le Resource Browser, en créant un prefab hérité ou une copie), puis éditez ce nouveau prefab avec l'outil. Rangez-le dans un dossier propre à votre mission, par exemple `VotreModDeMission/Prefabs/Characters/MaMission/`.

## Ouvrir l'outil

Dans le World Editor, cliquez sur l'icône **sac à dos GDC** de la barre d'outils. Le panneau de l'outil s'ouvre à droite.

## Le déroulé en 4 étapes

1. **Choisir le personnage** : dans la section **Character**, sélectionnez le prefab dans le champ **Prefab**, puis cliquez sur **Load**. Le panneau se remplit avec le loadout du prefab. La console indique le fichier qui sera modifié, ou que le prefab est en lecture seule.
2. **Modifier** le loadout dans les sections **Worn**, **Weapons**, **Equipment** et **Inventory** (voir ci-dessous).
3. Cliquer sur **Apply** : les modifications sont appliquées au prefab. Une barre de progression s'affiche pendant l'opération.
4. **Sauvegarder** : l'outil propose alors **Save world** (sauvegarde le monde et le prefab, comme Ctrl+S) ou **Later**. Tant que le monde n'est pas sauvegardé, le fichier du prefab n'est pas modifié sur le disque.

> **Important** — **Save world** sauvegarde aussi toutes les autres modifications en cours du monde ouvert, exactement comme Ctrl+S.

Tant que vous n'avez pas cliqué sur **Apply**, rien n'est écrit : un nouveau **Load** annule toutes vos modifications.

## Les sections du panneau

### Worn (tenue)

Un élément par slot de vêtement du personnage (`HAT`, `JACKET`, `PANTS`, `VEST`, `BACK`…), avec le prefab porté. Changez le prefab pour changer la tenue, videz-le pour retirer le vêtement. Les slots sont lus dans le prefab : les slots ajoutés par les mods (brassard, lunettes…) apparaissent aussi.

> **Important** — Après un changement de veste, de sac, de pantalon ou de gilet, cliquez sur **Refresh Containers** pour mettre à jour la liste des conteneurs de l'inventaire (voir [Changer de tenue](#changer-de-tenue)).

### Weapons (armes)

Un élément par emplacement d'arme du personnage, par exemple `WEAPON SLOT 0 (PRIMARY)`, `WEAPON SLOT 2 (SECONDARY)`, `GRENADE SLOT 3 (GRENADE)`. Choisissez l'arme ou la grenade de chaque emplacement, ou videz-le.

L'outil ne gère pas les accessoires d'une arme (lunette, silencieux…) : choisissez directement une **variante d'arme** déjà équipée.

### Equipment (équipements)

Les emplacements d'équipement du personnage, comme `BINOCULARSLOT` (jumelles) et `WRISTWATCHSLOT` (montre).

### Inventory (inventaire)

Un **conteneur** par rangement de l'équipement porté : la veste, le pantalon, le sac, et chaque pochette du gilet. Dépliez un conteneur pour voir et modifier ses objets (**Items**) : un prefab et une quantité par ligne. Mettre la quantité à 0 retire l'objet.

Le titre de chaque conteneur se lit ainsi :

```
VEST > m67_2 (Pouch_Filbe_M67_Base)  ·  1 item, 1 / 1 slots
```

- à gauche, le rangement : l'équipement porté (en majuscules), la pochette et son prefab ;
- après le point, le contenu et la capacité.

Il existe deux sortes de rangements :

| Type | Exemples | Capacité affichée | Ce qui compte en jeu |
|---|---|---|---|
| **À volume** | veste, pantalon, sac, gilet, certaines pochettes | `860 / 1600` (volume occupé / volume max) et, si défini, `1.2 / 10.0 kg` | le volume, le poids max et la taille max d'un objet |
| **À emplacements** | la plupart des pochettes de gilet RHS | `2 / 2 slots` | un seul objet par emplacement, et **seulement les types d'objets acceptés** (par exemple une pochette M67 n'accepte qu'une grenade à fragmentation) |

Les derniers conteneurs de la liste sont particuliers :

- **AUTOMATIC PLACEMENT** : les objets placés ici n'ont pas de destination précise ; c'est le jeu qui les range où il trouve de la place. Il est toujours en fin de liste.
- Les conteneurs dont le titre est un chemin (par exemple `Vest/Vest_PCGen_III_rifleman_filbe.et/nerd/Pouch_PCGEN3_ifak.et`) visent une pochette **qui n'est pas sur l'équipement actuel**. Leurs objets ne seront pas créés en jeu à cet endroit.

### Les alertes

| Icône | Message | Signification |
|---|---|---|
| ❗ | `OVER CAPACITY` | Trop d'objets pour le volume ou le nombre d'emplacements : le surplus ne sera pas créé. |
| ❗ | `OVERWEIGHT` | Le poids maximum du rangement est dépassé (le contenu des pochettes accrochées compte). |
| ❗ | `NOT ACCEPTED: <objet>` | Ce type d'objet n'est pas accepté par cette pochette (un chargeur dans une pochette à grenade par exemple). |
| ❗ | `TOO LARGE: <objet>` | L'objet est plus grand que ce que le rangement accepte. |
| ⚠ | `NOT ON CURRENT GEAR` | Le rangement visé n'existe pas sur l'équipement porté. |
| 🟡 | `AUTOMATIC PLACEMENT` | Des objets sont en placement automatique : le jeu décidera où ils vont. |

L'icône se place en début de ligne pour repérer les problèmes d'un coup d'œil. Le détail de chaque alerte est aussi écrit dans la console du Workbench.

## Changer de tenue

Quand vous changez un vêtement dans **Worn**, cliquez sur **Refresh Containers**. L'outil adapte alors l'inventaire au nouvel équipement :

1. chaque conteneur est rattaché au rangement équivalent du nouvel équipement : même nom d'emplacement (`radio` → `radio`), ou à défaut **même modèle de pochette** s'il n'y en a qu'un ;
2. les objets qui ne trouvent aucun rangement équivalent sont déplacés dans **Items To Distribute** (section **Automatic Distribution**), et la console liste ce qui a été déplacé.

Vous pouvez ensuite les répartir avec **Distribute Items** (voir ci-dessous) ou les placer à la main.

> **Attention** — Les objets restés dans **Items To Distribute** ne font pas partie du loadout : si vous cliquez sur **Apply** sans les distribuer, ils sont retirés du personnage.

## Répartir automatiquement les objets

La section **Automatic Distribution** permet de ranger une liste d'objets sans choisir chaque pochette à la main.

1. (Facultatif) **Clear Inventory** vide tout l'inventaire du panneau, après confirmation. Rien n'est écrit avant **Apply**.
2. Ajoutez les objets dans **Items To Distribute** (bouton **+**, puis un prefab et une quantité par ligne).
3. Réglez si besoin **Distribution Order** : l'ordre dans lequel l'équipement est rempli, sous forme de noms de slots séparés par des virgules. Par défaut : `Vest,ArmoredVest,Jacket,Pants,Back`.
4. Cliquez sur **Distribute Items**.

Pour chaque objet, l'outil cherche d'abord une **pochette à emplacements** libre qui accepte ce type d'objet (une grenade va dans une pochette à grenade), puis un **rangement à volume** qui a encore la place, en respectant le volume, le poids et la taille maximum. Ce qui ne rentre nulle part est mis en **AUTOMATIC PLACEMENT** et signalé dans la console. La liste **Items To Distribute** est vidée une fois la répartition faite.

Vérifiez le résultat dans **Inventory**, puis cliquez sur **Apply**.

## Boutons

| Bouton | Rôle |
|---|---|
| **Load** | Charge le prefab choisi dans **Character**. Annule aussi toutes les modifications non appliquées. |
| **Refresh Containers** | Adapte l'inventaire après un changement de tenue. |
| **Clear Inventory** | Vide tout l'inventaire du panneau, après confirmation. |
| **Distribute Items** | Répartit les objets de **Items To Distribute** dans l'équipement. |
| **Apply** | Applique les modifications au prefab, puis propose de sauvegarder le monde. |

## Sécurités

L'outil refuse ou demande confirmation dans les cas suivants :

- **Prefab en lecture seule** (jeu de base ou mod compilé) : Apply affiche une erreur et n'écrit rien.
- **Mod manquant** : si des prefabs du loadout ne peuvent pas être chargés (mod RHS non chargé par exemple), la console les liste au Load et **Apply est bloqué**.
- **Écriture inhabituelle** : si Apply ferait perdre plus de la moitié de l'inventaire, vider plusieurs slots de tenue ou mettre le même objet dans plusieurs slots, une confirmation est demandée (**Cancel** par défaut).
- **Prefab modifié ailleurs** : si le prefab a changé depuis le Load (modifié à la main par exemple), Apply refuse d'écrire ; faites **Load** et recommencez.

Tous les messages de l'outil dans la console commencent par `[GDC LoadoutEditor]`.

## Questions fréquentes

**Rien ne se passe quand je choisis un prefab.**
Cliquez sur **Load** : le Workbench ne prévient pas l'outil quand le champ change.

**Le Copy/Paste du clic droit ne fonctionne pas pour coller des objets.**
C'est une limite du Workbench : **Paste** est désactivé dans le panneau des outils. Saisissez les objets à la main, ou utilisez la répartition automatique.

**J'ai appliqué mais mon fichier n'a pas changé.**
L'outil modifie le prefab en mémoire : sauvegardez le monde (**Save world** dans la fenêtre proposée, ou Ctrl+S).

**Une pochette affiche `0 / 0` ou un gilet pare-balles `OVER CAPACITY` avec un seul objet.**
Certains équipements n'ont aucune place de rangement (un gilet pare-balles seul par exemple) : tout objet placé dedans ne sera pas créé en jeu. Placez-le ailleurs.

**Des objets restent en `NOT ON CURRENT GEAR` après un Load.**
Le prefab vise des pochettes qui ne sont pas sur l'équipement actuel. Cliquez sur **Refresh Containers** : les objets sont rattachés à une pochette équivalente, ou déplacés dans **Items To Distribute**.
