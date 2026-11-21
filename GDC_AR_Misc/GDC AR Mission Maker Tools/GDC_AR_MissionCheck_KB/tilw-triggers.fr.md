# Triggers TILW mal réglés

[English](tilw-triggers.md) | Français

Règles 5.2 à 5.5 · avertissement ou erreur · comportement d'addon

## Ce qui se passe
Un trigger TILW (`TILW_PresenceTriggerEntity`, `TILW_FactionTriggerEntity`) n'agit sur la mission que par le flag qu'il lève. Certains réglages le rendent inutile ou affichent une erreur aux joueurs :

| Règle | Réglage | Effet |
|---|---|---|
| 5.2 | aucun nom de flag (`m_flagName`) | le trigger change d'état, mais rien dans la logique de mission ne peut y réagir |
| 5.3 | trigger de faction sans faction propriétaire (`m_ownerFactionKey`) | il ne peut jamais être capturé ; ses messages d'état cherchent une faction qui n'existe pas |
| 5.4 | messages d'état activés (`m_sendStatusMessages`) avec une liste qui n'a pas exactement 6 entrées | les joueurs voient « ERROR: Trigger Status Messages are not configured properly » |
| 5.5 | trigger de présence sans nom d'entité, ou avec des noms que rien ne porte | il ne peut jamais les compter |

## Preuve
Lu dans les scripts de TILW (`TILW_BaseTriggerEntity`, `TILW_FactionTriggerEntity`, `TILW_PresenceTriggerEntity`), version 1.0.128.

## Que faire
Renseignez le flag, la faction propriétaire, les 6 messages ou les noms d'entités. Un nom donné à l'apparition d'une entité (`m_setEntityName(s)` d'un spawner ou d'une instruction) compte comme porté.

## Quand retirer les règles
Si TILW change l'usage de ces réglages par ses triggers : regarder ses scripts à chaque mise à jour.

## Historique
- 21/11/2026 : règles écrites (TILW 1.0.128).
