# Proxy de bibliothèque MM utilisé (GDC)

[English](gdc-mm-library-proxy.md) | Français

Règle 2.2 · erreur · convention GDC

## Ce qui se passe
Les éléments de la Prefab Library (PrefabLibrary/MM Forces, MM Markers, MM Systems) sont des proxys : des prefabs enfants vides, rangés dans votre propre addon GDC Mission Maker Local, qui n'est jamais publié. Une mission qui référence un proxy a besoin de cet addon : sur le serveur et chez les joueurs, l'entité ou la propriété ne pointe vers rien.

## Comment ça arrive
Déposer un élément dans la vue 3D ne pose pas de problème : MM Tools place le vrai prefab. En revanche, rien ne détecte un proxy choisi ou collé dans une propriété de ressource (spawner, filtre de deleter...).

## Que faire
- Entité : supprimez-la, puis redéposez l'élément depuis la Prefab Library dans la vue 3D.
- Propriété : choisissez le vrai prefab dans le Resource Browser.

## Historique
- Davay : un proxy référencé dans la propriété d'une entité « Bloc ».
