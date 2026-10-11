# _Media : sources des images

Ce dossier contient les images sources (loading screens, vignettes, pages Workshop, textures custom) utilisées par les addons GDC.

Il est volontairement **en dehors des dossiers d'addons** : Workbench ne le voit pas, rien n'y est importé ni publié sur le Workshop. Seul git le suit.

## Organisation

| Dossier | Contenu |
|---|---|
| `LoadingScreens/` | Exports prêts à importer des écrans de chargement (`m_sLoadingScreen`) |
| `LoadingScreens/_Originaux/` | Sources pleine résolution : captures, photos, fichiers PSD/XCF/KRA |
| `Thumbnails/` | Vignettes de missions (`m_sIcon`) |
| `Workshop/` | Images des pages Workshop (`thumbnail.png`, preview, galerie) |
| `Textures/` | Sources des textures custom (affiches, panneaux, etc.) |

Les dossiers `_Originaux/` peuvent être ajoutés dans chaque catégorie si besoin.

## Convention de nommage

Un export porte **le même nom que le `.edds` qu'il produit** dans l'addon :

```
_Media/LoadingScreens/huey_1.png
  → GDC_AR_ModPack/GDC_AR_Core/UI/MissionLoadingScreens/huey_1.edds
```

Pas d'accents ni de caractères spéciaux dans les noms de fichiers et de dossiers (Workbench ignore les dossiers dont le nom contient des caractères non ASCII).

## Loading screens : règles

- **Format 16:9**, chaque côté **multiple de 4** (sinon pas de compression GPU, fichier 3 à 4 fois plus lourd).
- Taille recommandée : **1280 × 720** ; 1920 × 1080 au maximum.
- PNG **RVB sans couche alpha**.
- Les loading screens partagés vont dans **GDC AR Core** (`UI/MissionLoadingScreens/`), pour n'être téléchargés qu'une fois.

### Réglages d'import dans Workbench

Dans les propriétés d'import de l'image (configuration PC), avant de générer le `.edds` :

| Propriété | Valeur |
|---|---|
| `MaxSize` | `1024` (léger) ou `2048` (pour du 1080p) |
| `GenerateMips` | désactivé (inutile pour une image d'interface) |
| `ColorSpace` | `ToSRGB` |
| `FormatCompress` | `Best` |

Ordre de grandeur attendu : 0,5 à 1 Mo pour du 1280 × 720, 1 à 2 Mo pour du 1920 × 1080. Vérifier la taille du `.edds` généré.

## Procédure

1. Placer l'original dans `_Originaux/`.
2. Recadrer et redimensionner en 16:9, puis exporter en PNG dans le dossier de la catégorie, en respectant la convention de nommage.
3. Copier ce PNG dans le dossier cible de l'addon, l'importer dans Workbench et appliquer les réglages ci-dessus.
4. Référencer le `.edds` dans le `.conf` de la mission (`m_sLoadingScreen`, `m_sIcon`…).
5. Committer la source dans `_Media/` en même temps que le `.edds`.

## Git LFS

Les images de ce dossier (`png`, `jpg`, `psd`, `xcf`, `kra`, `tif`) sont stockées avec **Git LFS** (voir `.gitattributes` à la racine du dépôt). Pour les récupérer, chaque contributeur doit installer Git LFS une fois :

```
git lfs install
```

Le quota LFS gratuit de GitHub est limité (stockage et bande passante) : éviter d'y versionner des fichiers inutilement lourds (vidéos, captures en 4K multiples).
