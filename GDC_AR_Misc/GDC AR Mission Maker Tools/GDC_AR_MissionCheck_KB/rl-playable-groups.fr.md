# Aucun groupe jouable (RL)

[English](rl-playable-groups.md) | Français

Règle 3.1 · erreur · comportement d'addon

## Ce qui se passe
Reforger Lobby construit ses slots à partir des groupes d'IA qui ont Set Playable coché (`m_bSetPlayable`). Sans aucun, le lobby n'a aucun slot à proposer et personne ne peut rejoindre la mission.

## Que faire
Cochez Set Playable sur les groupes des joueurs (le calque `players` d'une mission GDC).

## Quand retirer la règle
Si Reforger Lobby prend ses slots ailleurs que dans les groupes jouables.

## Historique
- 21/11/2026 : règle écrite (Reforger Lobby 1.8.23).
