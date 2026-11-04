#ifdef WORKBENCH
//! A prefab field holding one loadout prefab (worn slot, weapon slot or gadget slot) and its address in the prefab.
class GDC_LoadoutSlotRef
{
	string m_sKey;
	string m_sLabel;
	ResourceName m_Prefab;
	string m_sField;
	ref array<ref ContainerIdPathEntry> m_aPath;
}

//! One ItemsInitConfigurationItem of SCR_InventoryStorageManagerComponent.InitialInventoryItems.
class GDC_LoadoutEntryRef
{
	int m_iIndex;
	string m_sTargetStorage;
	int m_iTargetPurpose;
	ref array<ResourceName> m_aPrefabs = {};

	//! Defined in this prefab (not inherited), so it can be removed.
	bool m_bOwn;
}

//! Loadout of a character prefab as read from its entity source.
class GDC_LoadoutSnapshot
{
	ResourceName m_Prefab;
	int m_iStorageManagerIndex = -1;
	ref array<ref GDC_LoadoutSlotRef> m_aWorn = {};
	ref array<ref GDC_LoadoutSlotRef> m_aWeapons = {};
	ref array<ref GDC_LoadoutSlotRef> m_aGadgets = {};
	ref array<ref GDC_LoadoutEntryRef> m_aEntries = {};

	static GDC_LoadoutSlotRef FindSlot(notnull array<ref GDC_LoadoutSlotRef> slots, string key)
	{
		foreach (GDC_LoadoutSlotRef slot : slots)
		{
			if (slot.m_sKey == key)
				return slot;
		}

		return null;
	}

	//! Empty when both snapshots hold the same loadout, otherwise a description of the first difference.
	string Compare(notnull GDC_LoadoutSnapshot other)
	{
		string difference = CompareSlots("worn slot", m_aWorn, other.m_aWorn);
		if (difference.IsEmpty())
			difference = CompareSlots("weapon slot", m_aWeapons, other.m_aWeapons);

		if (difference.IsEmpty())
			difference = CompareSlots("gadget slot", m_aGadgets, other.m_aGadgets);

		if (!difference.IsEmpty())
			return difference;

		if (m_aEntries.Count() != other.m_aEntries.Count())
			return string.Format("number of inventory entries (%1 vs %2)", m_aEntries.Count(), other.m_aEntries.Count());

		for (int i, count = m_aEntries.Count(); i < count; i++)
		{
			GDC_LoadoutEntryRef mine = m_aEntries[i];
			GDC_LoadoutEntryRef theirs = other.m_aEntries[i];
			if (mine.m_sTargetStorage != theirs.m_sTargetStorage
				|| mine.m_iTargetPurpose != theirs.m_iTargetPurpose
				|| !GDC_LoadoutUtils.SamePrefabOrder(mine.m_aPrefabs, theirs.m_aPrefabs))
			{
				return string.Format("inventory entry %1", i);
			}
		}

		return string.Empty;
	}

	protected static string CompareSlots(string kind, notnull array<ref GDC_LoadoutSlotRef> mine, notnull array<ref GDC_LoadoutSlotRef> theirs)
	{
		if (mine.Count() != theirs.Count())
			return string.Format("number of %1s", kind);

		foreach (GDC_LoadoutSlotRef slot : mine)
		{
			GDC_LoadoutSlotRef other = FindSlot(theirs, slot.m_sKey);
			if (!other || other.m_Prefab != slot.m_Prefab)
				return string.Format("%1 %2", kind, slot.m_sLabel);
		}

		return string.Empty;
	}
}

//! A single field write.
class GDC_LoadoutWrite
{
	string m_sLabel;
	ref array<ref ContainerIdPathEntry> m_aPath;
	string m_sField;
	string m_sValue;

	void GDC_LoadoutWrite(string label, array<ref ContainerIdPathEntry> path, string field, string value)
	{
		m_sLabel = label;
		m_aPath = path;
		m_sField = field;
		m_sValue = value;
	}
}

//! An InitialInventoryItems entry to append.
class GDC_LoadoutNewEntry
{
	string m_sTargetStorage;
	int m_iTargetPurpose;
	ref array<ResourceName> m_aPrefabs = {};
}

//! Every change to make, validated before the first write.
class GDC_LoadoutPlan
{
	ref array<ref GDC_LoadoutWrite> m_aWrites = {};
	ref array<ref GDC_LoadoutNewEntry> m_aNewEntries = {};
	ref array<int> m_aRemovals = {};
	ref array<string> m_aChanges = {};

	//! Inventory item copies before and after the plan, used to ask for confirmation before a large loss.
	int m_iItemsBefore;
	int m_iItemsAfter;

	//! Worn slots the plan empties.
	int m_iWornCleared;

	//! Number of slot writes per written prefab, to spot several slots receiving the same item.
	ref map<ResourceName, int> m_mSlotValueCounts = new map<ResourceName, int>();

	//! Highest number of slots receiving the same item.
	int GetMaxSameValueWrites()
	{
		int highest;
		foreach (ResourceName value, int count : m_mSlotValueCounts)
		{
			if (count > highest)
				highest = count;
		}

		return highest;
	}

	bool IsEmpty()
	{
		return m_aWrites.IsEmpty() && m_aNewEntries.IsEmpty() && m_aRemovals.IsEmpty();
	}
}

//! Reads and writes the loadout of a character prefab.
//! Writes go through WorldEditorAPI on the prefab entity source, like vanilla SCR_PrefabHelper.
class GDC_LoadoutPrefabIO
{
	protected static const string COMPONENTS = "components";
	protected static const string SLOTS = "Slots";
	protected static const string PREFAB = "Prefab";
	protected static const string WEAPON_TEMPLATE = "WeaponTemplate";
	protected static const string INITIAL_ITEMS = "InitialInventoryItems";
	protected static const string INITIAL_STORAGE_SLOTS = "InitialStorageSlots";
	protected static const string ITEMS_INIT_CLASS = "ItemsInitConfigurationItem";
	protected static const string TARGET_STORAGE = "TargetStorage";
	protected static const string TARGET_PURPOSE = "TargetPurpose";
	protected static const string PREFABS_TO_SPAWN = "PrefabsToSpawn";

	//------------------------------------------------------------------------------------------------
	// Reading
	//------------------------------------------------------------------------------------------------

	static bool Read(notnull IEntitySource prefab, ResourceName prefabName, notnull GDC_LoadoutSnapshot snapshot)
	{
		snapshot.m_Prefab = prefabName;
		snapshot.m_aWorn.Clear();
		snapshot.m_aWeapons.Clear();
		snapshot.m_aGadgets.Clear();
		snapshot.m_aEntries.Clear();
		snapshot.m_iStorageManagerIndex = -1;

		int managerIndex = SCR_BaseContainerTools.FindComponentIndex(prefab, BaseLoadoutManagerComponent);
		if (managerIndex < 0)
		{
			Print(string.Format("[GDC LoadoutEditor] %1 has no BaseLoadoutManagerComponent, it is not a character.", prefabName), LogLevel.ERROR);
			return false;
		}

		ReadWorn(prefab, managerIndex, snapshot);
		ReadWeapons(prefab, snapshot);
		ReadGadgets(prefab, snapshot);
		ReadEntries(prefab, snapshot);
		return true;
	}

	protected static void ReadWorn(notnull IEntitySource prefab, int managerIndex, notnull GDC_LoadoutSnapshot snapshot)
	{
		BaseContainerList slots = prefab.GetComponent(managerIndex).GetObjectArray(SLOTS);
		if (!slots)
			return;

		for (int slotIndex, slotCount = slots.Count(); slotIndex < slotCount; slotIndex++)
		{
			BaseContainer slot = slots.Get(slotIndex);
			if (!slot || slot.GetName().IsEmpty())
				continue;

			ResourceName wornPrefab;
			slot.Get(PREFAB, wornPrefab);

			array<ref ContainerIdPathEntry> path = { new ContainerIdPathEntry(COMPONENTS, managerIndex), new ContainerIdPathEntry(SLOTS, slotIndex) };
			snapshot.m_aWorn.Insert(CreateSlotRef(slot.GetName(), slot.GetName(), wornPrefab, PREFAB, path));
		}
	}

	protected static void ReadWeapons(notnull IEntitySource prefab, notnull GDC_LoadoutSnapshot snapshot)
	{
		for (int componentIndex, componentCount = prefab.GetComponentCount(); componentIndex < componentCount; componentIndex++)
		{
			IEntityComponentSource component = prefab.GetComponent(componentIndex);
			if (!component)
				continue;

			typename componentType = component.GetClassName().ToType();
			if (!componentType)
				continue;

			string kind;
			if (componentType.IsInherited(CharacterGrenadeSlotComponent))
				kind = "Grenade slot";
			else if (componentType.IsInherited(CharacterWeaponSlotComponent))
				kind = "Weapon slot";
			else
				continue;

			int slotIndex;
			component.Get("WeaponSlotIndex", slotIndex);
			string slotType;
			component.Get("WeaponSlotType", slotType);

			ResourceName weaponPrefab;
			component.Get(WEAPON_TEMPLATE, weaponPrefab);

			string key = string.Format("%1:%2", component.GetClassName(), slotIndex);
			string label = string.Format("%1 %2 (%3)", kind, slotIndex, slotType);
			array<ref ContainerIdPathEntry> path = { new ContainerIdPathEntry(COMPONENTS, componentIndex) };
			snapshot.m_aWeapons.Insert(CreateSlotRef(key, label, weaponPrefab, WEAPON_TEMPLATE, path));
		}
	}

	//! Equipment slots of the character inventory storage that are saved in the loadout (binoculars, watch...).
	protected static void ReadGadgets(notnull IEntitySource prefab, notnull GDC_LoadoutSnapshot snapshot)
	{
		int inventoryIndex = SCR_BaseContainerTools.FindComponentIndex(prefab, CharacterInventoryStorageComponent);
		if (inventoryIndex < 0)
			return;

		BaseContainerList storages = prefab.GetComponent(inventoryIndex).GetObjectArray(COMPONENTS);
		if (!storages)
			return;

		for (int storageIndex, storageCount = storages.Count(); storageIndex < storageCount; storageIndex++)
		{
			BaseContainer storage = storages.Get(storageIndex);
			if (!storage)
				continue;

			bool savedInLoadout = true;
			storage.Get("SaveInLoadout", savedInLoadout);
			if (!savedInLoadout)
				continue;

			BaseContainerList slots = storage.GetObjectArray(INITIAL_STORAGE_SLOTS);
			if (!slots)
				continue;

			for (int slotIndex, slotCount = slots.Count(); slotIndex < slotCount; slotIndex++)
			{
				BaseContainer slot = slots.Get(slotIndex);
				if (!slot || slot.GetName().IsEmpty() || GDC_LoadoutSnapshot.FindSlot(snapshot.m_aGadgets, slot.GetName()))
					continue;

				ResourceName gadgetPrefab;
				slot.Get(PREFAB, gadgetPrefab);

				array<ref ContainerIdPathEntry> path = {
					new ContainerIdPathEntry(COMPONENTS, inventoryIndex),
					new ContainerIdPathEntry(COMPONENTS, storageIndex),
					new ContainerIdPathEntry(INITIAL_STORAGE_SLOTS, slotIndex)
				};
				snapshot.m_aGadgets.Insert(CreateSlotRef(slot.GetName(), slot.GetName(), gadgetPrefab, PREFAB, path));
			}
		}
	}

	protected static GDC_LoadoutSlotRef CreateSlotRef(string key, string label, ResourceName prefab, string field, notnull array<ref ContainerIdPathEntry> path)
	{
		GDC_LoadoutSlotRef slotRef = new GDC_LoadoutSlotRef();
		slotRef.m_sKey = key;
		slotRef.m_sLabel = label;
		slotRef.m_Prefab = prefab;
		slotRef.m_sField = field;
		slotRef.m_aPath = path;
		return slotRef;
	}

	protected static void ReadEntries(notnull IEntitySource prefab, notnull GDC_LoadoutSnapshot snapshot)
	{
		int managerIndex = SCR_BaseContainerTools.FindComponentIndex(prefab, SCR_InventoryStorageManagerComponent);
		snapshot.m_iStorageManagerIndex = managerIndex;
		if (managerIndex < 0)
			return;

		BaseContainerList entries = prefab.GetComponent(managerIndex).GetObjectArray(INITIAL_ITEMS);
		if (!entries)
			return;

		array<string> inheritedNames = {};
		CollectParentEntryNames(prefab, inheritedNames);

		for (int entryIndex, entryCount = entries.Count(); entryIndex < entryCount; entryIndex++)
		{
			// keep one snapshot entry per array index, even for broken entries, so indices stay aligned
			GDC_LoadoutEntryRef entryRef = new GDC_LoadoutEntryRef();
			entryRef.m_iIndex = entryIndex;
			snapshot.m_aEntries.Insert(entryRef);

			BaseContainer entry = entries.Get(entryIndex);
			if (!entry)
				continue;

			string targetStorage;
			entry.Get(TARGET_STORAGE, targetStorage);
			int targetPurpose;
			entry.Get(TARGET_PURPOSE, targetPurpose);

			// an entry whose identifier also exists in the parent prefab is inherited and cannot be removed
			string entryName = entry.GetName();
			if (entryName.IsEmpty())
				entryRef.m_bOwn = !entry.GetAncestor();
			else
				entryRef.m_bOwn = !inheritedNames.Contains(entryName);

			entryRef.m_sTargetStorage = targetStorage;
			entryRef.m_iTargetPurpose = targetPurpose;

			array<ResourceName> prefabs = {};
			if (entry.Get(PREFABS_TO_SPAWN, prefabs) && prefabs)
			{
				foreach (ResourceName item : prefabs)
				{
					if (!item.IsEmpty())
						entryRef.m_aPrefabs.Insert(item);
				}
			}
		}
	}

	//! Identifiers of the InitialInventoryItems entries of the parent prefab.
	protected static void CollectParentEntryNames(notnull IEntitySource prefab, notnull array<string> outNames)
	{
		IEntitySource parent = IEntitySource.Cast(prefab.GetAncestor());
		if (!parent)
			return;

		IEntityComponentSource parentManager = SCR_BaseContainerTools.FindComponentSource(parent, SCR_InventoryStorageManagerComponent);
		if (!parentManager)
			return;

		BaseContainerList parentEntries = parentManager.GetObjectArray(INITIAL_ITEMS);
		if (!parentEntries)
			return;

		for (int parentIndex, parentCount = parentEntries.Count(); parentIndex < parentCount; parentIndex++)
		{
			BaseContainer parentEntry = parentEntries.Get(parentIndex);
			if (parentEntry && !parentEntry.GetName().IsEmpty())
				outNames.Insert(parentEntry.GetName());
		}
	}

	protected static int CountInitialItems(notnull IEntitySource prefab, int managerIndex)
	{
		if (managerIndex < 0 || managerIndex >= prefab.GetComponentCount())
			return -1;

		BaseContainerList entries = prefab.GetComponent(managerIndex).GetObjectArray(INITIAL_ITEMS);
		if (!entries)
			return 0;

		return entries.Count();
	}

	protected static string GetEntryName(notnull IEntitySource prefab, int managerIndex, int entryIndex)
	{
		if (managerIndex < 0 || managerIndex >= prefab.GetComponentCount())
			return string.Empty;

		BaseContainerList entries = prefab.GetComponent(managerIndex).GetObjectArray(INITIAL_ITEMS);
		if (!entries || entryIndex < 0 || entryIndex >= entries.Count() || !entries.Get(entryIndex))
			return string.Empty;

		return entries.Get(entryIndex).GetName();
	}

	//------------------------------------------------------------------------------------------------
	// Planning
	//------------------------------------------------------------------------------------------------

	//! Computes the writes turning the prefab state (snapshot) into the panel state. Nothing is written here.
	static bool Plan(
		notnull GDC_LoadoutSnapshot snapshot,
		notnull array<ref GDC_LoadoutWornSlot> worn,
		notnull array<ref GDC_LoadoutWeaponSlot> weapons,
		notnull array<ref GDC_LoadoutGadgetSlot> gadgets,
		notnull array<ref GDC_LoadoutContainer> containers,
		bool removeEmptyEntries,
		notnull GDC_LoadoutPlan plan,
		notnull array<string> outErrors)
	{
		foreach (GDC_LoadoutWornSlot wornSlot : worn)
		{
			if (wornSlot)
				PlanSlot(snapshot.m_aWorn, wornSlot.m_sSlotName, wornSlot.m_Prefab, "Worn", plan, outErrors);
		}

		foreach (GDC_LoadoutWeaponSlot weaponSlot : weapons)
		{
			if (weaponSlot)
				PlanSlot(snapshot.m_aWeapons, weaponSlot.m_sKey, weaponSlot.m_Prefab, "Weapon", plan, outErrors);
		}

		foreach (GDC_LoadoutGadgetSlot gadgetSlot : gadgets)
		{
			if (gadgetSlot)
				PlanSlot(snapshot.m_aGadgets, gadgetSlot.m_sSlotName, gadgetSlot.m_Prefab, "Gadget", plan, outErrors);
		}

		PlanEntries(snapshot, containers, removeEmptyEntries, plan, outErrors);
		return outErrors.IsEmpty();
	}

	protected static void PlanSlot(
		notnull array<ref GDC_LoadoutSlotRef> slots,
		string key,
		ResourceName desired,
		string kind,
		notnull GDC_LoadoutPlan plan,
		notnull array<string> outErrors)
	{
		GDC_LoadoutSlotRef slotRef = GDC_LoadoutSnapshot.FindSlot(slots, key);
		if (!slotRef)
		{
			if (!desired.IsEmpty())
				outErrors.Insert(string.Format("%1 slot '%2' does not exist on this prefab (entries added by hand in the panel are not supported).", kind, key));

			return;
		}

		if (slotRef.m_Prefab == desired)
			return;

		if (desired.IsEmpty())
		{
			if (kind == "Worn")
				plan.m_iWornCleared++;
		}
		else
		{
			int sameValueWrites;
			plan.m_mSlotValueCounts.Find(desired, sameValueWrites);
			plan.m_mSlotValueCounts.Set(desired, sameValueWrites + 1);
		}

		string label = string.Format("%1 %2", kind, slotRef.m_sLabel);
		plan.m_aWrites.Insert(new GDC_LoadoutWrite(label, slotRef.m_aPath, slotRef.m_sField, desired));
		plan.m_aChanges.Insert(string.Format("%1: %2 -> %3", label, GDC_LoadoutUtils.ShortName(slotRef.m_Prefab), GDC_LoadoutUtils.ShortName(desired)));
	}

	protected static void PlanEntries(
		notnull GDC_LoadoutSnapshot snapshot,
		notnull array<ref GDC_LoadoutContainer> containers,
		bool removeEmptyEntries,
		notnull GDC_LoadoutPlan plan,
		notnull array<string> outErrors)
	{
		int entryCount = snapshot.m_aEntries.Count();

		// desired state per existing entry, initialised to the current state
		array<string> targets = {};
		array<int> purposes = {};
		array<ref array<ResourceName>> contents = {};
		array<bool> used = {};
		foreach (GDC_LoadoutEntryRef entry : snapshot.m_aEntries)
		{
			targets.Insert(entry.m_sTargetStorage);
			purposes.Insert(entry.m_iTargetPurpose);
			array<ResourceName> current = {};
			current.Copy(entry.m_aPrefabs);
			contents.Insert(current);
			used.Insert(false);
		}

		foreach (GDC_LoadoutContainer container : containers)
		{
			if (!container)
				continue;

			array<int> sources = {};
			foreach (int sourceIndex : container.m_aSourceIndices)
			{
				if (sourceIndex >= 0 && sourceIndex < entryCount && !used[sourceIndex])
				{
					sources.Insert(sourceIndex);
					used[sourceIndex] = true;
				}
			}

			array<ResourceName> desired = {};
			container.CollectPrefabs(desired);

			if (sources.IsEmpty())
			{
				if (desired.IsEmpty())
					continue;

				if (snapshot.m_iStorageManagerIndex < 0)
				{
					outErrors.Insert("The prefab has no SCR_InventoryStorageManagerComponent, inventory items cannot be added.");
					return;
				}

				GDC_LoadoutNewEntry newEntry = new GDC_LoadoutNewEntry();
				newEntry.m_sTargetStorage = container.m_sTargetStorage;
				newEntry.m_iTargetPurpose = container.m_iTargetPurpose;
				newEntry.m_aPrefabs.Copy(desired);
				plan.m_aNewEntries.Insert(newEntry);
				plan.m_aChanges.Insert(string.Format("New entry for '%1': %2 items", DescribeTarget(container.m_sTargetStorage), desired.Count()));
				continue;
			}

			int first = sources[0];
			array<ResourceName> currentUnion = {};
			foreach (int unionIndex : sources)
			{
				currentUnion.InsertAll(snapshot.m_aEntries[unionIndex].m_aPrefabs);
			}

			bool sameTarget = targets[first] == container.m_sTargetStorage;
			bool samePurpose = GDC_LoadoutUtils.SamePurpose(purposes[first], container.m_iTargetPurpose);
			if (sameTarget && samePurpose && GDC_LoadoutUtils.SamePrefabSet(currentUnion, desired))
				continue;

			targets[first] = container.m_sTargetStorage;
			if (!samePurpose)
				purposes[first] = container.m_iTargetPurpose;

			if (!GDC_LoadoutUtils.SamePrefabSet(contents[first], desired))
				contents[first] = desired;

			for (int otherSource = 1; otherSource < sources.Count(); otherSource++)
			{
				contents[sources[otherSource]] = new array<ResourceName>();
			}
		}

		// Entries no container refers to are left untouched: items are only ever removed from a container the user emptied.
		// Only entries that were already empty can be cleaned up below.

		foreach (GDC_LoadoutNewEntry addedEntry : plan.m_aNewEntries)
		{
			plan.m_iItemsAfter += addedEntry.m_aPrefabs.Count();
		}

		for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
		{
			GDC_LoadoutEntryRef original = snapshot.m_aEntries[entryIndex];
			array<ResourceName> finalContent = contents[entryIndex];
			plan.m_iItemsBefore += original.m_aPrefabs.Count();
			plan.m_iItemsAfter += finalContent.Count();
			string label = string.Format("Entry %1 ('%2')", entryIndex, DescribeTarget(original.m_sTargetStorage));

			if (finalContent.IsEmpty() && original.m_bOwn && removeEmptyEntries)
			{
				plan.m_aRemovals.Insert(entryIndex);
				plan.m_aChanges.Insert(string.Format("%1: removed (empty)", label));
				continue;
			}

			array<ref ContainerIdPathEntry> entryPath = {
				new ContainerIdPathEntry(COMPONENTS, snapshot.m_iStorageManagerIndex),
				new ContainerIdPathEntry(INITIAL_ITEMS, entryIndex)
			};

			// the destination of an entry left empty does not matter: it is not rewritten
			if (!finalContent.IsEmpty() && targets[entryIndex] != original.m_sTargetStorage)
			{
				plan.m_aWrites.Insert(new GDC_LoadoutWrite(label, entryPath, TARGET_STORAGE, targets[entryIndex]));
				plan.m_aChanges.Insert(string.Format("%1: destination -> '%2'", label, DescribeTarget(targets[entryIndex])));
			}

			if (!finalContent.IsEmpty() && purposes[entryIndex] != original.m_iTargetPurpose)
				plan.m_aWrites.Insert(new GDC_LoadoutWrite(label, entryPath, TARGET_PURPOSE, purposes[entryIndex].ToString()));

			if (!GDC_LoadoutUtils.SamePrefabOrder(finalContent, original.m_aPrefabs))
			{
				plan.m_aWrites.Insert(new GDC_LoadoutWrite(label, entryPath, PREFABS_TO_SPAWN, GDC_LoadoutUtils.JoinPrefabs(finalContent)));
				plan.m_aChanges.Insert(string.Format("%1: %2 -> %3 items", label, original.m_aPrefabs.Count(), finalContent.Count()));
			}
		}
	}

	protected static string DescribeTarget(string targetStorage)
	{
		if (targetStorage.IsEmpty())
			return "automatic";

		return targetStorage;
	}

	//------------------------------------------------------------------------------------------------
	// Writing
	//------------------------------------------------------------------------------------------------

	//! Applies a validated plan. Must be called inside a World Editor entity action.
	protected static void ReportProgress(WBProgressDialog progress, int step, int stepCount, string action)
	{
		if (!progress || stepCount <= 0)
			return;

		progress.SetMainTitle(string.Format("Applying the loadout (%1 / %2)", step + 1, stepCount));
		progress.SetActionTitle(action);
		float ratio = step;
		ratio /= stepCount;
		progress.SetProgress(ratio);
	}

	static bool Execute(notnull WorldEditorAPI api, notnull IEntitySource prefab, notnull GDC_LoadoutSnapshot snapshot, notnull GDC_LoadoutPlan plan, WBProgressDialog progress = null)
	{
		// each write makes the World Editor recreate the entities using the prefab, so progress is reported per write
		int stepCount = plan.m_aWrites.Count() + plan.m_aNewEntries.Count() + plan.m_aRemovals.Count();
		int step;

		foreach (GDC_LoadoutWrite write : plan.m_aWrites)
		{
			ReportProgress(progress, step++, stepCount, write.m_sLabel);
			if (!api.SetVariableValue(prefab, write.m_aPath, write.m_sField, write.m_sValue))
			{
				Print(string.Format("[GDC LoadoutEditor] could not write %1 of %2", write.m_sField, write.m_sLabel), LogLevel.ERROR);
				return false;
			}
		}

		if (plan.m_aNewEntries.IsEmpty() && plan.m_aRemovals.IsEmpty())
			return true;

		array<ref ContainerIdPathEntry> managerPath = { new ContainerIdPathEntry(COMPONENTS, snapshot.m_iStorageManagerIndex) };
		int nextIndex = snapshot.m_aEntries.Count();
		foreach (GDC_LoadoutNewEntry newEntry : plan.m_aNewEntries)
		{
			ReportProgress(progress, step++, stepCount, "New entry for " + DescribeTarget(newEntry.m_sTargetStorage));
			if (!api.CreateObjectArrayVariableMember(prefab, managerPath, INITIAL_ITEMS, ITEMS_INIT_CLASS, nextIndex))
			{
				Print(string.Format("[GDC LoadoutEditor] could not create an inventory entry for '%1'", DescribeTarget(newEntry.m_sTargetStorage)), LogLevel.ERROR);
				return false;
			}

			array<ref ContainerIdPathEntry> newPath = {
				new ContainerIdPathEntry(COMPONENTS, snapshot.m_iStorageManagerIndex),
				new ContainerIdPathEntry(INITIAL_ITEMS, nextIndex)
			};

			bool written = api.SetVariableValue(prefab, newPath, PREFABS_TO_SPAWN, GDC_LoadoutUtils.JoinPrefabs(newEntry.m_aPrefabs));
			if (written && !newEntry.m_sTargetStorage.IsEmpty())
				written = api.SetVariableValue(prefab, newPath, TARGET_STORAGE, newEntry.m_sTargetStorage);

			if (written && !GDC_LoadoutUtils.IsGeneralPurpose(newEntry.m_iTargetPurpose))
				written = api.SetVariableValue(prefab, newPath, TARGET_PURPOSE, newEntry.m_iTargetPurpose.ToString());

			if (!written)
			{
				Print(string.Format("[GDC LoadoutEditor] could not fill the new inventory entry for '%1'", DescribeTarget(newEntry.m_sTargetStorage)), LogLevel.ERROR);
				return false;
			}

			nextIndex++;
		}

		// highest index first so the remaining indices stay valid
		array<int> removals = {};
		removals.Copy(plan.m_aRemovals);
		removals.Sort(true);
		foreach (int removedIndex : removals)
		{
			ReportProgress(progress, step++, stepCount, string.Format("Removing empty entry %1", removedIndex));

			// the call can report success on an inherited entry that stays in place: check the entry is really gone
			// (same identifier still at this index), and never touch another entry that moved into its place
			int countBefore = CountInitialItems(prefab, snapshot.m_iStorageManagerIndex);
			string nameBefore = GetEntryName(prefab, snapshot.m_iStorageManagerIndex, removedIndex);
			bool removed = api.RemoveObjectArrayVariableMember(prefab, managerPath, INITIAL_ITEMS, removedIndex);
			if (removed)
			{
				bool stillThere = CountInitialItems(prefab, snapshot.m_iStorageManagerIndex) == countBefore
					&& GetEntryName(prefab, snapshot.m_iStorageManagerIndex, removedIndex) == nameBefore;

				if (!stillThere)
					continue;
			}

			array<ref ContainerIdPathEntry> removedPath = {
				new ContainerIdPathEntry(COMPONENTS, snapshot.m_iStorageManagerIndex),
				new ContainerIdPathEntry(INITIAL_ITEMS, removedIndex)
			};

			if (!api.SetVariableValue(prefab, removedPath, PREFABS_TO_SPAWN, string.Empty))
			{
				Print(string.Format("[GDC LoadoutEditor] could not remove or empty inventory entry %1", removedIndex), LogLevel.ERROR);
				return false;
			}

			Print(string.Format("[GDC LoadoutEditor] inventory entry %1 is inherited and cannot be removed, it was emptied instead.", removedIndex), LogLevel.WARNING);
		}

		return true;
	}
}
#endif
