# Player loadout editor (GDC Mission Maker Tools)

English | [Français](GDC_LoadoutEditor.fr.md)

The **GDC Loadout Editor** is a World Editor tool to edit the loadout of a character prefab without opening its components one by one: clothing, weapons, equipment (binoculars, watch) and inventory. It is designed to quickly prepare the player roles of a mission. The tool:

- shows the whole loadout on a single panel, with slot and prefab names;
- shows each pocket, pouch or backpack with its real capacity (volume, slots, weight) and flags what will not fit in game;
- can automatically distribute a list of items (magazines, grenades, medical supplies...) into the worn gear;
- works with vanilla, RHS or GDC prefabs, as long as they are in an editable project.

The tool is provided by the **GDC Mission Maker Tools** addon. It adds nothing to the game: it only edits prefabs in the Workbench. There is no need to add this addon as a dependency of a mission: load it to edit, by ticking it in "Open With Addons" when starting the Workbench.

## Before you start

- The **GDC Mission Maker Tools** addon must be loaded in the Workbench, along with **every mod the character's gear comes from** (RHS for example). Without them, the tool cannot read the gear: it says so and refuses to apply changes.
- A **world must be open** in the World Editor (any world, a test world is enough).
- The prefab to edit must be **in one of your projects** (your mission mod for example). Prefabs of the base game and of packed mods (RHS, Workshop mods) are read-only.

> **Tip** — To edit an RHS or vanilla soldier, first create **your own prefab** from it in your mission mod (from the Resource Browser, by creating an inherited prefab or a copy), then edit this new prefab with the tool. Store it in a folder of your mission, for example `YourMissionMod/Prefabs/Characters/MyMission/`.

## Open the tool

In the World Editor, click the **GDC backpack** icon in the toolbar. The tool panel opens on the right.

## The 4 steps

1. **Choose the character**: in the **Character** section, select the prefab in the **Prefab** field, then click **Load**. The panel fills with the prefab's loadout. The console shows the file that will be modified, or that the prefab is read-only.
2. **Edit** the loadout in the **Worn**, **Weapons**, **Equipment** and **Inventory** sections (see below).
3. Click **Apply**: the changes are applied to the prefab. A progress bar is shown during the operation.
4. **Save**: the tool then offers **Save world** (saves the world and the prefab, like Ctrl+S) or **Later**. As long as the world is not saved, the prefab file is not modified on disk.

> **Important** — **Save world** also saves all other pending changes of the open world, exactly like Ctrl+S.

Until you click **Apply**, nothing is written: a new **Load** cancels all your changes.

## Panel sections

### Worn (clothing)

One entry per clothing slot of the character (`HAT`, `JACKET`, `PANTS`, `VEST`, `BACK`...), with the worn prefab. Change the prefab to change the clothing, clear it to remove the item. Slots are read from the prefab: slots added by mods (armband, glasses...) appear too.

> **Important** — After changing the jacket, backpack, pants or vest, click **Refresh Containers** to update the list of inventory containers (see [Changing clothing](#changing-clothing)).

### Weapons

One entry per weapon slot of the character, for example `WEAPON SLOT 0 (PRIMARY)`, `WEAPON SLOT 2 (SECONDARY)`, `GRENADE SLOT 3 (GRENADE)`. Choose the weapon or grenade of each slot, or clear it.

The tool does not handle weapon attachments (scope, suppressor...): choose a **weapon variant** that already has them.

### Equipment

The character's equipment slots, such as `BINOCULARSLOT` (binoculars) and `WRISTWATCHSLOT` (watch).

### Inventory

One **container** per storage of the worn gear: the jacket, the pants, the backpack, and each pouch of the vest. Expand a container to see and edit its items (**Items**): one prefab and one quantity per line. Setting the quantity to 0 removes the item.

The title of each container reads as follows:

```
VEST > m67_2 (Pouch_Filbe_M67_Base)  ·  1 item, 1 / 1 slots
```

- on the left, the storage: the worn gear (in capitals), the pouch and its prefab;
- after the dot, the content and the capacity.

There are two kinds of storage:

| Type | Examples | Capacity shown | What matters in game |
|---|---|---|---|
| **Volume-based** | jacket, pants, backpack, vest, some pouches | `860 / 1600` (used volume / max volume) and, if set, `1.2 / 10.0 kg` | volume, max weight and max size of an item |
| **Slot-based** | most RHS vest pouches | `2 / 2 slots` | one item per slot, and **only the accepted item types** (for example an M67 pouch only accepts a fragmentation grenade) |

The last containers of the list are special:

- **AUTOMATIC PLACEMENT**: items placed here have no precise destination; the game stores them wherever it finds room. It is always last in the list.
- Containers whose title is a path (for example `Vest/Vest_PCGen_III_rifleman_filbe.et/nerd/Pouch_PCGEN3_ifak.et`) target a pouch **that is not on the current gear**. Their items will not be created there in game.

### Alerts

| Icon | Message | Meaning |
|---|---|---|
| ❗ | `OVER CAPACITY` | Too many items for the volume or number of slots: the excess will not be created. |
| ❗ | `OVERWEIGHT` | The storage's max weight is exceeded (the content of attached pouches counts). |
| ❗ | `NOT ACCEPTED: <item>` | This item type is not accepted by this pouch (a magazine in a grenade pouch for example). |
| ❗ | `TOO LARGE: <item>` | The item is larger than what the storage accepts. |
| ⚠ | `NOT ON CURRENT GEAR` | The targeted storage does not exist on the worn gear. |
| 🟡 | `AUTOMATIC PLACEMENT` | Items are in automatic placement: the game will decide where they go. |

The icon is placed at the start of the line, to spot problems at a glance. Each alert is also detailed in the Workbench console.

## Changing clothing

When you change a clothing item in **Worn**, click **Refresh Containers**. The tool then adapts the inventory to the new gear:

1. each container is attached to the equivalent storage of the new gear: same slot name (`radio` → `radio`), or failing that **same pouch model** if there is only one;
2. items that find no equivalent storage are moved to **Items To Distribute** (**Automatic Distribution** section), and the console lists what was moved.

You can then distribute them with **Distribute Items** (see below) or place them by hand.

> **Warning** — Items left in **Items To Distribute** are not part of the loadout: if you click **Apply** without distributing them, they are removed from the character.

## Distributing items automatically

The **Automatic Distribution** section stores a list of items without choosing each pouch by hand.

1. (Optional) **Clear Inventory** empties the whole inventory of the panel, after confirmation. Nothing is written before **Apply**.
2. Add the items to **Items To Distribute** (**+** button, then one prefab and one quantity per line).
3. If needed, set **Distribution Order**: the order in which the gear is filled, as slot names separated by commas. Default: `Vest,ArmoredVest,Jacket,Pants,Back`.
4. Click **Distribute Items**.

For each item, the tool first looks for a free **slot-based pouch** that accepts this item type (a grenade goes into a grenade pouch), then for a **volume-based storage** that still has room, respecting max volume, weight and size. What fits nowhere goes to **AUTOMATIC PLACEMENT** and is reported in the console. The **Items To Distribute** list is emptied once the distribution is done.

Check the result in **Inventory**, then click **Apply**.

## Buttons

| Button | Role |
|---|---|
| **Load** | Loads the prefab chosen in **Character**. Also cancels all changes not applied. |
| **Refresh Containers** | Adapts the inventory after a clothing change. |
| **Clear Inventory** | Empties the whole inventory of the panel, after confirmation. |
| **Distribute Items** | Distributes the items of **Items To Distribute** into the gear. |
| **Apply** | Applies the changes to the prefab, then offers to save the world. |

## Safeguards

The tool refuses or asks for confirmation in the following cases:

- **Read-only prefab** (base game or packed mod): Apply shows an error and writes nothing.
- **Missing mod**: if prefabs of the loadout cannot be loaded (RHS not loaded for example), the console lists them at Load and **Apply is blocked**.
- **Unusual write**: if Apply would lose more than half of the inventory, empty several clothing slots or put the same item in several slots, a confirmation is asked (**Cancel** by default).
- **Prefab modified elsewhere**: if the prefab changed since the Load (edited by hand for example), Apply refuses to write; click **Load** and start again.

All tool messages in the console start with `[GDC LoadoutEditor]`.

## FAQ

**Nothing happens when I choose a prefab.**
Click **Load**: the Workbench does not notify the tool when the field changes.

**Right-click Copy/Paste does not work to paste items.**
It is a Workbench limitation: **Paste** is disabled in the tool panel. Type the items by hand, or use automatic distribution.

**I applied but my file did not change.**
The tool edits the prefab in memory: save the world (**Save world** in the offered window, or Ctrl+S).

**A pouch shows `0 / 0`, or a body armor shows `OVER CAPACITY` with a single item.**
Some gear has no storage room (a plate carrier alone for example): any item placed in it will not be created in game. Place it elsewhere.

**Items stay in `NOT ON CURRENT GEAR` after a Load.**
The prefab targets pouches that are not on the current gear. Click **Refresh Containers**: the items are attached to an equivalent pouch, or moved to **Items To Distribute**.
