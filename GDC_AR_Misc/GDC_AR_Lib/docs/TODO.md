# TODO GDC AR Lib

## Réduire les dépendances de l'addon

`addon.gproj` déclare TILW, PSCore, Reforger Lobby et Coalition VON : toute mission qui charge GDC AR Lib entraîne ces mods, même si elle n'utilise pas les fonctions concernées.

| Mod | Origine relevée dans le code |
|---|---|
| TILW | `GDC_ArtilleryFireSupportComponent` (module d'artillerie IA) : déclenchement par flag de mission |
| Reforger Lobby, PSCore | Aucune référence `PS_` dans les scripts, prefabs et layouts de la bibliothèque : à confirmer, puis retirer si rien ne les utilise |
| Coalition VON | `GDC_SCR_GroupsManagerComponent_SlotRadios` (radios par slot) |
