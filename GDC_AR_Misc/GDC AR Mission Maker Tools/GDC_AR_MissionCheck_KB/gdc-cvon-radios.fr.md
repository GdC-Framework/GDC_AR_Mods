# Radio qui n'est pas une radio Coalition VON (GDC)

[English](gdc-cvon-radios.md) | Français

Règle 3.4 · erreur · convention GDC

## Ce qui se passe
Les missions GDC utilisent Coalition VON (CVON) pour les radios. Un slot jouable qui démarre avec une autre radio (jeu de base, ACE, autre mod) reçoit une radio qui ne fonctionne pas avec CVON.

## Comment la règle reconnaît une radio CVON
Une radio est un objet doté d'un `BaseRadioComponent` ; une radio CVON a aussi un `CVON_RadioComponent`. Le prefab de l'objet est lu tel qu'il est chargé : CVON remplace certaines radios du jeu de base avec le même GUID (AN/PRC-68, R-148), qui comptent alors comme radios CVON.

Vérifié : les objets avec lesquels le prefab de slot démarre (inventaire de départ, emplacements d'équipement). Non vérifié : l'équipement porté. Un sac radio du jeu de base (AN/PRC-77, R-107M) est justement ce dont CVON a besoin sur le dos pour utiliser une radio longue portée en manpack.

## Que faire
Remplacez la radio dans le loadout du prefab de slot par une radio CVON : AN/PRC-152, AN/PRC-117, R-148, R-168, R-187…

## Quand retirer la règle
Si GDC cesse d'utiliser Coalition VON.

## Historique
- 21/11/2026 : règle écrite (Coalition VON 3.2.4).
