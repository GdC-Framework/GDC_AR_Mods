# No playable group (RL)

English | [Français](rl-playable-groups.fr.md)

Rule 3.1 · error · addon behaviour

## What happens
Reforger Lobby builds its slots from the AI groups that have Set Playable ticked (`m_bSetPlayable`). Without any, the lobby has no slot to offer and nobody can join the mission.

## What to do
Tick Set Playable on the groups of the players (the `players` layer of a GDC mission).

## When to remove the rule
If Reforger Lobby takes its slots from something else than playable groups.

## History
- 2026-11-21: rule written (Reforger Lobby 1.8.23).
