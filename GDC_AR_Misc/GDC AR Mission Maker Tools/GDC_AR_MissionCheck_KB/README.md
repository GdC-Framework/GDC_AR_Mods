# Knowledge base for mission checking

English | [Français](README.fr.md)

The Check Mission plugin of the World Editor builds its checks on this knowledge base. This lets the checks evolve with what mission makers or players discover, or with what the supervision of the modset brings to light.

The report of Check Mission, in the Log Console, gives the address of the sheet of each finding.

| Rule | Sheet | Type |
|---|---|---|
| 2.2 | [MM library proxy used](gdc-mm-library-proxy.md) | GDC convention |
| 2.4 | [Excluded addon loaded](gdc-excluded-addons.md) | GDC convention |
| 3.1 | [No playable group](rl-playable-groups.md) | addon behaviour (RL) |
| 3.4 | [Radio that is not a Coalition VON radio](gdc-cvon-radios.md) | GDC convention |
| 4.1 | [TILW entity deleter without a filter](tilw-entity-deleter.md) | addon behaviour (TILW) |
| 4.2 | [TILW vehicle crew that never spawns](tilw-vehicle-crew.md) | addon behaviour (TILW) |
| 5.1 | [TILW interaction placed as a child](tilw-interaction-child.md) | addon behaviour (TILW) |
| 5.2 to 5.5 | [TILW triggers badly set](tilw-triggers.md) | addon behaviour (TILW) |
| 5.6 to 5.14 | [TILW mission events badly set](tilw-mission-events.md) | addon behaviour (TILW) |

## Maintaining the sheets (AI food)

- One sheet per rule, existing or planned: `<id>.md` and `<id>.fr.md`. The id is lowercase words joined by `-`, starting with the origin of the rule: `gdc-`, `tilw-`, `rl-`, `engine-`...
- Plan of a sheet: what happens, cause or evidence, what to do, when to remove the rule, history. Short.
- Line under the language switch: `Rule 5.2 · error · addon behaviour`, or `Rule planned · ...` for a rule not written yet.
- In the rule catalogue (`scripts/WorkbenchGame/GDC/MissionCheck/GDC_MMMissionCheckCatalog.c`): `rule.m_sKB = "<id>";`, and for an addon behaviour, `rule.SetVerified(addon, version, date)` at each new verification.
- List the sheet in both indexes, then run `python Tools/MissionCheckKB/check_kb.py` and commit the sheet with its rule.
