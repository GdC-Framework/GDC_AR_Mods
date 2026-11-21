# TILW triggers badly set

English | [Français](tilw-triggers.fr.md)

Rules 5.2 to 5.5 · warning or error · addon behaviour

## What happens
A TILW trigger (`TILW_PresenceTriggerEntity`, `TILW_FactionTriggerEntity`) only acts on the mission through the flag it sets. Some settings make it useless or show an error to the players:

| Rule | Setting | Effect |
|---|---|---|
| 5.2 | no flag name (`m_flagName`) | the trigger changes state, but nothing in the mission logic can react to it |
| 5.3 | faction trigger without an owner faction (`m_ownerFactionKey`) | it can never be captured; its status messages look up a faction that does not exist |
| 5.4 | status messages on (`m_sendStatusMessages`) with a list that has not exactly 6 entries | players see "ERROR: Trigger Status Messages are not configured properly" |
| 5.5 | presence trigger without entity names, or with names nobody holds | it can never count them |

## Evidence
Read in the TILW scripts (`TILW_BaseTriggerEntity`, `TILW_FactionTriggerEntity`, `TILW_PresenceTriggerEntity`), version 1.0.128.

## What to do
Set the flag, the owner faction, the 6 messages or the entity names. A name given when an entity spawns (`m_setEntityName(s)` of a spawner or an instruction) counts as held.

## When to remove the rules
If TILW changes how its triggers use these settings: check its scripts at each update.

## History
- 2026-11-21: rules written (TILW 1.0.128).
