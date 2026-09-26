#ifdef WORKBENCH
class EPE_WriteOp
{
	string m_sLabel;
	ref array<ref ContainerIdPathEntry> m_Path;
	ref array<string> m_aFields;
	ref array<string> m_aValues;

	string m_sComponentClass;

	string m_sMemberClass;
	string m_sArrayField;
	int m_iMemberIndex;
	EPE_LoadoutStorageEntry m_Storage;

	void EPE_WriteOp(string label)
	{
		m_sLabel = label;
		m_Path = {};
		m_aFields = {};
		m_aValues = {};
		m_iMemberIndex = -1;
	}

	void Write(string field, string value)
	{
		m_aFields.Insert(field);
		m_aValues.Insert(value);
	}

	void AtComponent(int componentIndex)
	{
		m_Path.Insert(ContainerIdPathEntry("components", componentIndex));
	}

	void AtMember(string arrayField, int memberIndex)
	{
		m_Path.Insert(ContainerIdPathEntry(arrayField, memberIndex));
	}
}

class EPE_PrefabWriter
{
	protected static const string STORAGE_MANAGER = "SCR_InventoryStorageManagerComponent";
	protected static const string WEAPON_SLOT = "CharacterWeaponSlotComponent";
	protected static const string GRENADE_SLOT = "CharacterGrenadeSlotComponent";

	protected static const string SLOTS_FIELD = "Slots";
	protected static const string INITIAL_ITEMS_FIELD = "InitialInventoryItems";
	protected static const string INITIAL_STORAGE_SLOTS_FIELD = "InitialStorageSlots";
	protected static const string COMPONENTS_FIELD = "components";
	protected static const string WEAPON_TEMPLATE_FIELD = "WeaponTemplate";
	protected static const string WEAPON_SLOT_INDEX_FIELD = "WeaponSlotIndex";
	protected static const string WEAPON_SLOT_TYPE_FIELD = "WeaponSlotType";
	protected static const string SAVE_IN_LOADOUT_FIELD = "SaveInLoadout";

	protected static const string ITEMS_INIT_CLASS = "ItemsInitConfigurationItem";

	static bool ApplyLoadout(
		notnull WorldEditorAPI api,
		notnull IEntitySource entitySource,
		notnull EPE_LoadoutDefinition loadout,
		notnull EPE_LoadoutDefinition previousLoadout)
	{
		if (!ValidateStorageSources(entitySource, previousLoadout))
			return false;

		array<ref EPE_WriteOp> plan = {};
		bool needsStorageManager;
		if (!PlanLoadout(entitySource, loadout, previousLoadout, plan, needsStorageManager))
			return false;

		if (plan.IsEmpty() && !needsStorageManager)
		{
			Print("EPE: nothing to apply; the prefab already matches the panel.");
			return true;
		}

		if (needsStorageManager)
		{
			if (!api.CreateComponent(entitySource, STORAGE_MANAGER))
			{
				PrintFormat("EPE_PrefabWriter: cannot create %1", STORAGE_MANAGER, level: LogLevel.ERROR);
				return false;
			}

			plan.Clear();
			bool stillMissing;
			if (!PlanLoadout(entitySource, loadout, previousLoadout, plan, stillMissing) || stillMissing)
			{
				Print("EPE_PrefabWriter: could not address the inventory component after creating it; reload the prefab", LogLevel.ERROR);
				return false;
			}
		}

		return ExecutePlan(api, entitySource, plan);
	}

	protected static bool PlanLoadout(
		notnull IEntitySource entitySource,
		notnull EPE_LoadoutDefinition loadout,
		notnull EPE_LoadoutDefinition previousLoadout,
		notnull array<ref EPE_WriteOp> plan,
		out bool needsStorageManager)
	{
		needsStorageManager = false;

		bool planned = PlanWorn(entitySource, loadout, previousLoadout, plan);

		planned = PlanSlotComponents(entitySource, loadout.m_aWeapons, previousLoadout.m_aWeapons,
			CharacterWeaponSlotComponent, WEAPON_SLOT, "weapon", plan) && planned;

		planned = PlanSlotComponents(entitySource, loadout.m_aGrenades, previousLoadout.m_aGrenades,
			CharacterGrenadeSlotComponent, GRENADE_SLOT, "grenade/throwable", plan) && planned;

		planned = PlanGadgets(entitySource, loadout, previousLoadout, plan) && planned;
		planned = PlanStorages(entitySource, loadout, previousLoadout, plan, needsStorageManager) && planned;
		return planned;
	}

	protected static bool ExecutePlan(
		notnull WorldEditorAPI api,
		notnull IEntitySource entitySource,
		notnull array<ref EPE_WriteOp> plan)
	{
		foreach (EPE_WriteOp op : plan)
		{
			if (!op.m_sComponentClass.IsEmpty())
			{
				IEntityComponentSource created = api.CreateComponent(entitySource, op.m_sComponentClass);
				if (!created)
				{
					PrintFormat("EPE_PrefabWriter: cannot create %1 for %2", op.m_sComponentClass, op.m_sLabel, level: LogLevel.ERROR);
					return false;
				}

				const int createdIndex = FindTopLevelComponentIndex(entitySource, created);
				if (createdIndex < 0)
				{
					PrintFormat("EPE_PrefabWriter: cannot address the new %1 for %2", op.m_sComponentClass, op.m_sLabel, level: LogLevel.ERROR);
					return false;
				}

				op.AtComponent(createdIndex);
			}

			if (!op.m_sMemberClass.IsEmpty())
			{
				if (!api.CreateObjectArrayVariableMember(entitySource, op.m_Path, op.m_sArrayField, op.m_sMemberClass, op.m_iMemberIndex))
				{
					PrintFormat("EPE_PrefabWriter: cannot create an entry for %1", op.m_sLabel, level: LogLevel.ERROR);
					return false;
				}

				if (op.m_Storage)
					op.m_Storage.m_iSourceIndex = op.m_iMemberIndex;

				op.AtMember(op.m_sArrayField, op.m_iMemberIndex);
			}

			for (int i = 0, count = op.m_aFields.Count(); i < count; i++)
			{
				if (api.SetVariableValue(entitySource, op.m_Path, op.m_aFields[i], op.m_aValues[i]))
					continue;

				PrintFormat("EPE_PrefabWriter: could not write %1 of %2", op.m_aFields[i], op.m_sLabel, level: LogLevel.ERROR);
				return false;
			}
		}

		return true;
	}

	protected static bool NeedsWrite(string label, ResourceName desired, ResourceName current, ResourceName previous, out bool conflict)
	{
		conflict = false;
		if (desired == current)
			return false;

		if (current != previous)
		{
			if (desired == previous)
			{
				PrintFormat("EPE: %1 was changed outside the tool and is left as it is now ('%2'); the panel still shows the value it was loaded with.",
					label, current, level: LogLevel.WARNING);
				return false;
			}

			PrintFormat("EPE_PrefabWriter: %1 was changed both here and outside the tool ('%2' vs '%3'); reload the prefab and redo the change",
				label, current, desired, level: LogLevel.ERROR);
			conflict = true;
			return false;
		}

		return true;
	}

	protected static bool PlanGadgets(
		notnull IEntitySource entitySource,
		notnull EPE_LoadoutDefinition loadout,
		notnull EPE_LoadoutDefinition previousLoadout,
		notnull array<ref EPE_WriteOp> plan)
	{
		if (loadout.m_aGadgets.IsEmpty())
			return true;

		bool hasConfiguredGadget;
		foreach (EPE_LoadoutGadgetEntry gadget : loadout.m_aGadgets)
		{
			if (gadget && !gadget.m_Prefab.IsEmpty())
			{
				hasConfiguredGadget = true;
				break;
			}
		}

		IEntityComponentSource inventoryStorage = SCR_BaseContainerTools.FindComponentSource(entitySource, CharacterInventoryStorageComponent);
		if (!inventoryStorage)
		{
			if (!hasConfiguredGadget)
				return true;

			Print("EPE_PrefabWriter: character has no inventory storage for the configured gadget", LogLevel.ERROR);
			return false;
		}

		const int inventoryStorageIndex = FindTopLevelComponentIndex(entitySource, inventoryStorage);
		if (inventoryStorageIndex < 0)
		{
			Print("EPE_PrefabWriter: cannot address character inventory storage", LogLevel.ERROR);
			return false;
		}

		array<int> storageIndices = {};
		array<int> slotIndices = {};
		array<string> slotNames = {};
		array<BaseContainer> slotContainers = {};
		CollectGadgetSlots(inventoryStorage, storageIndices, slotIndices, slotNames, slotContainers);
		if (slotNames.IsEmpty())
		{
			if (!hasConfiguredGadget)
				return true;

			Print("EPE_PrefabWriter: character has no addressable gadget slots", LogLevel.ERROR);
			return false;
		}

		bool planned = true;
		foreach (EPE_LoadoutGadgetEntry gadget : loadout.m_aGadgets)
		{
			if (!gadget || gadget.m_sSlotName.IsEmpty())
				continue;

			const int nameIndex = FindSlotName(slotNames, gadget.m_sSlotName);
			if (nameIndex == -1)
			{
				if (gadget.m_Prefab.IsEmpty())
					continue;

				PrintFormat("EPE_PrefabWriter: gadget slot '%1' cannot be addressed safely on this prefab.",
					gadget.m_sSlotName, level: LogLevel.ERROR);
				planned = false;
				continue;
			}

			ResourceName current = EPE_StoragePathResolver.ResolvePrefab(slotContainers[nameIndex]);
			ResourceName previous = FindGadgetPrefab(previousLoadout, gadget.m_sSlotName);
			bool conflict;
			if (!NeedsWrite(string.Format("gadget slot '%1'", gadget.m_sSlotName), gadget.m_Prefab, current, previous, conflict))
			{
				planned = planned && !conflict;
				continue;
			}

			EPE_WriteOp op = new EPE_WriteOp(string.Format("gadget slot '%1'", gadget.m_sSlotName));
			op.AtComponent(inventoryStorageIndex);
			op.AtComponent(storageIndices[nameIndex]);
			op.AtMember(INITIAL_STORAGE_SLOTS_FIELD, slotIndices[nameIndex]);
			op.Write("Prefab", gadget.m_Prefab);
			plan.Insert(op);
		}

		return planned;
	}

	protected static ResourceName FindGadgetPrefab(notnull EPE_LoadoutDefinition loadout, string slotName)
	{
		foreach (EPE_LoadoutGadgetEntry gadget : loadout.m_aGadgets)
		{
			if (gadget && EPE_LoadoutSlotNames.SameSlot(gadget.m_sSlotName, slotName))
				return gadget.m_Prefab;
		}

		return ResourceName.Empty;
	}

	protected static bool SavedInLoadout(notnull BaseContainer component)
	{
		BaseContainer current = component;
		while (current)
		{
			if (current.IsVariableSetDirectly(SAVE_IN_LOADOUT_FIELD))
			{
				bool saved;
				if (current.Get(SAVE_IN_LOADOUT_FIELD, saved))
					return saved;
			}

			current = current.GetAncestor();
		}

		return true;
	}

	protected static void CollectGadgetSlots(
		notnull BaseContainer inventoryStorage,
		notnull array<int> outStorageIndices,
		notnull array<int> outSlotIndices,
		notnull array<string> outNames,
		notnull array<BaseContainer> outContainers,
		bool loadoutOnly = true)
	{
		BaseContainer current = inventoryStorage;
		while (current)
		{
			BaseContainerList nested = current.GetObjectArray(COMPONENTS_FIELD);
			if (nested && nested.Count() > 0)
			{
				for (int i = 0, count = nested.Count(); i < count; i++)
				{
					BaseContainer component = nested.Get(i);
					if (!component)
						continue;

					if (loadoutOnly && !SavedInLoadout(component))
						continue;

					array<int> slotIndices = {};
					array<string> slotNames = {};
					array<BaseContainer> slotContainers = {};
					CollectNamedIndices(component, INITIAL_STORAGE_SLOTS_FIELD, slotIndices, slotNames, slotContainers);

					for (int s = 0, slotCount = slotNames.Count(); s < slotCount; s++)
					{
						if (outNames.Contains(slotNames[s]))
							continue;

						outStorageIndices.Insert(i);
						outSlotIndices.Insert(slotIndices[s]);
						outNames.Insert(slotNames[s]);
						outContainers.Insert(slotContainers[s]);
					}
				}

				return;
			}

			current = current.GetAncestor();
		}
	}

	protected static int FindTopLevelComponentIndex(
		notnull IEntitySource entitySource,
		notnull BaseContainer target)
	{
		for (int i, count = entitySource.GetComponentCount(); i < count; i++)
		{
			if (entitySource.GetComponent(i) == target)
				return i;
		}

		return -1;
	}

	protected static int FindSlotName(notnull array<string> slotNames, string wanted)
	{
		foreach (int i, string name : slotNames)
		{
			if (EPE_LoadoutSlotNames.SameSlot(name, wanted))
				return i;
		}

		return -1;
	}

	protected static bool PlanWorn(
		notnull IEntitySource entitySource,
		notnull EPE_LoadoutDefinition loadout,
		notnull EPE_LoadoutDefinition previousLoadout,
		notnull array<ref EPE_WriteOp> plan)
	{
		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(entitySource, BaseLoadoutManagerComponent);
		if (!component)
		{
			foreach (EPE_LoadoutWornEntry configured : loadout.m_aWorn)
			{
				if (configured && !configured.m_Prefab.IsEmpty())
				{
					Print("EPE_PrefabWriter: the prefab has no loadout manager for the configured clothing", LogLevel.ERROR);
					return false;
				}
			}

			return true;
		}

		const int componentIndex = FindTopLevelComponentIndex(entitySource, component);
		if (componentIndex < 0)
		{
			Print("EPE_PrefabWriter: cannot address loadout manager", LogLevel.ERROR);
			return false;
		}

		array<int> slotIndices = {};
		array<string> slotNames = {};
		array<BaseContainer> slotContainers = {};
		CollectSlotIndices(component, slotIndices, slotNames, slotContainers);

		bool planned = true;
		foreach (EPE_LoadoutWornEntry worn : loadout.m_aWorn)
		{
			if (!worn || worn.m_sSlotName.IsEmpty())
				continue;

			const int nameIndex = FindSlotName(slotNames, worn.m_sSlotName);
			if (nameIndex == -1)
			{
				if (worn.m_Prefab.IsEmpty())
					continue;

				PrintFormat("EPE_PrefabWriter: worn slot '%1' cannot be addressed safely on this prefab.",
					worn.m_sSlotName, level: LogLevel.ERROR);
				planned = false;
				continue;
			}

			ResourceName current = EPE_StoragePathResolver.ResolvePrefab(slotContainers[nameIndex]);
			EPE_LoadoutWornEntry loaded = previousLoadout.FindWorn(worn.m_sSlotName);
			ResourceName previous;
			if (loaded)
				previous = loaded.m_Prefab;

			bool conflict;
			string label = string.Format("worn slot '%1'", worn.m_sSlotName);
			if (!NeedsWrite(label, worn.m_Prefab, current, previous, conflict))
			{
				planned = planned && !conflict;
				continue;
			}

			EPE_WriteOp op = new EPE_WriteOp(label);
			op.AtComponent(componentIndex);
			op.AtMember(SLOTS_FIELD, slotIndices[nameIndex]);
			op.Write("Prefab", worn.m_Prefab);
			plan.Insert(op);
		}

		return planned;
	}

	protected static void CollectSlotIndices(
		notnull BaseContainer component,
		notnull array<int> outIndices,
		notnull array<string> outNames,
		notnull array<BaseContainer> outContainers)
	{
		CollectNamedIndices(component, SLOTS_FIELD, outIndices, outNames, outContainers);
	}

	protected static void CollectNamedIndices(
		notnull BaseContainer component,
		string fieldName,
		notnull array<int> outIndices,
		notnull array<string> outNames,
		notnull array<BaseContainer> outContainers)
	{
		BaseContainer current = component;
		while (current)
		{
			BaseContainerList slots = current.GetObjectArray(fieldName);
			if (slots)
			{
				for (int i, count = slots.Count(); i < count; i++)
				{
					BaseContainer slot = slots.Get(i);
					if (!slot)
						continue;

					string name = slot.GetName();
					if (name.IsEmpty() || outNames.Contains(name))
						continue;

					outIndices.Insert(i);
					outNames.Insert(name);
					outContainers.Insert(slot);
				}

				return;
			}

			current = current.GetAncestor();
		}
	}

	protected static bool PlanSlotComponents(
		notnull IEntitySource entitySource,
		notnull array<ref EPE_LoadoutWeaponEntry> configured,
		notnull array<ref EPE_LoadoutWeaponEntry> previousEntries,
		typename componentType,
		string componentClass,
		string kind,
		notnull array<ref EPE_WriteOp> plan)
	{
		array<IEntityComponentSource> slotComponents = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, componentType, false, slotComponents);

		bool planned = true;
		foreach (EPE_LoadoutWeaponEntry entry : configured)
		{
			if (!entry)
				continue;

			string label = string.Format("%1 slot %2", kind, entry.m_iSlotIndex);
			ResourceName previous = FindSlotPrefab(previousEntries, entry.m_iSlotIndex);
			IEntityComponentSource target = FindWeaponSlotComponent(slotComponents, entry.m_iSlotIndex);
			if (!target)
			{
				if (entry.m_Prefab.IsEmpty())
					continue;

				EPE_WriteOp created = new EPE_WriteOp(label);
				created.m_sComponentClass = componentClass;
				created.Write(WEAPON_SLOT_INDEX_FIELD, entry.m_iSlotIndex.ToString());
				created.Write(WEAPON_TEMPLATE_FIELD, entry.m_Prefab);
				plan.Insert(created);
				continue;
			}

			const int targetIndex = FindTopLevelComponentIndex(entitySource, target);
			if (targetIndex < 0)
			{
				PrintFormat("EPE_PrefabWriter: cannot address %1", label, level: LogLevel.ERROR);
				planned = false;
				continue;
			}

			ResourceName current = ReadSlotTemplate(target);

			bool conflict;
			if (!NeedsWrite(label, entry.m_Prefab, current, previous, conflict))
			{
				planned = planned && !conflict;
				continue;
			}

			EPE_WriteOp op = new EPE_WriteOp(label);
			op.AtComponent(targetIndex);
			op.Write(WEAPON_TEMPLATE_FIELD, entry.m_Prefab);
			plan.Insert(op);
		}

		return planned;
	}

	protected static ResourceName FindSlotPrefab(notnull array<ref EPE_LoadoutWeaponEntry> entries, int slotIndex)
	{
		foreach (EPE_LoadoutWeaponEntry entry : entries)
		{
			if (entry && entry.m_iSlotIndex == slotIndex)
				return entry.m_Prefab;
		}

		return ResourceName.Empty;
	}

	static int ReadSlotIndex(notnull BaseContainer slotComponent)
	{
		BaseContainer current = slotComponent;
		while (current)
		{
			if (current.IsVariableSetDirectly(WEAPON_SLOT_INDEX_FIELD))
			{
				int authored;
				if (current.Get(WEAPON_SLOT_INDEX_FIELD, authored))
					return authored;
			}

			current = current.GetAncestor();
		}

		return 0;
	}

	static string DescribeWeaponSlots(notnull IEntitySource entitySource)
	{
		string report;
		report += DescribeSlotClass(entitySource, CharacterWeaponSlotComponent, "weapon");
		report += DescribeSlotClass(entitySource, CharacterGrenadeSlotComponent, "grenade");
		report += DescribeSlotClass(entitySource, CharacterHandWeaponSlotComponent, "in-hand");
		if (report.IsEmpty())
			return "    no weapon slot components on this prefab\n";

		return report;
	}

	static string DescribeGadgetSlots(notnull IEntitySource entitySource)
	{
		IEntityComponentSource inventoryStorage = SCR_BaseContainerTools.FindComponentSource(entitySource, CharacterInventoryStorageComponent);
		if (!inventoryStorage)
			return "    no character inventory storage on this prefab\n";

		array<int> storageIndices = {};
		array<int> slotIndices = {};
		array<string> slotNames = {};
		array<BaseContainer> slotContainers = {};
		CollectGadgetSlots(inventoryStorage, storageIndices, slotIndices, slotNames, slotContainers, false);
		if (slotNames.IsEmpty())
			return "    none\n";

		array<int> editableIndices = {};
		array<int> editableSlots = {};
		array<string> editableNames = {};
		array<BaseContainer> editableContainers = {};
		CollectGadgetSlots(inventoryStorage, editableIndices, editableSlots, editableNames, editableContainers);

		string report;
		for (int i = 0, count = slotNames.Count(); i < count; i++)
		{
			ResourceName fitted = EPE_StoragePathResolver.ResolvePrefab(slotContainers[i]);
			string shown = "empty";
			if (!fitted.IsEmpty())
				shown = EPE_StoragePathResolver.ReadableFileName(FilePath.StripPath(fitted.GetPath()));

			string note;
			if (!editableNames.Contains(slotNames[i]))
				note = " | not saved in loadout, hidden from the panel";

			report += string.Format("    %1 | storage %2, slot %3 | %4%5\n",
				slotNames[i], storageIndices[i], slotIndices[i], shown, note);
		}

		return report;
	}

	protected static string DescribeSlotClass(notnull IEntitySource entitySource, typename componentType, string label)
	{
		array<IEntityComponentSource> components = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, componentType, false, components);

		string report;
		foreach (IEntityComponentSource component : components)
		{
			ResourceName template = ReadSlotTemplate(component);
			string shown = "empty";
			if (!template.IsEmpty())
				shown = EPE_StoragePathResolver.ReadableFileName(FilePath.StripPath(template.GetPath()));

			report += string.Format("    slot %1 | %2 | type '%3' | %4\n",
				ReadSlotIndex(component), label, ReadSlotType(component), shown);
		}

		return report;
	}

	protected static string ReadSlotType(notnull BaseContainer slotComponent)
	{
		BaseContainer current = slotComponent;
		while (current)
		{
			string value;
			if (current.Get(WEAPON_SLOT_TYPE_FIELD, value) && !value.IsEmpty())
				return value;

			current = current.GetAncestor();
		}

		return "unset";
	}

	static ResourceName ReadSlotTemplate(notnull BaseContainer slotComponent)
	{
		BaseContainer current = slotComponent;
		while (current)
		{
			if (current.IsVariableSetDirectly(WEAPON_TEMPLATE_FIELD))
			{
				ResourceName authored;
				if (current.Get(WEAPON_TEMPLATE_FIELD, authored))
					return authored;
			}

			current = current.GetAncestor();
		}

		return ResourceName.Empty;
	}

	protected static IEntityComponentSource FindWeaponSlotComponent(notnull array<IEntityComponentSource> weaponSlots, int slotIndex)
	{
		foreach (IEntityComponentSource candidate : weaponSlots)
		{
			if (ReadSlotIndex(candidate) == slotIndex)
				return candidate;
		}

		return null;
	}

	protected static bool PlanStorages(
		notnull IEntitySource entitySource,
		notnull EPE_LoadoutDefinition loadout,
		notnull EPE_LoadoutDefinition previousLoadout,
		notnull array<ref EPE_WriteOp> plan,
		out bool needsStorageManager)
	{
		needsStorageManager = false;

		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(entitySource, SCR_InventoryStorageManagerComponent);
		if (!component)
		{
			if (loadout.m_aStorages.IsEmpty())
				return true;

			needsStorageManager = true;
			return true;
		}

		const int componentIndex = FindTopLevelComponentIndex(entitySource, component);
		if (componentIndex < 0)
		{
			Print("EPE_PrefabWriter: cannot address the inventory storage manager", LogLevel.ERROR);
			return false;
		}

		BaseContainerList entries = component.GetObjectArray(INITIAL_ITEMS_FIELD);
		int writeIndex = 0;
		if (entries)
			writeIndex = entries.Count();

		bool planned = true;
		foreach (EPE_LoadoutStorageEntry storage : loadout.m_aStorages)
		{
			if (!storage)
				continue;

			EPE_LoadoutStorageEntry previous = FindStorageBySource(previousLoadout, storage.m_iSourceIndex);
			string label = string.Format("items for '%1'", storage.m_sDisplayLabel);
			EPE_WriteOp op = new EPE_WriteOp(label);
			op.AtComponent(componentIndex);

			if (storage.m_iSourceIndex < 0)
			{
				op.m_sMemberClass = ITEMS_INIT_CLASS;
				op.m_sArrayField = INITIAL_ITEMS_FIELD;
				op.m_iMemberIndex = writeIndex;
				op.m_Storage = storage;
				writeIndex++;
			}
			else if (!previous)
			{
				Print("EPE_PrefabWriter: unknown source entry; reload the prefab before applying", LogLevel.ERROR);
				planned = false;
				continue;
			}
			else
			{
				op.AtMember(INITIAL_ITEMS_FIELD, storage.m_iSourceIndex);
			}

			if (!previous || previous.m_sTargetStorage != storage.m_sTargetStorage)
				op.Write("TargetStorage", storage.m_sTargetStorage);

			if (!previous || previous.m_iTargetPurpose != storage.m_iTargetPurpose)
				op.Write("TargetPurpose", storage.m_iTargetPurpose.ToString());

			if (!previous || !SameStorageItems(previous, storage))
			{
				array<ResourceName> prefabs = {};
				storage.BuildPrefabsToSpawn(prefabs);
				op.Write("PrefabsToSpawn", JoinResourceNames(prefabs));
			}

			if (!op.m_aFields.IsEmpty() || !op.m_sMemberClass.IsEmpty())
				plan.Insert(op);
		}

		foreach (EPE_LoadoutStorageEntry removed : previousLoadout.m_aStorages)
		{
			if (!removed || removed.m_iSourceIndex < 0 || FindStorageBySource(loadout, removed.m_iSourceIndex))
				continue;

			EPE_WriteOp op = new EPE_WriteOp(string.Format("clearing '%1'", removed.m_sDisplayLabel));
			op.AtComponent(componentIndex);
			op.AtMember(INITIAL_ITEMS_FIELD, removed.m_iSourceIndex);
			op.Write("PrefabsToSpawn", string.Empty);
			plan.Insert(op);
		}

		loadout.m_iStorageEntryCount = writeIndex;
		return planned;
	}

	protected static EPE_LoadoutStorageEntry FindStorageBySource(notnull EPE_LoadoutDefinition loadout, int sourceIndex)
	{
		if (sourceIndex < 0)
			return null;
		foreach (EPE_LoadoutStorageEntry storage : loadout.m_aStorages)
		{
			if (storage && storage.m_iSourceIndex == sourceIndex)
				return storage;
		}
		return null;
	}

	protected static bool SameStorageItems(notnull EPE_LoadoutStorageEntry left, notnull EPE_LoadoutStorageEntry right)
	{
		array<ResourceName> leftItems = {};
		array<ResourceName> rightItems = {};
		left.BuildPrefabsToSpawn(leftItems);
		right.BuildPrefabsToSpawn(rightItems);
		if (leftItems.Count() != rightItems.Count())
			return false;
		for (int i = 0; i < leftItems.Count(); i++)
		{
			if (leftItems[i] != rightItems[i])
				return false;
		}
		return true;
	}

	protected static bool ValidateStorageSources(notnull IEntitySource entitySource, notnull EPE_LoadoutDefinition previousLoadout)
	{
		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(entitySource, SCR_InventoryStorageManagerComponent);
		BaseContainerList entries;
		if (component)
			entries = component.GetObjectArray(INITIAL_ITEMS_FIELD);
		int entryCount = 0;
		if (entries)
			entryCount = entries.Count();
		if (entryCount != previousLoadout.m_iStorageEntryCount)
		{
			Print("EPE: inventory entry count changed outside the tool; reload or undo the external change before applying", LogLevel.ERROR);
			return false;
		}

		foreach (EPE_LoadoutStorageEntry previous : previousLoadout.m_aStorages)
		{
			if (!previous || previous.m_iSourceIndex < 0)
				continue;
			if (!entries || previous.m_iSourceIndex >= entries.Count())
			{
				Print("EPE: inventory changed outside the tool; use Load From Prefab before applying", LogLevel.ERROR);
				return false;
			}

			BaseContainer entry = entries.Get(previous.m_iSourceIndex);
			if (!entry)
				return false;
			string target;
			int purpose;
			array<ResourceName> prefabs = {};
			entry.Get("TargetStorage", target);
			entry.Get("TargetPurpose", purpose);
			entry.Get("PrefabsToSpawn", prefabs);
			EPE_LoadoutStorageEntry current = new EPE_LoadoutStorageEntry();
			foreach (ResourceName prefab : prefabs)
				current.AddOrMergeItem(prefab);

			if (target != previous.m_sTargetStorage || purpose != previous.m_iTargetPurpose || !SameStorageItems(previous, current))
			{
				Print("EPE: inventory changed outside the tool; use Load From Prefab before applying", LogLevel.ERROR);
				return false;
			}
		}
		return true;
	}

	protected static string JoinResourceNames(notnull array<ResourceName> prefabs)
	{
		array<string> parts = {};
		foreach (ResourceName prefab : prefabs)
		{
			parts.Insert(prefab);
		}

		return SCR_StringHelper.Join(",", parts, false);
	}

	static bool ReadLoadout(ResourceName characterPrefab, notnull EPE_LoadoutDefinition loadout)
	{
		Resource resource = Resource.Load(characterPrefab);
		if (!resource || !resource.IsValid())
		{
			PrintFormat("EPE_PrefabWriter: cannot load '%1'", characterPrefab, level: LogLevel.ERROR);
			return false;
		}

		loadout.m_BaseCharacter = characterPrefab;
		loadout.m_aWorn.Clear();
		loadout.m_aWeapons.Clear();
		loadout.m_aGrenades.Clear();
		loadout.m_aGadgets.Clear();
		loadout.m_aStorages.Clear();
		loadout.m_iStorageEntryCount = 0;

		ReadWorn(resource, loadout);
		ReadSlotComponents(resource, CharacterWeaponSlotComponent, loadout.m_aWeapons);
		ReadSlotComponents(resource, CharacterGrenadeSlotComponent, loadout.m_aGrenades);
		ReadGadgets(resource, loadout);
		ReadStorages(resource, loadout);

		return true;
	}

	protected static void ReadGadgets(notnull Resource resource, notnull EPE_LoadoutDefinition loadout)
	{
		IEntityComponentSource inventoryStorage = SCR_BaseContainerTools.FindComponentSource(resource, CharacterInventoryStorageComponent);
		if (!inventoryStorage)
			return;

		array<int> storageIndices = {};
		array<int> slotIndices = {};
		array<string> slotNames = {};
		array<BaseContainer> slotContainers = {};
		CollectGadgetSlots(inventoryStorage, storageIndices, slotIndices, slotNames, slotContainers);

		for (int i = 0, count = slotNames.Count(); i < count; i++)
		{
			loadout.m_aGadgets.Insert(new EPE_LoadoutGadgetEntry(slotNames[i], EPE_StoragePathResolver.ResolvePrefab(slotContainers[i])));
		}
	}

	protected static void ReadWorn(notnull Resource resource, notnull EPE_LoadoutDefinition loadout)
	{
		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(resource, BaseLoadoutManagerComponent);
		if (!component)
			return;

		map<string, BaseContainer> slots = new map<string, BaseContainer>();
		EPE_StoragePathResolver.GatherSlots(component, slots);

		foreach (string slotName, BaseContainer slot : slots)
		{
			loadout.SetWorn(slotName, EPE_StoragePathResolver.ResolvePrefab(slot));
		}
	}

	protected static void ReadSlotComponents(
		notnull Resource resource,
		typename componentType,
		notnull array<ref EPE_LoadoutWeaponEntry> outEntries)
	{
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return;

		array<IEntityComponentSource> slotComponents = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(entitySource, componentType, false, slotComponents);

		foreach (IEntityComponentSource slotComponent : slotComponents)
		{
			ResourceName prefab = ReadSlotTemplate(slotComponent);
			if (prefab.IsEmpty())
				continue;

			outEntries.Insert(new EPE_LoadoutWeaponEntry(prefab, ReadSlotIndex(slotComponent)));
		}
	}

	protected static void ReadStorages(notnull Resource resource, notnull EPE_LoadoutDefinition loadout)
	{
		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(resource, SCR_InventoryStorageManagerComponent);
		if (!component)
			return;

		BaseContainerList entries = component.GetObjectArray(INITIAL_ITEMS_FIELD);
		if (!entries)
			return;

		loadout.m_iStorageEntryCount = entries.Count();
		for (int i = 0, count = entries.Count(); i < count; i++)
		{
			BaseContainer entry = entries.Get(i);
			if (!entry)
				continue;

			string targetStorage;
			entry.Get("TargetStorage", targetStorage);

			array<ResourceName> prefabs = {};
			if (!entry.Get("PrefabsToSpawn", prefabs))
				continue;

			int targetPurpose;
			entry.Get("TargetPurpose", targetPurpose);

			EPE_LoadoutStorageEntry storage = new EPE_LoadoutStorageEntry();
			storage.SetDestination(targetStorage, targetPurpose);
			storage.m_iSourceIndex = i;
			loadout.m_aStorages.Insert(storage);

			foreach (ResourceName prefab : prefabs)
			{
				storage.AddOrMergeItem(prefab);
			}
		}
	}
}
#endif
