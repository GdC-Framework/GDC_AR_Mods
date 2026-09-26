#ifdef WORKBENCH
//! A storage reachable through the worn gear, addressed like vanilla TargetStorage values:
//! "Vest/Vest_ALICE_rifleman.et" for the gear itself, "Vest/Vest_ALICE_rifleman.et/MagPouch/Pouch_ALICE_30rnd_STANAG.et" for an attached pouch.
class GDC_LoadoutStorageNode
{
	string m_sPath;
	string m_sRootSlot;
	string m_sLabel;
	ResourceName m_Prefab;
	int m_iPurpose;
	bool m_bCapacityKnown;
	float m_fCapacity;

	bool IsGeneral()
	{
		return GDC_LoadoutUtils.IsGeneralPurpose(m_iPurpose);
	}

	string GetKey()
	{
		return GDC_LoadoutContainer.MakeKey(m_sPath, m_iPurpose);
	}
}

//! Item volumes read from prefabs, cached for one operation.
class GDC_LoadoutVolumeCache
{
	protected ref map<ResourceName, float> m_mVolumes = new map<ResourceName, float>();

	//! Item volume, or -1 when it cannot be read.
	float Get(ResourceName prefab)
	{
		float volume;
		if (m_mVolumes.Find(prefab, volume))
			return volume;

		volume = Read(prefab);
		m_mVolumes.Set(prefab, volume);
		return volume;
	}

	//! Volume used by the items of a container; unknown volumes count as 0 and set hasUnknown.
	float UsedVolume(notnull GDC_LoadoutContainer container, out bool hasUnknown)
	{
		float used;
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			float volume = Get(item.m_Prefab);
			if (volume < 0)
				hasUnknown = true;
			else
				used += volume * item.m_iCount;
		}

		return used;
	}

	protected float Read(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return -1;

		IEntityComponentSource itemComponent = SCR_BaseContainerTools.FindComponentSource(resource, InventoryItemComponent);
		if (!itemComponent)
			return -1;

		BaseContainer attributes = itemComponent.GetObject("Attributes");
		if (!attributes)
			return -1;

		BaseContainer physAttributes = attributes.GetObject("ItemPhysAttributes");
		if (!physAttributes)
			return -1;

		float volume;
		if (!physAttributes.Get("ItemVolume", volume) || volume <= 0)
			return -1;

		return volume;
	}
}

//! Builds the storage tree of the worn gear and binds inventory entries to it.
class GDC_LoadoutStorageTree
{
	protected static const int MAX_DEPTH = 4;
	protected static const string AUTOMATIC_LABEL = "Automatic placement (the game picks a storage)";

	//------------------------------------------------------------------------------------------------
	// Storage discovery
	//------------------------------------------------------------------------------------------------

	static void CollectNodes(notnull array<ref GDC_LoadoutWornSlot> wornSlots, notnull array<ref GDC_LoadoutStorageNode> outNodes)
	{
		outNodes.Clear();
		foreach (GDC_LoadoutWornSlot worn : wornSlots)
		{
			if (!worn || worn.m_sSlotName.IsEmpty() || worn.m_Prefab.IsEmpty())
				continue;

			string path = string.Format("%1/%2", worn.m_sSlotName, FilePath.StripPath(worn.m_Prefab.GetPath()));
			array<ResourceName> branch = {};
			CollectItem(worn.m_sSlotName, worn.m_sSlotName, worn.m_Prefab, path, 0, branch, outNodes);
		}
	}

	protected static void CollectItem(
		string rootSlot,
		string slotChain,
		ResourceName prefab,
		string path,
		int depth,
		notnull array<ResourceName> branch,
		notnull array<ref GDC_LoadoutStorageNode> outNodes)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("GDC Loadout Editor: cannot load %1", prefab), LogLevel.WARNING);
			return;
		}

		IEntitySource source = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!source)
			return;

		string label = string.Format("%1 (%2)", slotChain, GDC_LoadoutUtils.ShortName(prefab));

		float capacity;
		bool capacityKnown;
		if (ReadGeneralStorage(source, capacity, capacityKnown))
			outNodes.Insert(CreateNode(rootSlot, label, prefab, path, 0, capacity, capacityKnown));

		array<int> purposes = {};
		for (int componentIndex, componentCount = source.GetComponentCount(); componentIndex < componentCount; componentIndex++)
		{
			CollectSpecialPurposes(source.GetComponent(componentIndex), purposes, 0);
		}

		foreach (int purpose : purposes)
		{
			string specialLabel = string.Format("%1 [%2]", label, GDC_LoadoutUtils.PurposeName(purpose));
			outNodes.Insert(CreateNode(rootSlot, specialLabel, prefab, path, purpose, 0, false));
		}

		if (depth >= MAX_DEPTH)
			return;

		IEntityComponentSource cloth = SCR_BaseContainerTools.FindComponentSource(source, BaseLoadoutClothComponent);
		if (!cloth)
			return;

		BaseContainerList slots = cloth.GetObjectArray("Slots");
		if (!slots)
			return;

		branch.Insert(prefab);
		for (int slotIndex, slotCount = slots.Count(); slotIndex < slotCount; slotIndex++)
		{
			BaseContainer slot = slots.Get(slotIndex);
			if (!slot)
				continue;

			ResourceName childPrefab;
			slot.Get("Prefab", childPrefab);
			if (childPrefab.IsEmpty() || branch.Contains(childPrefab))
				continue;

			string childPath = string.Format("%1/%2/%3", path, slot.GetName(), FilePath.StripPath(childPrefab.GetPath()));
			string childChain = string.Format("%1 > %2", slotChain, slot.GetName());
			CollectItem(rootSlot, childChain, childPrefab, childPath, depth + 1, branch, outNodes);
		}

		branch.RemoveItem(prefab);
	}

	protected static GDC_LoadoutStorageNode CreateNode(string rootSlot, string label, ResourceName prefab, string path, int purpose, float capacity, bool capacityKnown)
	{
		GDC_LoadoutStorageNode node = new GDC_LoadoutStorageNode();
		node.m_sRootSlot = rootSlot;
		node.m_sLabel = label;
		node.m_Prefab = prefab;
		node.m_sPath = path;
		node.m_iPurpose = purpose;
		node.m_fCapacity = capacity;
		node.m_bCapacityKnown = capacityKnown;
		return node;
	}

	//! Cloth node storage (vests, pouches) or top-level universal storages (jackets, backpacks).
	protected static bool ReadGeneralStorage(notnull IEntitySource source, out float capacity, out bool capacityKnown)
	{
		capacity = 0;
		capacityKnown = false;

		IEntityComponentSource clothStorage = SCR_BaseContainerTools.FindComponentSource(source, ClothNodeStorageComponent);
		if (clothStorage && IsEnabled(clothStorage))
		{
			float clothVolume;
			if (clothStorage.Get("MaxCumulativeVolume", clothVolume) && clothVolume > 0)
			{
				capacity = clothVolume;
				capacityKnown = true;
			}

			return true;
		}

		bool found;
		bool allKnown = true;
		for (int componentIndex, componentCount = source.GetComponentCount(); componentIndex < componentCount; componentIndex++)
		{
			IEntityComponentSource component = source.GetComponent(componentIndex);
			if (!component)
				continue;

			typename componentType = component.GetClassName().ToType();
			if (!componentType || !componentType.IsInherited(UniversalInventoryStorageComponent) || !IsEnabled(component))
				continue;

			found = true;
			float volume;
			if (component.Get("MaxCumulativeVolume", volume) && volume > 0)
				capacity += volume;
			else
				allKnown = false;
		}

		capacityKnown = found && allKnown;
		return found;
	}

	//! Equipment storages with a dedicated purpose, e.g. the flashlight holder of a jacket.
	protected static void CollectSpecialPurposes(BaseContainer component, notnull array<int> outPurposes, int depth)
	{
		if (!component || depth > 5)
			return;

		typename componentType = component.GetClassName().ToType();
		if (componentType && componentType.IsInherited(SCR_EquipmentStorageComponent) && IsEnabled(component))
		{
			int purpose;
			if (component.Get("StoragePurpose", purpose) && !GDC_LoadoutUtils.IsGeneralPurpose(purpose) && !outPurposes.Contains(purpose))
				outPurposes.Insert(purpose);
		}

		BaseContainerList children = component.GetObjectArray("components");
		if (!children)
			return;

		for (int childIndex, childCount = children.Count(); childIndex < childCount; childIndex++)
		{
			CollectSpecialPurposes(children.Get(childIndex), outPurposes, depth + 1);
		}
	}

	protected static bool IsEnabled(notnull BaseContainer component)
	{
		bool enabled = true;
		if (component.Get("Enabled", enabled))
			return enabled;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Destination matching
	//------------------------------------------------------------------------------------------------

	static bool PurposeFits(notnull GDC_LoadoutStorageNode node, int purpose)
	{
		if (GDC_LoadoutUtils.IsGeneralPurpose(purpose))
			return node.IsGeneral();

		return node.m_iPurpose == purpose;
	}

	static GDC_LoadoutStorageNode FindByPath(notnull array<ref GDC_LoadoutStorageNode> nodes, string path, int purpose)
	{
		if (path.IsEmpty())
			return null;

		foreach (GDC_LoadoutStorageNode node : nodes)
		{
			if (PurposeFits(node, purpose) && GDC_LoadoutUtils.SamePath(node.m_sPath, path))
				return node;
		}

		return null;
	}

	//! True when the TargetStorage value designates this node, in either format.
	static bool IsTargetOf(notnull GDC_LoadoutStorageNode node, string targetStorage)
	{
		if (targetStorage.IsEmpty())
			return false;

		return GDC_LoadoutUtils.SamePath(node.m_sPath, targetStorage) || GDC_LoadoutUtils.SamePath(node.m_Prefab.GetPath(), targetStorage);
	}

	//! Resolves a TargetStorage value: slot path ("Vest/Vest.et/radio/Pouch.et") or prefab path ("Prefabs/.../Pouch.et").
	static GDC_LoadoutStorageNode Match(notnull array<ref GDC_LoadoutStorageNode> nodes, string targetStorage, int purpose)
	{
		GDC_LoadoutStorageNode node = FindByPath(nodes, targetStorage, purpose);
		if (node || targetStorage.IsEmpty())
			return node;

		foreach (GDC_LoadoutStorageNode candidate : nodes)
		{
			if (PurposeFits(candidate, purpose) && GDC_LoadoutUtils.SamePath(candidate.m_Prefab.GetPath(), targetStorage))
				return candidate;
		}

		return null;
	}

	//! Same slot chain on different gear, e.g. "Vest/A.et/radio/X.et" -> "Vest/B.et/radio/Y.et". Only a unique candidate is returned.
	static GDC_LoadoutStorageNode FindReplacement(notnull array<ref GDC_LoadoutStorageNode> nodes, string oldPath, int purpose)
	{
		array<string> oldParts = {};
		oldPath.Split("/", oldParts, true);
		if (oldParts.Count() < 2 || oldParts.Count() % 2 != 0)
			return null;

		GDC_LoadoutStorageNode match;
		foreach (GDC_LoadoutStorageNode node : nodes)
		{
			if (!PurposeFits(node, purpose))
				continue;

			array<string> parts = {};
			node.m_sPath.Split("/", parts, true);
			if (parts.Count() != oldParts.Count())
				continue;

			bool sameChain = true;
			for (int partIndex = 0; partIndex < parts.Count(); partIndex += 2)
			{
				if (!GDC_LoadoutUtils.SamePath(parts[partIndex], oldParts[partIndex]))
					sameChain = false;
			}

			if (!sameChain)
				continue;

			if (match)
				return null;

			match = node;
		}

		return match;
	}

	//------------------------------------------------------------------------------------------------
	// Containers
	//------------------------------------------------------------------------------------------------

	//! One container per storage node, plus automatic placement and destinations not found on the gear.
	static void BuildContainers(
		notnull array<ref GDC_LoadoutEntryRef> entries,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		notnull array<ref GDC_LoadoutContainer> outContainers)
	{
		outContainers.Clear();
		foreach (GDC_LoadoutStorageNode node : nodes)
		{
			outContainers.Insert(CreateNodeContainer(node));
		}

		foreach (GDC_LoadoutEntryRef entry : entries)
		{
			GDC_LoadoutStorageNode entryNode = Match(nodes, entry.m_sTargetStorage, entry.m_iTargetPurpose);
			string key;
			if (entryNode)
				key = entryNode.GetKey();
			else
				key = GDC_LoadoutContainer.MakeKey(entry.m_sTargetStorage, entry.m_iTargetPurpose);

			GDC_LoadoutContainer container = FindContainer(outContainers, key);
			if (!container)
			{
				container = new GDC_LoadoutContainer();
				container.m_sTargetStorage = entry.m_sTargetStorage;
				container.m_iTargetPurpose = entry.m_iTargetPurpose;
				outContainers.Insert(container);
			}

			container.AddSource(entry.m_iIndex, entry.m_sTargetStorage, entry.m_iTargetPurpose);
			foreach (ResourceName prefab : entry.m_aPrefabs)
			{
				container.AddItem(prefab);
			}
		}

		EnsureAutomaticContainer(outContainers);
	}

	//! Rebinds the containers after a gear change. Containers whose storage disappeared keep their items and are flagged.
	static void Remap(notnull array<ref GDC_LoadoutContainer> containers, notnull array<ref GDC_LoadoutStorageNode> nodes)
	{
		array<ref GDC_LoadoutContainer> bound = {};
		array<string> boundKeys = {};
		array<ref GDC_LoadoutContainer> unbound = {};

		foreach (GDC_LoadoutContainer container : containers)
		{
			if (!container)
				continue;

			GDC_LoadoutStorageNode node = ResolveContainer(container, nodes);
			if (!node)
			{
				container.m_sNodePath = string.Empty;
				if (container.IsAutomatic() || container.CountItems() > 0)
					unbound.Insert(container);

				continue;
			}

			container.m_sNodePath = node.m_sPath;
			container.m_iTargetPurpose = node.m_iPurpose;
			if (!IsTargetOf(node, container.m_sTargetStorage))
				container.m_sTargetStorage = node.m_sPath;

			int existing = boundKeys.Find(node.GetKey());
			if (existing < 0)
			{
				bound.Insert(container);
				boundKeys.Insert(node.GetKey());
				continue;
			}

			// two containers now point to the same storage: merge them
			GDC_LoadoutContainer target = bound[existing];
			foreach (GDC_LoadoutItem item : container.m_aItems)
			{
				if (item)
					target.AddItem(item.m_Prefab, item.m_iCount);
			}

			foreach (int sourceIndex : container.m_aSourceIndices)
			{
				target.m_aSourceIndices.Insert(sourceIndex);
			}
		}

		containers.Clear();
		foreach (GDC_LoadoutStorageNode orderedNode : nodes)
		{
			int boundIndex = boundKeys.Find(orderedNode.GetKey());
			if (boundIndex >= 0)
				containers.Insert(bound[boundIndex]);
			else
				containers.Insert(CreateNodeContainer(orderedNode));
		}

		foreach (GDC_LoadoutContainer other : unbound)
		{
			containers.Insert(other);
		}

		EnsureAutomaticContainer(containers);
	}

	protected static GDC_LoadoutStorageNode ResolveContainer(notnull GDC_LoadoutContainer container, notnull array<ref GDC_LoadoutStorageNode> nodes)
	{
		if (!container.m_sNodePath.IsEmpty())
		{
			GDC_LoadoutStorageNode node = FindByPath(nodes, container.m_sNodePath, container.m_iTargetPurpose);
			if (node)
				return node;

			return FindReplacement(nodes, container.m_sNodePath, container.m_iTargetPurpose);
		}

		return Match(nodes, container.m_sTargetStorage, container.m_iTargetPurpose);
	}

	protected static GDC_LoadoutContainer CreateNodeContainer(notnull GDC_LoadoutStorageNode node)
	{
		GDC_LoadoutContainer container = new GDC_LoadoutContainer();
		container.m_sNodePath = node.m_sPath;
		container.m_sTargetStorage = node.m_sPath;
		container.m_iTargetPurpose = node.m_iPurpose;
		return container;
	}

	protected static void EnsureAutomaticContainer(notnull array<ref GDC_LoadoutContainer> containers)
	{
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (container && container.IsAutomatic() && GDC_LoadoutUtils.IsGeneralPurpose(container.m_iTargetPurpose))
				return;
		}

		containers.Insert(new GDC_LoadoutContainer());
	}

	protected static GDC_LoadoutContainer FindContainer(notnull array<ref GDC_LoadoutContainer> containers, string key)
	{
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (container && container.GetKey() == key)
				return container;
		}

		return null;
	}

	//! Refreshes container titles with fill level; adds a warning line per problem.
	static void UpdateLabels(
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		notnull GDC_LoadoutVolumeCache volumes,
		notnull array<string> outWarnings)
	{
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (!container)
				continue;

			GDC_LoadoutStorageNode node = FindByPath(nodes, container.m_sNodePath, container.m_iTargetPurpose);

			// gear found but no storage with this purpose was detected on it: not an error, the game resolves it
			GDC_LoadoutStorageNode gear;
			if (!node && !container.m_sTargetStorage.IsEmpty())
				gear = Match(nodes, container.m_sTargetStorage, 0);

			string label;
			if (node)
				label = node.m_sLabel;
			else if (gear)
				label = gear.m_sLabel;
			else if (container.m_sTargetStorage.IsEmpty())
				label = AUTOMATIC_LABEL;
			else
				label = string.Format("NOT ON CURRENT GEAR: %1", container.m_sTargetStorage);

			if (!node && !GDC_LoadoutUtils.IsGeneralPurpose(container.m_iTargetPurpose))
				label += string.Format(" [%1]", GDC_LoadoutUtils.PurposeName(container.m_iTargetPurpose));

			int itemCount = container.CountItems();
			bool hasUnknown;
			float used = volumes.UsedVolume(container, hasUnknown);
			if (node && node.m_bCapacityKnown)
			{
				int shownUsed = Math.Round(used);
				int shownCapacity = Math.Round(node.m_fCapacity);
				label += string.Format(" | %1 items | volume %2 / %3", itemCount, shownUsed, shownCapacity);
				if (used > node.m_fCapacity)
				{
					label += " | OVER CAPACITY";
					outWarnings.Insert(string.Format("%1: over capacity (%2 / %3), the game may not spawn every item.", node.m_sLabel, shownUsed, shownCapacity));
				}
			}
			else if (itemCount > 0)
			{
				label += string.Format(" | %1 items", itemCount);
			}

			if (hasUnknown && itemCount > 0)
				label += " | some volumes unknown";

			if (!node && !gear && !container.m_sTargetStorage.IsEmpty() && itemCount > 0)
				outWarnings.Insert(string.Format("%1 items target '%2', which is not on the current gear. They are kept as configured.", itemCount, container.m_sTargetStorage));

			container.m_sLabel = label;
		}
	}

	//------------------------------------------------------------------------------------------------
	// Automatic distribution
	//------------------------------------------------------------------------------------------------

	//! Places items into the general storages by volume, following the gear order given in slotOrder ("Vest,Jacket,..."). Items that fit nowhere go to automatic placement.
	static void Distribute(
		notnull array<ref GDC_LoadoutItem> items,
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		string slotOrder,
		notnull GDC_LoadoutVolumeCache volumes,
		notnull array<string> outReport)
	{
		array<GDC_LoadoutContainer> candidates = {};
		array<string> order = {};
		slotOrder.Split(",", order, true);
		foreach (string orderedSlot : order)
		{
			orderedSlot.TrimInPlace();
			AddCandidates(containers, nodes, orderedSlot, candidates);
		}

		AddCandidates(containers, nodes, string.Empty, candidates);

		GDC_LoadoutContainer automatic;
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (container && container.IsAutomatic() && GDC_LoadoutUtils.IsGeneralPurpose(container.m_iTargetPurpose))
				automatic = container;
		}

		foreach (GDC_LoadoutItem item : items)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			float volume = volumes.Get(item.m_Prefab);
			if (volume < 0)
				outReport.Insert(string.Format("%1: volume unknown, counted as 0.", GDC_LoadoutUtils.ShortName(item.m_Prefab)));

			int unplaced;
			for (int copy = 0; copy < item.m_iCount; copy++)
			{
				GDC_LoadoutContainer destination = FindRoom(candidates, nodes, Math.Max(volume, 0), volumes);
				if (!destination)
				{
					destination = automatic;
					unplaced++;
				}

				destination.AddItem(item.m_Prefab);
			}

			if (unplaced > 0)
				outReport.Insert(string.Format("%1: %2 copies did not fit and were put in automatic placement.", GDC_LoadoutUtils.ShortName(item.m_Prefab), unplaced));
		}
	}

	protected static void AddCandidates(
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		string rootSlot,
		notnull array<GDC_LoadoutContainer> candidates)
	{
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (!container || candidates.Contains(container))
				continue;

			GDC_LoadoutStorageNode node = FindByPath(nodes, container.m_sNodePath, container.m_iTargetPurpose);
			if (!node || !node.IsGeneral() || !node.m_bCapacityKnown)
				continue;

			if (!rootSlot.IsEmpty() && !GDC_LoadoutUtils.SamePath(node.m_sRootSlot, rootSlot))
				continue;

			candidates.Insert(container);
		}
	}

	protected static GDC_LoadoutContainer FindRoom(
		notnull array<GDC_LoadoutContainer> candidates,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		float volume,
		notnull GDC_LoadoutVolumeCache volumes)
	{
		foreach (GDC_LoadoutContainer candidate : candidates)
		{
			GDC_LoadoutStorageNode node = FindByPath(nodes, candidate.m_sNodePath, candidate.m_iTargetPurpose);
			bool hasUnknown;
			if (volumes.UsedVolume(candidate, hasUnknown) + volume <= node.m_fCapacity)
				return candidate;
		}

		return null;
	}
}
#endif
