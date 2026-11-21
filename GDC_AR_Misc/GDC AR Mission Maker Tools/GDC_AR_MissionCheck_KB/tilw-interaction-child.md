# TILW interaction placed as a child

English | [Français](tilw-interaction-child.fr.md)

Rule 5.1 · warning · addon behaviour

## What happens
A TILW interaction (`TILW_SetFlagInteraction`, probably also `TILW_TeleportInteraction`) placed as a child of a prop may not display its action label on a dedicated server. In the Workbench and in local play, everything looks fine.

## Evidence
Seen and confirmed on the GDC dedicated server in an earlier mission. No other effect is known.

## What to do
Place the interaction as a root entity at the prop's position, with "Display UI At Reference Point" ticked on its UserActionContext. Example: the "Fin de mission" interactions of Entre Deux (framework layer).

## When to remove the rule
When a test on a dedicated server, with a newer TILW, shows the label working on a child interaction.

## History
- Before 2026-10-07: bug seen on the dedicated server.
- 2026-11-20: rule written (TILW 1.0.128).
