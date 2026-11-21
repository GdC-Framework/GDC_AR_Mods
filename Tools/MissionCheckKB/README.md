# Vérificateur de la base de connaissance de Check Mission

`check_kb.py` vérifie la base de connaissance de la vérification de mission (`GDC_AR_Misc/GDC AR Mission Maker Tools/GDC_AR_MissionCheck_KB/`) par rapport au catalogue des règles du plugin :

- chaque fiche existe en anglais (`<id>.md`) et en français (`<id>.fr.md`) ;
- chaque règle du catalogue (`rule.m_sKB`) renvoie à une fiche qui existe ;
- chaque fiche correspond à une règle, ou se déclare « règle prévue » ;
- les deux index (`README.md`, `README.fr.md`) listent toutes les fiches.

```
python Tools/MissionCheckKB/check_kb.py
```

Il renvoie le code 1 en cas d'erreur. La façon de rédiger une fiche est décrite dans l'index de la base.
