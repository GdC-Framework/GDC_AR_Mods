# Radio that is not a Coalition VON radio (GDC)

English | [Français](gdc-cvon-radios.fr.md)

Rule 3.5 · error · GDC convention

## What happens
GDC missions use Coalition VON (CVON) for the radios. A playable slot that starts with another radio (base game, ACE, another mod) gets a radio that does not work with CVON.

## How the rule recognises a CVON radio
A radio is an item with a `BaseRadioComponent`; a CVON radio also has a `CVON_RadioComponent`. The item prefab is read as loaded: CVON replaces some base game radios with the same GUID (AN/PRC-68, R-148), which then count as CVON radios.

Checked: the items the slot prefab starts with (initial inventory items, equipment slots). Not checked: the worn equipment. A base game radio backpack (AN/PRC-77, R-107M) is what CVON needs on the back to use a long-range radio as a manpack.

## What to do
Replace the radio in the loadout of the slot prefab with a CVON radio: AN/PRC-152, AN/PRC-117, R-148, R-168, R-187...

## When to remove the rule
If GDC stops using Coalition VON.

## History
- 2026-11-21: rule written (Coalition VON 3.2.4).
