# _Media : sources des images

Ce dossier contient les images sources (loading screens, vignettes, pages Workshop, textures custom) utilisées par les addons GDC.

Il est volontairement **en dehors des dossiers d'addons** pour ne pas être affiché dans le Workbench.
## Organisation

| Dossier | Contenu |
|---|---|
| `LoadingScreens/_Originaux/` | Sources pleine résolution : captures, photos, fichiers PSD/XCF/KRA. **Jamais modifiés par les outils** |
| `LoadingScreens/Exports/` | Exports recadrés 16:9 et redimensionnés en 1280 × 720, générés par le script, prêts à importer (`m_sLoadingScreen`) |
| `Tools/` | Outils de préparation des images |
| `Thumbnails/` | Vignettes de missions (`m_sIcon`) |
| `Workshop/` | Images des pages Workshop (`thumbnail.png`, preview, galerie) |
| `Textures/` | Sources des textures custom (affiches, panneaux, etc.) |

Les dossiers `_Originaux/` peuvent être ajoutés dans chaque catégorie si besoin.

## Convention de nommage

Un export porte **le même nom que le `.edds` qu'il produit** dans l'addon :

```
_Media/LoadingScreens/_Originaux/huey_1.png
  → _Media/LoadingScreens/Exports/huey_1.png
  → GDC_AR_ModPack/GDC_AR_Core/UI/MissionLoadingScreens/huey_1.edds
```

Pas d'accents ni de caractères spéciaux dans les noms de fichiers et de dossiers (Workbench ignore les dossiers dont le nom contient des caractères non ASCII).

## Loading screens : règles

- **Format 16:9**, chaque côté **multiple de 4** (sinon pas de compression GPU, fichier 3 à 4 fois plus lourd).
- Taille recommandée : **1280 × 720** ; 1920 × 1080 au maximum.
- PNG **RVB sans couche alpha**.
- Les loading screens partagés vont dans **GDC AR Core** (`UI/MissionLoadingScreens/`), pour n'être téléchargés qu'une fois.

Ordre de grandeur attendu : environ 0,9 Mo par `.edds` en 1280 × 720 sans mipmaps.

## Procédure (loading screens)

1. Placer l'original dans `LoadingScreens/_Originaux/`, avec son nom définitif (sans accent ni espace).
2. Double-cliquer sur `Tools/Preparer_LoadingScreens.cmd` : les originaux nouveaux ou modifiés sont recadrés au centre en 16:9 et exportés en 1280 × 720 dans `LoadingScreens/Exports/`. Pour un autre cadrage que le centre, déposer dans `_Originaux/` une version déjà recadrée en 16:9.
3. Dans Workbench (projet GDC AR Core ouvert depuis le dépôt Git) : **Resource Manager > Plugins > GDC Import Loading Screens**. Tous les exports sont copiés dans `GDC_AR_Core/UI/MissionLoadingScreens/` puis importés ou ré-importés (sans mipmaps). Le PNG copié reste à côté du `.edds` : c'est sa source pour Workbench, et la publication d'une mission le copie dans le bundle quand l'image sert d'icône ou d'aperçu (`m_sIcon`, `m_sPreviewImage`). **Ne jamais le supprimer** : sans lui, le `.edds` est effacé par Workbench et la publication échoue (« Cannot copy image »). Il est versionné via LFS. Une image déjà importée garde son GUID : les missions qui la référencent restent valides.
4. Référencer le `.edds` dans le `.conf` de la mission (`m_sLoadingScreen`, `m_sPreviewImage`…).
5. Committer l'original et l'export dans `_Media/` en même temps que le `.edds`, son `.meta` et le PNG de l'addon.

Le script nécessite Python ; le `.cmd` installe Pillow au premier lancement si besoin. Le plugin se trouve dans GDC AR Lib (`scripts/WorkbenchGame/GDC/LoadingScreens/`).

## Git LFS

Les images de ce dossier (`png`, `jpg`, `psd`, `xcf`, `kra`, `tif`) sont stockées avec **Git LFS** (voir `.gitattributes` à la racine du dépôt). Pour les récupérer, chaque contributeur doit installer Git LFS une fois :

```
git lfs install
```

Le quota LFS gratuit de GitHub est limité (stockage et bande passante) : éviter d'y versionner des fichiers inutilement lourds (vidéos, captures en 4K multiples).
