#ifdef WORKBENCH
[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_Prefab", true)]
class EPE_GroupItem
{
	[Attribute(uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Item")]
	ResourceName m_Prefab;

	[Attribute(defvalue: "1", params: "0 inf 1", desc: "Total copies in this equipment group (0 removes the item)")]
	int m_iCount;

	[Attribute(uiwidget: UIWidgets.None)]
	string m_sPinnedPath;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.None)]
	int m_iPinnedPurpose;
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sLabel")]
class EPE_InventoryGroup
{
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sKey;

	[Attribute(uiwidget: UIWidgets.None)]
	string m_sLabel;

	[Attribute(desc: "Combined items in this equipment group")]
	ref array<ref EPE_GroupItem> m_aItems;

	[Attribute(uiwidget: UIWidgets.None)]
	ref array<ref EPE_LoadoutStorageEntry> m_aSources;

	string m_sSummary;
	string m_sDetails;

	void EPE_InventoryGroup()
	{
		if (!m_aItems)
			m_aItems = {};

		if (!m_aSources)
			m_aSources = {};
	}
}

class EPE_ItemDemand
{
	ResourceName m_Prefab;
	int m_iRemaining;
	string m_sPin;
	int m_iPurpose;
}

class EPE_InventoryGroups
{
	static string RootSlot(string path)
	{
		if (path.IsEmpty())
			return "@automatic";

		array<string> parts = {};
		path.Split("/", parts, true);
		if (parts.Count() < 2 || parts[0] == "Prefabs")
			return "@unresolved";

		return parts[0];
	}

	static string GroupName(string key)
	{
		switch (key)
		{
			case EPE_LoadoutSlotNames.VEST: return "Load-bearing vest";
			case EPE_LoadoutSlotNames.ARMORED_VEST: return "Body armour";
			case EPE_LoadoutSlotNames.BACK: return "Backpack";
			case EPE_LoadoutSlotNames.JACKET: return "Jacket";
			case EPE_LoadoutSlotNames.PANTS: return "Trousers";
			case EPE_LoadoutSlotNames.HAT: return "Headgear";
			case EPE_LoadoutSlotNames.HANDWEAR: return "Gloves";
			case EPE_LoadoutSlotNames.BOOTS: return "Footwear";
			case "@automatic": return "Automatic placement";
			case "@unresolved": return "Unresolved destinations";
		}

		return key;
	}

	static bool PurposesMatch(int configured, int available)
	{
		if (configured == available)
			return true;

		return configured <= EStoragePurpose.PURPOSE_DEPOSIT && available <= EStoragePurpose.PURPOSE_DEPOSIT;
	}

	static EPE_StorageNode FindNode(notnull array<ref EPE_StorageNode> nodes, string path, int purpose)
	{
		foreach (EPE_StorageNode node : nodes)
		{
			if (node.m_bHasStorage && node.m_sPath == path && PurposesMatch(purpose, node.m_iTargetPurpose))
				return node;
		}

		return null;
	}

	static string ReplacementPath(string path, int purpose, notnull array<ref EPE_StorageNode> nodes)
	{
		if (path.IsEmpty() || FindNode(nodes, path, purpose))
			return path;

		array<string> original = {};
		path.Split("/", original, true);
		if (original.Count() < 2 || original[0] == "Prefabs" || original.Count() % 2 != 0)
			return path;

		string match;
		foreach (EPE_StorageNode node : nodes)
		{
			if (!node.m_bHasStorage || !PurposesMatch(purpose, node.m_iTargetPurpose))
				continue;

			array<string> candidate = {};
			node.m_sPath.Split("/", candidate, true);
			if (candidate.Count() != original.Count())
				continue;

			bool same = true;
			for (int i = 0; i < original.Count(); i += 2)
			{
				if (!EPE_LoadoutSlotNames.SameSlot(original[i], candidate[i]))
					same = false;
			}

			if (!same)
				continue;

			if (!match.IsEmpty())
				return path;

			match = node.m_sPath;
		}

		if (match.IsEmpty())
			return path;

		return match;
	}

	static bool IsGearSlot(string key)
	{
		foreach (string slot : EPE_LoadoutSlotNames.GetAll())
		{
			if (EPE_LoadoutSlotNames.SameSlot(slot, key))
				return true;
		}

		return false;
	}

	static bool IsGearRemoved(string key, notnull array<string> wornSlots)
	{
		if (!IsGearSlot(key))
			return false;

		foreach (string worn : wornSlots)
		{
			if (EPE_LoadoutSlotNames.SameSlot(worn, key))
				return false;
		}

		return true;
	}

	static void DropRemovedGear(notnull array<ref EPE_InventoryGroup> groups, notnull array<string> wornSlots)
	{
		for (int i = groups.Count() - 1; i >= 0; i--)
		{
			EPE_InventoryGroup group = groups[i];
			if (group && IsGearRemoved(group.m_sKey, wornSlots))
				groups.Remove(i);
		}
	}

	static EPE_InventoryGroup EnsureGroup(notnull array<ref EPE_InventoryGroup> groups, string key)
	{
		foreach (EPE_InventoryGroup group : groups)
		{
			if (group && group.m_sKey == key)
				return group;
		}

		EPE_InventoryGroup created = new EPE_InventoryGroup();
		created.m_sKey = key;
		created.m_sLabel = GroupName(key);
		groups.Insert(created);
		return created;
	}

	static void AddSummaryItem(notnull EPE_InventoryGroup group, ResourceName prefab, int count)
	{
		foreach (EPE_GroupItem item : group.m_aItems)
		{
			if (item.m_Prefab != prefab)
				continue;

			item.m_iCount += count;
			return;
		}

		EPE_GroupItem added = new EPE_GroupItem();
		added.m_Prefab = prefab;
		added.m_iCount = count;
		added.m_iPinnedPurpose = EStoragePurpose.PURPOSE_DEPOSIT;
		group.m_aItems.Insert(added);
	}

	static EPE_LoadoutStorageEntry CopyOf(notnull EPE_LoadoutStorageEntry source, bool withItems)
	{
		EPE_LoadoutStorageEntry copy = new EPE_LoadoutStorageEntry();
		copy.m_iSourceIndex = source.m_iSourceIndex;
		copy.SetDestination(source.m_sTargetStorage, source.m_iTargetPurpose);
		if (!withItems)
			return copy;

		foreach (EPE_LoadoutItemEntry item : source.m_aItems)
		{
			if (item && !item.m_Prefab.IsEmpty())
				copy.m_aItems.Insert(new EPE_LoadoutItemEntry(item.m_Prefab, Math.Max(item.m_iCount, 1)));
		}

		return copy;
	}

	static void Build(
		notnull array<ref EPE_LoadoutStorageEntry> storages,
		notnull array<ref EPE_StorageNode> nodes,
		notnull array<ref EPE_InventoryGroup> groups)
	{
		groups.Clear();

		foreach (string root : EPE_LoadoutSlotNames.GetAll())
		{
			foreach (EPE_StorageNode candidate : nodes)
			{
				if (candidate.m_bHasStorage && RootSlot(candidate.m_sPath) == root)
				{
					EnsureGroup(groups, root);
					break;
				}
			}
		}

		foreach (EPE_StorageNode node : nodes)
		{
			if (node.m_bHasStorage)
				EnsureGroup(groups, RootSlot(node.m_sPath));
		}

		foreach (EPE_LoadoutStorageEntry storage : storages)
		{
			if (!storage)
				continue;

			EPE_InventoryGroup group = EnsureGroup(groups, RootSlot(storage.m_sTargetStorage));
			group.m_aSources.Insert(CopyOf(storage, true));

			foreach (EPE_LoadoutItemEntry item : storage.m_aItems)
			{
				if (item && !item.m_Prefab.IsEmpty())
					AddSummaryItem(group, item.m_Prefab, Math.Max(item.m_iCount, 1));
			}
		}
	}

	static void Remap(notnull array<ref EPE_InventoryGroup> groups, notnull array<ref EPE_StorageNode> nodes)
	{
		array<ref EPE_LoadoutStorageEntry> remapped = {};
		foreach (EPE_InventoryGroup group : groups)
		{
			if (!group)
				continue;

			foreach (EPE_LoadoutStorageEntry source : group.m_aSources)
			{
				if (!source)
					continue;

				source.SetDestination(
					ReplacementPath(source.m_sTargetStorage, source.m_iTargetPurpose, nodes),
					source.m_iTargetPurpose);
				remapped.Insert(source);
			}
		}

		array<ref EPE_InventoryGroup> rebuilt = {};
		Build(remapped, nodes, rebuilt);

		foreach (EPE_InventoryGroup previous : groups)
		{
			if (!previous)
				continue;

			EPE_InventoryGroup target = EnsureGroup(rebuilt, previous.m_sKey);
			target.m_aItems.Clear();
			foreach (EPE_GroupItem item : previous.m_aItems)
			{
				if (item && !item.m_Prefab.IsEmpty())
					CarryOverItem(target, item, nodes);
			}
		}

		groups.Clear();
		foreach (EPE_InventoryGroup result : rebuilt)
		{
			groups.Insert(result);
		}
	}

	protected static void CarryOverItem(
		notnull EPE_InventoryGroup target,
		notnull EPE_GroupItem item,
		notnull array<ref EPE_StorageNode> nodes)
	{
		string pin = item.m_sPinnedPath;
		if (!pin.IsEmpty())
			pin = ReplacementPath(pin, item.m_iPinnedPurpose, nodes);

		foreach (EPE_GroupItem existing : target.m_aItems)
		{
			if (existing.m_Prefab != item.m_Prefab || existing.m_sPinnedPath != pin)
				continue;

			existing.m_iCount += item.m_iCount;
			return;
		}

		EPE_GroupItem added = new EPE_GroupItem();
		added.m_Prefab = item.m_Prefab;
		added.m_iCount = item.m_iCount;
		added.m_sPinnedPath = pin;
		added.m_iPinnedPurpose = item.m_iPinnedPurpose;
		target.m_aItems.Insert(added);
	}

	static array<ref EPE_ItemDemand> Demands(notnull EPE_InventoryGroup group)
	{
		array<ref EPE_ItemDemand> demands = {};
		foreach (EPE_GroupItem item : group.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			EPE_ItemDemand demand = new EPE_ItemDemand();
			demand.m_Prefab = item.m_Prefab;
			demand.m_iRemaining = item.m_iCount;
			demand.m_sPin = item.m_sPinnedPath;
			demand.m_iPurpose = item.m_iPinnedPurpose;
			demands.Insert(demand);
		}

		return demands;
	}

	static EPE_LoadoutStorageEntry Destination(notnull array<ref EPE_LoadoutStorageEntry> output, string path, int purpose)
	{
		foreach (EPE_LoadoutStorageEntry storage : output)
		{
			if (storage.m_sTargetStorage == path && storage.m_iTargetPurpose == purpose)
				return storage;
		}

		EPE_LoadoutStorageEntry created = new EPE_LoadoutStorageEntry();
		created.m_iSourceIndex = -1;
		created.SetDestination(path, purpose);
		output.Insert(created);
		return created;
	}

	static float UsedVolume(
		notnull array<ref EPE_LoadoutStorageEntry> storages,
		notnull EPE_StorageNode node,
		notnull EPE_ItemMetrics metrics)
	{
		float used = 0;
		foreach (EPE_LoadoutStorageEntry storage : storages)
		{
			if (storage.m_sTargetStorage != node.m_sPath || !PurposesMatch(storage.m_iTargetPurpose, node.m_iTargetPurpose))
				continue;

			foreach (EPE_LoadoutItemEntry item : storage.m_aItems)
			{
				EPE_ItemMeasure measure = metrics.Get(item.m_Prefab);
				if (measure.m_bKnown)
					used += measure.m_fVolume * Math.Max(item.m_iCount, 1);
			}
		}

		return used;
	}

	static void Convert(
		notnull array<ref EPE_InventoryGroup> groups,
		notnull array<ref EPE_StorageNode> nodes,
		notnull EPE_ItemMetrics metrics,
		notnull array<ref EPE_LoadoutStorageEntry> output,
		bool redistribute = false)
	{
		output.Clear();
		array<ref EPE_ItemDemand> allDemands = {};
		array<string> demandGroups = {};
		array<string> fallbacks = {};
		array<int> fallbackPurposes = {};
		array<int> sourceIds = {};

		foreach (EPE_InventoryGroup group : groups)
		{
			if (!group)
				continue;

			array<ref EPE_ItemDemand> demands = Demands(group);
			foreach (EPE_LoadoutStorageEntry source : group.m_aSources)
			{
				if (!source)
					continue;

				EPE_LoadoutStorageEntry copy = CopyOf(source, false);
				if (sourceIds.Contains(copy.m_iSourceIndex))
					copy.m_iSourceIndex = -1;
				else if (copy.m_iSourceIndex >= 0)
					sourceIds.Insert(copy.m_iSourceIndex);

				output.Insert(copy);

				foreach (EPE_LoadoutItemEntry original : source.m_aItems)
				{
					int available = Math.Max(original.m_iCount, 1);
					for (int pinPass = 0; pinPass < 2; pinPass++)
					{
						foreach (EPE_ItemDemand keep : demands)
						{
							if (available == 0 || keep.m_iRemaining == 0 || keep.m_Prefab != original.m_Prefab)
								continue;

							bool pinned = !keep.m_sPin.IsEmpty();
							if ((pinPass == 0) != pinned)
								continue;

							if (pinned && (keep.m_sPin != source.m_sTargetStorage || keep.m_iPurpose != source.m_iTargetPurpose))
								continue;

							if (!pinned && redistribute && FindNode(nodes, source.m_sTargetStorage, source.m_iTargetPurpose))
								continue;

							int retained = Math.Min(available, keep.m_iRemaining);
							copy.AddOrMergeItem(original.m_Prefab, retained);
							available -= retained;
							keep.m_iRemaining -= retained;
						}
					}
				}
			}

			string fallback;
			int fallbackPurpose = EStoragePurpose.PURPOSE_DEPOSIT;
			foreach (EPE_LoadoutStorageEntry home : group.m_aSources)
			{
				if (!home || home.m_sTargetStorage.IsEmpty())
					continue;

				if (fallback.IsEmpty() || (!PurposesMatch(fallbackPurpose, EStoragePurpose.PURPOSE_DEPOSIT)
					&& PurposesMatch(home.m_iTargetPurpose, EStoragePurpose.PURPOSE_DEPOSIT)))
				{
					fallback = home.m_sTargetStorage;
					fallbackPurpose = home.m_iTargetPurpose;
				}
			}

			foreach (EPE_ItemDemand pending : demands)
			{
				allDemands.Insert(pending);
				demandGroups.Insert(group.m_sKey);
				fallbacks.Insert(fallback);
				fallbackPurposes.Insert(fallbackPurpose);
			}
		}

		foreach (int demandIndex, EPE_ItemDemand demand : allDemands)
		{
			if (demand.m_iRemaining <= 0)
				continue;

			if (!demand.m_sPin.IsEmpty())
			{
				Destination(output, demand.m_sPin, demand.m_iPurpose).AddOrMergeItem(demand.m_Prefab, demand.m_iRemaining);
				continue;
			}

			EPE_ItemMeasure unit = metrics.Get(demand.m_Prefab);
			EPE_StorageNode fallbackNode;
			foreach (EPE_StorageNode node : nodes)
			{
				if (!node.IsContainer() || RootSlot(node.m_sPath) != demandGroups[demandIndex])
					continue;

				if (!fallbackNode)
					fallbackNode = node;

				if (!node.m_bCapacityKnown || !unit.m_bKnown)
					continue;

				if (node.RejectsItemVolume(unit.m_fVolume))
					continue;

				int fits = demand.m_iRemaining;
				if (unit.m_fVolume > 0)
				{
					int room = Math.Floor((node.m_fMaxVolume - UsedVolume(output, node, metrics)) / unit.m_fVolume);
					if (room < fits)
						fits = room;
				}

				if (fits <= 0)
					continue;

				Destination(output, node.m_sPath, node.m_iTargetPurpose).AddOrMergeItem(demand.m_Prefab, fits);
				demand.m_iRemaining -= fits;
				if (demand.m_iRemaining == 0)
					break;
			}

			if (demand.m_iRemaining <= 0)
				continue;

			string target = fallbacks[demandIndex];
			int purpose = fallbackPurposes[demandIndex];
			if (fallbackNode)
			{
				target = fallbackNode.m_sPath;
				purpose = fallbackNode.m_iTargetPurpose;
			}

			Destination(output, target, purpose).AddOrMergeItem(demand.m_Prefab, demand.m_iRemaining);
		}
	}
}
#endif
