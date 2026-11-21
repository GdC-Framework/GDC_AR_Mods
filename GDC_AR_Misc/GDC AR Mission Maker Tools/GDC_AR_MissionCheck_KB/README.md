# Knowledge base for mission checking

English | [Français](README.fr.md)

The Check Mission plugin of the World Editor builds its checks on this knowledge base. This lets the checks evolve with what mission makers or players discover, or with what the supervision of the modset brings to light.

The report of Check Mission, in the Log Console, gives the address of the sheet of each finding.

| Rule | Sheet | Type |
|---|---|---|
| 2.2 | [MM library proxy used](gdc-mm-library-proxy.md) | GDC convention |
| 2.5 | [Excluded addon loaded](gdc-excluded-addons.md) | GDC convention |
| 4.1 | [Entity deleter without a filter](tilw-entity-deleter.md) | addon behaviour (TILW) |
| 5.1 | [Interaction placed as a child](tilw-interaction-child.md) | addon behaviour (TILW) |
| 5.2 | [Vehicle crew that never spawns](tilw-vehicle-crew.md) | addon behaviour (TILW) |

## Maintaining the sheets (AI food)

- One sheet per rule, existing or planned: `<id>.md` and `<id>.fr.md`. The id is lowercase words joined by `-`, starting with the origin of the rule: `gdc-`, `tilw-`, `engine-`...
- Plan of a sheet: what happens, cause or evidence, what to do, when to remove the rule, history. Short.
- Line under the language switch: `Rule 5.2 · error · addon behaviour`, or `Rule planned · ...` for a rule not written yet.
- In the rule catalogue (`scripts/WorkbenchGame/GDC/MissionCheck/GDC_MMMissionCheckCatalog.c`): `rule.m_sKB = "<id>";`, and for an addon behaviour, `rule.SetVerified(addon, version, date)` at each new verification.
- List the sheet in both indexes, then run `python Tools/MissionCheckKB/check_kb.py` and commit the sheet with its rule.
