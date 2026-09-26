#ifdef WORKBENCH
class EPE_ItemMeasure
{
	bool m_bKnown;
	float m_fVolume;
	string m_sName;
}

class EPE_ItemMetrics
{
	protected ref map<ResourceName, ref EPE_ItemMeasure> m_Cache = new map<ResourceName, ref EPE_ItemMeasure>();

	EPE_ItemMeasure Get(ResourceName prefab)
	{
		EPE_ItemMeasure measure;
		if (m_Cache.Find(prefab, measure))
			return measure;

		measure = Read(prefab);
		m_Cache.Set(prefab, measure);
		return measure;
	}

	protected EPE_ItemMeasure Read(ResourceName prefab)
	{
		EPE_ItemMeasure measure = new EPE_ItemMeasure();
		measure.m_bKnown = EPE_StoragePathResolver.DescribeItem(prefab, measure.m_fVolume, measure.m_sName);
		return measure;
	}
}

class EPE_InventoryDiagnostics
{
	static void Describe(
		notnull array<ref EPE_InventoryGroup> groups,
		notnull array<ref EPE_LoadoutStorageEntry> storages,
		notnull array<ref EPE_StorageNode> nodes,
		notnull EPE_ItemMetrics metrics,
		notnull array<string> warnings)
	{
		warnings.Clear();

		foreach (EPE_InventoryGroup group : groups)
		{
			if (!group)
				continue;

			DescribeGroup(group, storages, nodes, metrics, warnings);
		}
	}

	protected static void DescribeGroup(
		notnull EPE_InventoryGroup group,
		notnull array<ref EPE_LoadoutStorageEntry> storages,
		notnull array<ref EPE_StorageNode> nodes,
		notnull EPE_ItemMetrics metrics,
		notnull array<string> warnings)
	{
		float capacity = 0;
		float used = 0;
		bool unknownCapacity = false;
		bool unknownVolume = false;
		string overflowing;
		string unresolved;
		string truncated;
		string oversized;
		string gearName;
		int copies = 0;
		string details = "  containers\n";

		foreach (EPE_StorageNode node : nodes)
		{
			if (!node.m_bHasStorage || EPE_InventoryGroups.RootSlot(node.m_sPath) != group.m_sKey)
				continue;

			if (node.m_iDepth == 0 && gearName.IsEmpty())
				gearName = node.m_sDisplayName;

			string line = string.Format("    %1 [purpose %2]", node.m_sPath, node.m_iTargetPurpose);

			if (node.m_bChildrenTruncated)
			{
				truncated += string.Format("\n    %1", node.m_sPath);
				line += " | deeper containers not listed";
			}

			if (!node.IsContainer())
			{
				details += line + " | special purpose\n";
				continue;
			}

			if (node.m_iCompartments > 1)
			{
				int shownLargest = Math.Round(node.m_fLargestCompartment);
				line += string.Format(" | %1 compartments, largest %2", node.m_iCompartments, shownLargest);
			}

			const float occupied = EPE_InventoryGroups.UsedVolume(storages, node, metrics);
			int shownOccupied = Math.Round(occupied);
			if (!node.m_bCapacityKnown)
			{
				unknownCapacity = true;
				details += line + string.Format(" | %1 / unknown\n", shownOccupied);
				continue;
			}

			int shownCapacity = Math.Round(node.m_fMaxVolume);
			details += line + string.Format(" | %1 / %2\n", shownOccupied, shownCapacity);

			if (!HasNestedContainer(node, nodes))
			{
				capacity += node.m_fMaxVolume;
				used += occupied;
			}

			if (occupied > node.m_fMaxVolume)
				overflowing += string.Format("\n    %1: %2 / %3", node.m_sPath, shownOccupied, shownCapacity);
		}

		details += "  items\n";
		foreach (EPE_LoadoutStorageEntry storage : storages)
		{
			if (!storage || EPE_InventoryGroups.RootSlot(storage.m_sTargetStorage) != group.m_sKey)
				continue;

			EPE_StorageNode destination = EPE_InventoryGroups.FindNode(nodes, storage.m_sTargetStorage, storage.m_iTargetPurpose);
			string label = string.Format("%1 [purpose %2]", storage.m_sTargetStorage, storage.m_iTargetPurpose);
			if (storage.m_sTargetStorage.IsEmpty())
				label = string.Format("automatic [purpose %1]", storage.m_iTargetPurpose);

			if (!destination && !storage.m_sTargetStorage.IsEmpty() && !storage.m_aItems.IsEmpty())
			{
				unresolved += string.Format("\n    %1", storage.m_sTargetStorage);
				label += " | no such container, kept";
			}

			details += string.Format("    %1\n", label);
			foreach (EPE_LoadoutItemEntry item : storage.m_aItems)
			{
				if (!item || item.m_Prefab.IsEmpty())
					continue;

				const int count = Math.Max(item.m_iCount, 1);
				copies += count;
				EPE_ItemMeasure measure = metrics.Get(item.m_Prefab);
				string itemName = measure.m_sName;
				if (itemName.IsEmpty())
					itemName = EPE_StoragePathResolver.ReadableFileName(FilePath.StripPath(item.m_Prefab.GetPath()));

				if (!measure.m_bKnown)
					unknownVolume = true;

				if (measure.m_bKnown && destination && destination.RejectsItemVolume(measure.m_fVolume))
				{
					int itemVolume = Math.Round(measure.m_fVolume);
					int largest = Math.Round(destination.m_fLargestCompartment);
					oversized += string.Format("\n    %1 (volume %2) in %3, largest compartment %4",
						itemName, itemVolume, storage.m_sTargetStorage, largest);
					details += string.Format("      %1 x %2 | too large for any compartment\n", count, itemName);
					continue;
				}

				details += string.Format("      %1 x %2\n", count, itemName);
			}
		}

		string heading = EPE_InventoryGroups.GroupName(group.m_sKey);
		if (!gearName.IsEmpty())
			heading += string.Format(" (%1)", gearName);

		group.m_sLabel = heading;
		if (!unresolved.IsEmpty())
			group.m_sLabel += " - NO CONTAINER, items kept";

		int shownUsed = Math.Round(used);
		int shownTotal = Math.Round(capacity);
		string summary = string.Format("%1 | %2 items | volume %3 / %4", heading, copies, shownUsed, shownTotal);

		if (!overflowing.IsEmpty())
			summary += " | OVER CAPACITY - kept";

		if (!unresolved.IsEmpty())
			summary += " | NO CONTAINER - kept";

		if (!oversized.IsEmpty())
			summary += " | ITEM TOO LARGE - kept";

		if (!truncated.IsEmpty())
			summary += " | SOME CONTAINERS NOT LISTED";

		if (unknownCapacity || unknownVolume)
			summary += " | estimate incomplete";

		group.m_sSummary = summary;
		group.m_sDetails = details;

		if (!overflowing.IsEmpty())
		{
			warnings.Insert(string.Format(
				"%1: over capacity. Items and quantities are kept; the game may not spawn what does not fit.%2",
				heading, overflowing));
		}

		if (!unresolved.IsEmpty())
		{
			warnings.Insert(string.Format(
				"%1: these destinations are not on the gear currently worn. Their items are kept as configured.%2",
				heading, unresolved));
		}

		if (!oversized.IsEmpty())
		{
			warnings.Insert(string.Format(
				"%1: these items are larger than any single compartment of their destination. They are kept as configured.%2",
				heading, oversized));
		}

		if (!truncated.IsEmpty())
		{
			warnings.Insert(string.Format(
				"%1: the container tree was cut here, so deeper containers are missing. Items configured for them are kept.%2",
				heading, truncated));
		}

		if (unknownCapacity || unknownVolume)
		{
			warnings.Insert(string.Format(
				"%1: a container capacity or an item volume could not be read, so no fit check was made for those.",
				heading));
		}
	}

	protected static bool HasNestedContainer(notnull EPE_StorageNode node, notnull array<ref EPE_StorageNode> nodes)
	{
		foreach (EPE_StorageNode candidate : nodes)
		{
			if (candidate.IsContainer() && candidate.m_bCapacityKnown && candidate.IsNestedIn(node))
				return true;
		}

		return false;
	}
}
#endif
