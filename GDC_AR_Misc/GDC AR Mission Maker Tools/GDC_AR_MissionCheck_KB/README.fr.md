# Base de connaissance de la vérification de mission GDC

Pourquoi les règles de *Check Mission* (GDC Mission Maker Tools) existent : ce qui se passe, la preuve, que faire et quand retirer la règle. Une fiche par règle, en anglais et en français ([English version](README.md)).

Le rapport de Check Mission, dans la Log Console, donne l'adresse de la fiche de chaque constat.

| Règle | Fiche | Type |
|---|---|---|
| 2.2 | [Proxy de bibliothèque MM utilisé](gdc-mm-library-proxy.fr.md) | convention GDC |
| 4.1 | [Deleter sans filtre](tilw-entity-deleter.fr.md) | comportement d'addon (TILW) |
| 5.1 | [Interaction placée en enfant](tilw-interaction-child.fr.md) | comportement d'addon (TILW) |
| 5.2 | [Équipage de véhicule qui n'apparaît jamais](tilw-vehicle-crew.fr.md) | comportement d'addon (TILW) |

## Tenir les fiches à jour

- Une fiche par règle, existante ou prévue : `<id>.md` et `<id>.fr.md`. L'identifiant est en minuscules avec des tirets, et commence par le framework : `gdc-`, `tilw-`, `engine-`…
- Plan d'une fiche : ce qui se passe, cause ou preuve, que faire, quand retirer la règle, historique. Court.
- Deuxième ligne : `Règle 5.2 · erreur · comportement d'addon`, ou `Règle prévue · …` pour une règle pas encore écrite (`Rule planned · …` en anglais).
- Dans le catalogue des règles (`scripts/WorkbenchGame/GDC/MissionCheck/GDC_MMMissionCheckCatalog.c`) : `rule.m_sKB = "<id>";`, et pour un comportement d'addon, `rule.SetVerified(addon, version, date)` à chaque nouvelle vérification.
- Ajouter la fiche aux deux index, lancer `python Tools/MissionCheckKB/check_kb.py`, puis commiter la fiche avec sa règle.
