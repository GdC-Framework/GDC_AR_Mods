# Base de connaissance pour la vérification de mission

[English](README.md) | Français

Le plugin Check Mission du World Editor enrichit ses tests grâce à cette base de connaissance. Cela permet aux tests d'évoluer en fonction de ce qui est découvert par les MM ou les joueurs, ou au décours de la supervision du modset.

Le rapport de Check Mission, dans la Log Console, donne l'adresse de la fiche de chaque constat.

| Règle | Fiche | Type |
|---|---|---|
| 2.2 | [Proxy de bibliothèque MM utilisé](gdc-mm-library-proxy.fr.md) | convention GDC |
| 2.4 | [Addon exclu chargé](gdc-excluded-addons.fr.md) | convention GDC |
| 3.1 | [Aucun groupe jouable](rl-playable-groups.fr.md) | comportement d'addon (RL) |
| 3.4 | [Radio qui n'est pas une radio Coalition VON](gdc-cvon-radios.fr.md) | convention GDC |
| 4.1 | [Deleter TILW sans filtre](tilw-entity-deleter.fr.md) | comportement d'addon (TILW) |
| 4.2 | [Équipage TILW de véhicule qui n'apparaît jamais](tilw-vehicle-crew.fr.md) | comportement d'addon (TILW) |
| 5.1 | [Interaction TILW placée en enfant](tilw-interaction-child.fr.md) | comportement d'addon (TILW) |
| 5.2 à 5.5 | [Triggers TILW mal réglés](tilw-triggers.fr.md) | comportement d'addon (TILW) |
| 5.6 à 5.14 | [Événements de mission TILW mal réglés](tilw-mission-events.fr.md) | comportement d'addon (TILW) |

## Tenir les fiches à jour (IA food)

- Une fiche par règle, existante ou prévue : `<id>.md` et `<id>.fr.md`. L'identifiant est en minuscules avec des tirets, et commence par l'origine de la règle : `gdc-`, `tilw-`, `rl-`, `engine-`…
- Plan d'une fiche : ce qui se passe, cause ou preuve, que faire, quand retirer la règle, historique. Court.
- Ligne sous la bascule de langue : `Règle 5.2 · erreur · comportement d'addon`, ou `Règle prévue · …` pour une règle pas encore écrite (`Rule planned · …` en anglais).
- Dans le catalogue des règles (`scripts/WorkbenchGame/GDC/MissionCheck/GDC_MMMissionCheckCatalog.c`) : `rule.m_sKB = "<id>";`, et pour un comportement d'addon, `rule.SetVerified(addon, version, date)` à chaque nouvelle vérification.
- Ajouter la fiche aux deux index, lancer `python Tools/MissionCheckKB/check_kb.py`, puis commiter la fiche avec sa règle.
