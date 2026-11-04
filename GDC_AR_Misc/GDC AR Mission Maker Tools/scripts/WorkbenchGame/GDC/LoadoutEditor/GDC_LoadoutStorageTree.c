#ifdef WORKBENCH
//! A storage reachable through the worn gear, addressed like vanilla TargetStorage values:
//! "Vest/Vest_ALICE_rifleman.et" for the gear itself, "Vest/Vest_ALICE_rifleman.et/MagPouch/Pouch_ALICE_30rnd_STANAG.et" for an attached pouch.
class GDC_LoadoutStorageNode
{
	string m_sPath;

	//! Path of the gear this storage is attached to, empty for worn gear.
	string m_sParentPath;
	string m_sRootSlot;
	string m_sLabel;
	ResourceName m_Prefab;
	int m_iPurpose;

	//! Volume capacity of a regular storage.
	bool m_bCapacityKnown;
	float m_fCapacity;

	//! Weight limit of a regular storage, 0 when there is none.
	float m_fMaxWeight;

	//! Slot-based storage (equipment pouches): one item per slot, and only the accepted item types.
	bool m_bSlotBased;

	//! Accepted item types (ECommonItemType names) per slot; an empty list accepts every type.
	ref array<ref array<string>> m_aSlotTypes = {};

	//! MaxItemSize of each storage component; an item must fit in one of them. Empty when not defined.
	ref array<vector> m_aMaxItemSizes = {};

	bool IsGeneral()
	{
		return GDC_LoadoutUtils.IsGeneralPurpose(m_iPurpose);
	}

	//! False only when both the item dimensions and the size limits are known and the item fits in none of them.
	bool AcceptsSize(notnull GDC_LoadoutItemInfo info)
	{
		if (!info.m_bDimensionsKnown || m_aMaxItemSizes.IsEmpty())
			return true;

		foreach (vector maxSize : m_aMaxItemSizes)
		{
			if (GDC_LoadoutUtils.FitsIn(info.m_vDimensions, maxSize))
				return true;
		}

		return false;
	}

	string GetKey()
	{
		return GDC_LoadoutContainer.MakeKey(m_sPath, m_iPurpose);
	}

	//! Places item copies (given by their item types) into the slots.
	//! Returns how many fit; types that no slot accepts at all are added to outRefused.
	int FitSlots(notnull array<string> itemTypes, notnull array<string> outRefused)
	{
		// items accepted by fewer slots are placed first, so a generic slot is not taken by an item that had other choices
		array<string> ordered = {};
		for (int choices = 0; choices <= m_aSlotTypes.Count(); choices++)
		{
			foreach (string candidateType : itemTypes)
			{
				if (CountAcceptingSlots(candidateType) == choices)
					ordered.Insert(candidateType);
			}
		}

		array<bool> used = {};
		used.Resize(m_aSlotTypes.Count());

		int fitted;
		foreach (string itemType : ordered)
		{
			if (CountAcceptingSlots(itemType) == 0)
			{
				if (!outRefused.Contains(itemType))
					outRefused.Insert(itemType);

				continue;
			}

			for (int slotIndex = 0; slotIndex < m_aSlotTypes.Count(); slotIndex++)
			{
				if (used[slotIndex] || !SlotAccepts(slotIndex, itemType))
					continue;

				used[slotIndex] = true;
				fitted++;
				break;
			}
		}

		return fitted;
	}

	protected int CountAcceptingSlots(string itemType)
	{
		int count;
		for (int slotIndex = 0; slotIndex < m_aSlotTypes.Count(); slotIndex++)
		{
			if (SlotAccepts(slotIndex, itemType))
				count++;
		}

		return count;
	}

	protected bool SlotAccepts(int slotIndex, string itemType)
	{
		array<string> accepted = m_aSlotTypes[slotIndex];
		return accepted.IsEmpty() || accepted.Contains(itemType);
	}
}

//! Physical data of an item prefab.
class GDC_LoadoutItemInfo
{
	//! -1 when unknown
	float m_fVolume = -1;
	float m_fWeight = -1;

	//! ECommonItemType name, empty when unknown
	string m_sType;

	//! ItemDimensions, only known when set in the prefab
	vector m_vDimensions;
	bool m_bDimensionsKnown;
}

//! Item data read from prefabs, cached for one operation.
class GDC_LoadoutItemCache
{
	protected ref map<ResourceName, ref GDC_LoadoutItemInfo> m_mInfos = new map<ResourceName, ref GDC_LoadoutItemInfo>();

	GDC_LoadoutItemInfo Get(ResourceName prefab)
	{
		GDC_LoadoutItemInfo info;
		if (m_mInfos.Find(prefab, info))
			return info;

		info = Read(prefab);
		m_mInfos.Set(prefab, info);
		return info;
	}

	//! Volume used by the items of a container; unknown volumes count as 0 and set hasUnknown.
	float UsedVolume(notnull GDC_LoadoutContainer container, out bool hasUnknown)
	{
		float used;
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			float volume = Get(item.m_Prefab).m_fVolume;
			if (volume < 0)
				hasUnknown = true;
			else
				used += volume * item.m_iCount;
		}

		return used;
	}

	//! Weight of the items of a container; unknown weights count as 0.
	float UsedWeight(notnull GDC_LoadoutContainer container)
	{
		float used;
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			float weight = Get(item.m_Prefab).m_fWeight;
			if (weight > 0)
				used += weight * item.m_iCount;
		}

		return used;
	}

	//! Item type of every copy in the container, as used by FitSlots.
	void CollectTypes(notnull GDC_LoadoutContainer container, notnull array<string> outTypes)
	{
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			string itemType = Get(item.m_Prefab).m_sType;
			for (int copy = 0; copy < item.m_iCount; copy++)
			{
				outTypes.Insert(itemType);
			}
		}
	}

	//! Display name of the first item of the container having this type.
	string FindItemName(notnull GDC_LoadoutContainer container, string itemType)
	{
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (item && !item.m_Prefab.IsEmpty() && Get(item.m_Prefab).m_sType == itemType)
				return GDC_LoadoutUtils.ShortName(item.m_Prefab);
		}

		return itemType;
	}

	protected GDC_LoadoutItemInfo Read(ResourceName prefab)
	{
		GDC_LoadoutItemInfo info = new GDC_LoadoutItemInfo();
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return info;

		IEntityComponentSource itemComponent = SCR_BaseContainerTools.FindComponentSource(resource, InventoryItemComponent);
		if (!itemComponent)
			return info;

		BaseContainer attributes = itemComponent.GetObject("Attributes");
		if (!attributes)
			return info;

		info.m_sType = GDC_LoadoutStorageTree.ReadCommonType(attributes);

		BaseContainer physAttributes = attributes.GetObject("ItemPhysAttributes");
		if (!physAttributes)
			return info;

		float volume;
		if (physAttributes.Get("ItemVolume", volume) && volume > 0)
			info.m_fVolume = volume;

		float weight;
		if (physAttributes.Get("Weight", weight) && weight > 0)
			info.m_fWeight = weight;

		vector dimensions;
		if (physAttributes.Get("ItemDimensions", dimensions) && dimensions[0] > 0 && dimensions[1] > 0 && dimensions[2] > 0)
		{
			info.m_vDimensions = dimensions;
			info.m_bDimensionsKnown = true;
		}

		return info;
	}
}

//! Builds the storage tree of the worn gear and binds inventory entries to it.
class GDC_LoadoutStorageTree
{
	protected static const int MAX_DEPTH = 4;
	protected static const string AUTOMATIC_LABEL = "AUTOMATIC PLACEMENT";

	//! Alert icons moved to the start of container titles; prepended in this order, so the last one ends up first.
	protected static array<string> GetAlertIcons()
	{
		return { GDC_LoadoutUtils.ICON_WARNING, GDC_LoadoutUtils.ICON_OVER_LIMIT };
	}

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
			CollectItem(worn.m_sSlotName, GDC_LoadoutUtils.Upper(worn.m_sSlotName), worn.m_Prefab, path, string.Empty, 0, branch, outNodes);
		}
	}

	protected static void CollectItem(
		string rootSlot,
		string slotChain,
		ResourceName prefab,
		string path,
		string parentPath,
		int depth,
		notnull array<ResourceName> branch,
		notnull array<ref GDC_LoadoutStorageNode> outNodes)
	{
		// prefabs that cannot be loaded are reported once by the tool when the loadout is loaded
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		IEntitySource source = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!source)
			return;

		string label = string.Format("%1 (%2)", slotChain, GDC_LoadoutUtils.ShortName(prefab));

		GDC_LoadoutStorageNode general = CreateNode(rootSlot, label, prefab, path, 0);
		general.m_sParentPath = parentPath;
		if (ReadGeneralStorage(source, general))
			outNodes.Insert(general);

		array<int> purposes = {};
		for (int componentIndex, componentCount = source.GetComponentCount(); componentIndex < componentCount; componentIndex++)
		{
			CollectSpecialPurposes(source.GetComponent(componentIndex), purposes, 0);
		}

		// the purpose is shown in the details part of the container title
		foreach (int purpose : purposes)
		{
			GDC_LoadoutStorageNode special = CreateNode(rootSlot, label, prefab, path, purpose);
			special.m_sParentPath = parentPath;
			outNodes.Insert(special);
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
			CollectItem(rootSlot, childChain, childPrefab, childPath, path, depth + 1, branch, outNodes);
		}

		branch.RemoveItem(prefab);
	}

	protected static GDC_LoadoutStorageNode CreateNode(string rootSlot, string label, ResourceName prefab, string path, int purpose)
	{
		GDC_LoadoutStorageNode node = new GDC_LoadoutStorageNode();
		node.m_sRootSlot = rootSlot;
		node.m_sLabel = label;
		node.m_Prefab = prefab;
		node.m_sPath = path;
		node.m_iPurpose = purpose;
		return node;
	}

	//! Regular storage of an item: cloth node storage (vests), top-level universal storages (jackets, backpacks)
	//! with their volume and weight limits, or top-level equipment storages (pouches) described by their slots.
	protected static bool ReadGeneralStorage(notnull IEntitySource source, notnull GDC_LoadoutStorageNode node)
	{
		IEntityComponentSource clothStorage = SCR_BaseContainerTools.FindComponentSource(source, ClothNodeStorageComponent);
		if (clothStorage && IsEnabled(clothStorage))
		{
			float clothVolume;
			if (clothStorage.Get("MaxCumulativeVolume", clothVolume) && clothVolume > 0)
			{
				node.m_fCapacity = clothVolume;
				node.m_bCapacityKnown = true;
			}

			AddMaxItemSize(clothStorage, node);
			return true;
		}

		bool volumeFound;
		bool allKnown = true;
		array<IEntityComponentSource> equipmentStorages = {};
		for (int componentIndex, componentCount = source.GetComponentCount(); componentIndex < componentCount; componentIndex++)
		{
			IEntityComponentSource component = source.GetComponent(componentIndex);
			if (!component)
				continue;

			typename componentType = component.GetClassName().ToType();
			if (!componentType || !componentType.IsInherited(UniversalInventoryStorageComponent) || !IsEnabled(component))
				continue;

			// equipment storages derive from the universal storage but hold one item per slot
			if (componentType.IsInherited(BaseEquipmentStorageComponent))
			{
				int purpose;
				component.Get("StoragePurpose", purpose);
				if (GDC_LoadoutUtils.IsGeneralPurpose(purpose))
					equipmentStorages.Insert(component);

				continue;
			}

			volumeFound = true;
			float volume;
			if (component.Get("MaxCumulativeVolume", volume) && volume > 0)
				node.m_fCapacity += volume;
			else
				allKnown = false;

			float maxWeight;
			if (component.Get("m_fMaxWeight", maxWeight) && maxWeight > 0)
				node.m_fMaxWeight += maxWeight;

			AddMaxItemSize(component, node);
		}

		if (volumeFound)
		{
			node.m_bCapacityKnown = allKnown;
			return true;
		}

		foreach (IEntityComponentSource equipmentStorage : equipmentStorages)
		{
			ReadSlots(equipmentStorage, node);
			AddMaxItemSize(equipmentStorage, node);
		}

		if (node.m_aSlotTypes.IsEmpty())
			return false;

		node.m_bSlotBased = true;
		node.m_bCapacityKnown = true;
		return true;
	}

	protected static void AddMaxItemSize(notnull BaseContainer storage, notnull GDC_LoadoutStorageNode node)
	{
		vector maxSize;
		if (storage.Get("MaxItemSize", maxSize) && (maxSize[0] > 0 || maxSize[1] > 0 || maxSize[2] > 0))
			node.m_aMaxItemSizes.Insert(maxSize);
	}

	//! Accepted item types of each slot: the engine list (AllowedItemTypes) and the RHS list (m_aCompatibleItemTypes) both apply.
	protected static void ReadSlots(notnull BaseContainer equipmentStorage, notnull GDC_LoadoutStorageNode node)
	{
		BaseContainerList slots = equipmentStorage.GetObjectArray("InitialStorageSlots");
		if (!slots)
			return;

		for (int slotIndex, slotCount = slots.Count(); slotIndex < slotCount; slotIndex++)
		{
			BaseContainer slot = slots.Get(slotIndex);
			if (!slot)
				continue;

			array<string> allowed = {};
			ReadTypeList(slot, "AllowedItemTypes", allowed);
			array<string> compatible = {};
			ReadTypeList(slot, "m_aCompatibleItemTypes", compatible);

			array<string> accepted = {};
			if (allowed.IsEmpty())
			{
				accepted.Copy(compatible);
			}
			else if (compatible.IsEmpty())
			{
				accepted.Copy(allowed);
			}
			else
			{
				foreach (string allowedType : allowed)
				{
					if (compatible.Contains(allowedType))
						accepted.Insert(allowedType);
				}
			}

			node.m_aSlotTypes.Insert(accepted);
		}
	}

	//! Reads a list of ECommonItemType values stored either as names or as numbers. NONE and empty values are ignored.
	protected static void ReadTypeList(notnull BaseContainer container, string field, notnull array<string> outTypes)
	{
		array<string> names = {};
		if (!container.Get(field, names) || !names)
		{
			array<int> values = {};
			if (container.Get(field, values) && values)
			{
				foreach (int value : values)
				{
					names.Insert(typename.EnumToString(ECommonItemType, value));
				}
			}
		}

		foreach (string typeName : names)
		{
			if (!typeName.IsEmpty() && typeName != "NONE" && typeName != "0" && !outTypes.Contains(typeName))
				outTypes.Insert(typeName);
		}
	}

	//! ECommonItemType name of an item, from its SCR_ItemAttributeCollection.
	static string ReadCommonType(notnull BaseContainer attributes)
	{
		int typeValue;
		if (attributes.Get("CommonItemType", typeValue))
		{
			string enumName = typename.EnumToString(ECommonItemType, typeValue);
			if (!enumName.IsEmpty())
				return enumName;
		}

		string typeName;
		if (attributes.Get("CommonItemType", typeName))
			return typeName;

		return string.Empty;
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

	//! Same pouch model on the same worn gear slot, when the pouch moved to a slot with another name,
	//! e.g. "Vest/A.et/nerd/Pouch_ifak.et" -> "Vest/B.et/ifak/Pouch_ifak.et". Only a unique candidate is returned.
	static GDC_LoadoutStorageNode FindByModel(notnull array<ref GDC_LoadoutStorageNode> nodes, string oldPath, int purpose)
	{
		array<string> oldParts = {};
		oldPath.Split("/", oldParts, true);
		if (oldParts.Count() < 4)
			return null;

		string oldRoot = oldParts[0];
		string oldFile = oldParts[oldParts.Count() - 1];

		GDC_LoadoutStorageNode match;
		foreach (GDC_LoadoutStorageNode node : nodes)
		{
			if (!PurposeFits(node, purpose))
				continue;

			array<string> parts = {};
			node.m_sPath.Split("/", parts, true);
			if (parts.Count() < 4 || !GDC_LoadoutUtils.SamePath(parts[0], oldRoot) || !GDC_LoadoutUtils.SamePath(parts[parts.Count() - 1], oldFile))
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

			// empty entry aimed at gear that is not worn (typically an inherited entry emptied earlier): nothing to show
			// or edit, and left untouched in the prefab since no container refers to it
			if (!entryNode && entry.m_aPrefabs.IsEmpty() && !entry.m_sTargetStorage.IsEmpty())
				continue;

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

	//! Rebinds the containers after a gear change.
	//! Containers whose storage disappeared keep their items and are flagged, unless outOrphanItems is given:
	//! their items are then moved into it (the 'Items To Distribute' list), and the prefab entries are emptied on Apply.
	static void Remap(
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		array<ref GDC_LoadoutItem> outOrphanItems = null,
		array<string> outReport = null)
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
				if (outOrphanItems && !container.IsAutomatic())
					MoveOrphanItems(container, outOrphanItems, outReport);

				// a container read from prefab entries is kept even when emptied, so that Apply empties those entries
				if (container.IsAutomatic() || container.CountItems() > 0 || !container.m_aSourceIndices.IsEmpty())
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

	//! Moves the items of a container whose storage is not on the current gear into the given list.
	protected static void MoveOrphanItems(notnull GDC_LoadoutContainer container, notnull array<ref GDC_LoadoutItem> outItems, array<string> outReport)
	{
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			bool merged;
			foreach (GDC_LoadoutItem listed : outItems)
			{
				if (listed && listed.m_Prefab == item.m_Prefab)
				{
					listed.m_iCount += item.m_iCount;
					merged = true;
					break;
				}
			}

			if (!merged)
				outItems.Insert(GDC_LoadoutItem.Create(item.m_Prefab, item.m_iCount));

			if (outReport)
				outReport.Insert(string.Format("%1 x %2 moved to 'Items To Distribute' ('%3' is not on the current gear).", item.m_iCount, GDC_LoadoutUtils.ShortName(item.m_Prefab), container.m_sTargetStorage));
		}

		container.m_aItems.Clear();
	}

	//! Storage of a container on the current gear: same path, same slot chain, then same pouch model.
	protected static GDC_LoadoutStorageNode ResolveContainer(notnull GDC_LoadoutContainer container, notnull array<ref GDC_LoadoutStorageNode> nodes)
	{
		GDC_LoadoutStorageNode node;
		if (!container.m_sNodePath.IsEmpty())
		{
			node = FindByPath(nodes, container.m_sNodePath, container.m_iTargetPurpose);
			if (!node)
				node = FindReplacement(nodes, container.m_sNodePath, container.m_iTargetPurpose);

			if (!node)
				node = FindByModel(nodes, container.m_sNodePath, container.m_iTargetPurpose);

			return node;
		}

		node = Match(nodes, container.m_sTargetStorage, container.m_iTargetPurpose);
		if (!node && !container.m_sTargetStorage.IsEmpty())
			node = FindByModel(nodes, container.m_sTargetStorage, container.m_iTargetPurpose);

		return node;
	}

	protected static GDC_LoadoutContainer CreateNodeContainer(notnull GDC_LoadoutStorageNode node)
	{
		GDC_LoadoutContainer container = new GDC_LoadoutContainer();
		container.m_sNodePath = node.m_sPath;
		container.m_sTargetStorage = node.m_sPath;
		container.m_iTargetPurpose = node.m_iPurpose;
		return container;
	}

	//! Makes sure a general automatic placement container exists, and moves the automatic containers to the end of the list
	//! (after the storages of the gear and the destinations not found on it).
	protected static void EnsureAutomaticContainer(notnull array<ref GDC_LoadoutContainer> containers)
	{
		array<ref GDC_LoadoutContainer> automatic = {};
		for (int index = containers.Count() - 1; index >= 0; index--)
		{
			GDC_LoadoutContainer container = containers[index];
			if (container && container.IsAutomatic())
			{
				automatic.InsertAt(container, 0);
				containers.RemoveOrdered(index);
			}
		}

		bool hasGeneral;
		foreach (GDC_LoadoutContainer automaticContainer : automatic)
		{
			if (GDC_LoadoutUtils.IsGeneralPurpose(automaticContainer.m_iTargetPurpose))
				hasGeneral = true;

			containers.Insert(automaticContainer);
		}

		if (!hasGeneral)
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

	//! Refreshes container titles: "NAME (prefab)  ·  details  ⚠ ALERT"; adds a warning line per problem.
	static void UpdateLabels(
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		notnull GDC_LoadoutItemCache itemCache,
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

			string name;
			array<string> details = {};
			array<string> alerts = {};
			if (node)
			{
				name = node.m_sLabel;
			}
			else if (gear)
			{
				name = gear.m_sLabel;
			}
			else if (container.m_sTargetStorage.IsEmpty())
			{
				// marked when it holds items, since their final place is decided by the game
				name = AUTOMATIC_LABEL;
				if (container.CountItems() > 0)
					name = GDC_LoadoutUtils.ICON_AUTOMATIC + " " + name;

				details.Insert("the game picks a storage");
			}
			else
			{
				// an emptied orphan only waits for Apply to empty its prefab entries: no alert
				name = container.m_sTargetStorage;
				if (container.CountItems() > 0)
					alerts.Insert(GDC_LoadoutUtils.ICON_WARNING + " NOT ON CURRENT GEAR");
			}

			if (!GDC_LoadoutUtils.IsGeneralPurpose(container.m_iTargetPurpose))
				details.Insert(GDC_LoadoutUtils.ReadablePurpose(container.m_iTargetPurpose));

			int itemCount = container.CountItems();
			details.Insert(GDC_LoadoutUtils.ItemCountText(itemCount));

			if (node && node.m_bSlotBased)
				DescribeSlots(container, node, itemCache, details, alerts, outWarnings);
			else if (node)
				DescribeVolume(container, node, containers, itemCache, details, alerts, outWarnings);

			if (node)
				DescribeSize(container, node, itemCache, alerts, outWarnings);

			if (!node && !gear && !container.m_sTargetStorage.IsEmpty() && itemCount > 0)
				outWarnings.Insert(string.Format("%1 items target '%2', which is not on the current gear. They are kept as configured.", itemCount, container.m_sTargetStorage));

			// alert icons go at the start of the line (each once), the alert texts stay at the end
			string label = name + GDC_LoadoutUtils.SEPARATOR + SCR_StringHelper.Join(", ", details);
			array<string> alertIcons = GetAlertIcons();
			array<string> icons = {};
			foreach (string alert : alerts)
			{
				string shown = alert;
				foreach (string icon : alertIcons)
				{
					if (!shown.StartsWith(icon))
						continue;

					if (!icons.Contains(icon))
						icons.Insert(icon);

					shown = shown.Substring(icon.Length(), shown.Length() - icon.Length());
					shown.TrimInPlace();
					break;
				}

				label += "  " + shown;
			}

			// same order whatever the order of the alerts: ❗ first, then ⚠
			foreach (string orderedIcon : alertIcons)
			{
				if (icons.Contains(orderedIcon))
					label = orderedIcon + " " + label;
			}

			container.m_sLabel = label;
		}
	}

	//! Slot-based storage: one item per slot, only accepted item types.
	protected static void DescribeSlots(
		notnull GDC_LoadoutContainer container,
		notnull GDC_LoadoutStorageNode node,
		notnull GDC_LoadoutItemCache itemCache,
		notnull array<string> details,
		notnull array<string> alerts,
		notnull array<string> outWarnings)
	{
		array<string> itemTypes = {};
		itemCache.CollectTypes(container, itemTypes);

		array<string> refused = {};
		int fitted = node.FitSlots(itemTypes, refused);
		int slotCount = node.m_aSlotTypes.Count();
		details.Insert(string.Format("%1 / %2 slots", itemTypes.Count(), slotCount));

		foreach (string refusedType : refused)
		{
			string itemName = itemCache.FindItemName(container, refusedType);
			alerts.Insert(GDC_LoadoutUtils.ICON_OVER_LIMIT + " NOT ACCEPTED: " + itemName);
			outWarnings.Insert(string.Format("%1: %2 is not accepted by this pouch (item type '%3'), it will not be spawned there.", node.m_sLabel, itemName, refusedType));
		}

		int accepted = itemTypes.Count();
		foreach (string itemType : itemTypes)
		{
			if (refused.Contains(itemType))
				accepted--;
		}

		if (fitted < accepted)
		{
			alerts.Insert(GDC_LoadoutUtils.ICON_OVER_LIMIT + " OVER CAPACITY");
			outWarnings.Insert(string.Format("%1: %2 items for %3 slots, only %4 will be spawned there.", node.m_sLabel, itemTypes.Count(), slotCount, fitted));
		}
	}

	//! Items larger than every size limit of the storage (MaxItemSize), when both sizes are known.
	protected static void DescribeSize(
		notnull GDC_LoadoutContainer container,
		notnull GDC_LoadoutStorageNode node,
		notnull GDC_LoadoutItemCache itemCache,
		notnull array<string> alerts,
		notnull array<string> outWarnings)
	{
		foreach (GDC_LoadoutItem item : container.m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty() || item.m_iCount <= 0)
				continue;

			GDC_LoadoutItemInfo info = itemCache.Get(item.m_Prefab);
			if (node.AcceptsSize(info))
				continue;

			string itemName = GDC_LoadoutUtils.ShortName(item.m_Prefab);
			alerts.Insert(GDC_LoadoutUtils.ICON_OVER_LIMIT + " TOO LARGE: " + itemName);
			outWarnings.Insert(string.Format("%1: %2 (size %3) is larger than the storage allows, it will not be spawned there.", node.m_sLabel, itemName, info.m_vDimensions.ToString(false)));
		}
	}

	//! Weight carried by a storage: items of its containers and of the containers of the gear attached below it
	//! (the game counts the content of attached pouches in the weight of the storage that holds them).
	protected static float SubtreeWeight(
		notnull GDC_LoadoutStorageNode node,
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull GDC_LoadoutItemCache itemCache)
	{
		string nodePath = node.m_sPath;
		nodePath.ToLower();
		string prefix = nodePath + "/";

		float total;
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (!container || container.m_sNodePath.IsEmpty())
				continue;

			string containerPath = container.m_sNodePath;
			containerPath.ToLower();
			if (containerPath == nodePath || containerPath.StartsWith(prefix))
				total += itemCache.UsedWeight(container);
		}

		return total;
	}

	//! Regular storage: volume and, when defined, weight limits.
	protected static void DescribeVolume(
		notnull GDC_LoadoutContainer container,
		notnull GDC_LoadoutStorageNode node,
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull GDC_LoadoutItemCache itemCache,
		notnull array<string> details,
		notnull array<string> alerts,
		notnull array<string> outWarnings)
	{
		bool hasUnknown;
		float used = itemCache.UsedVolume(container, hasUnknown);
		if (node.m_bCapacityKnown)
		{
			int shownUsed = Math.Round(used);
			int shownCapacity = Math.Round(node.m_fCapacity);
			details.Insert(string.Format("%1 / %2", shownUsed, shownCapacity));
			if (used > node.m_fCapacity)
			{
				alerts.Insert(GDC_LoadoutUtils.ICON_OVER_LIMIT + " OVER CAPACITY");
				outWarnings.Insert(string.Format("%1: over capacity (%2 / %3), the game may not spawn every item.", node.m_sLabel, shownUsed, shownCapacity));
			}
		}

		if (hasUnknown && container.CountItems() > 0)
			details.Insert("some volumes unknown");

		if (node.m_fMaxWeight <= 0)
			return;

		float weight = SubtreeWeight(node, containers, itemCache);
		details.Insert(string.Format("%1 / %2 kg", weight.ToString(-1, 1), node.m_fMaxWeight.ToString(-1, 1)));
		if (weight > node.m_fMaxWeight)
		{
			alerts.Insert(GDC_LoadoutUtils.ICON_OVER_LIMIT + " OVERWEIGHT");
			outWarnings.Insert(string.Format("%1: overweight (%2 / %3 kg), the game may not spawn every item.", node.m_sLabel, weight.ToString(-1, 1), node.m_fMaxWeight.ToString(-1, 1)));
		}
	}

	//! True when one more copy of the item fits in the container: size, slots or volume, and the weight limits
	//! of the storage and of the storage it is attached to.
	protected static bool HasRoom(
		notnull GDC_LoadoutContainer container,
		notnull GDC_LoadoutStorageNode node,
		notnull GDC_LoadoutItemInfo info,
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		notnull GDC_LoadoutItemCache itemCache)
	{
		if (!node.AcceptsSize(info))
			return false;

		float weight = Math.Max(info.m_fWeight, 0);
		if (node.m_bSlotBased)
		{
			array<string> itemTypes = {};
			itemCache.CollectTypes(container, itemTypes);
			itemTypes.Insert(info.m_sType);

			array<string> refused = {};
			if (node.FitSlots(itemTypes, refused) != itemTypes.Count())
				return false;
		}
		else
		{
			bool hasUnknown;
			float volume = Math.Max(info.m_fVolume, 0);
			if (itemCache.UsedVolume(container, hasUnknown) + volume > node.m_fCapacity)
				return false;

			if (node.m_fMaxWeight > 0 && SubtreeWeight(node, containers, itemCache) + weight > node.m_fMaxWeight)
				return false;
		}

		GDC_LoadoutStorageNode parent = FindByPath(nodes, node.m_sParentPath, 0);
		if (parent && parent.m_fMaxWeight > 0 && SubtreeWeight(parent, containers, itemCache) + weight > parent.m_fMaxWeight)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Automatic distribution
	//------------------------------------------------------------------------------------------------

	//! Places items into the general storages, following the gear order given in slotOrder ("Vest,Jacket,...").
	//! Pouches with a free slot accepting the item come first, then storages with enough volume and weight left.
	//! Items that fit nowhere go to automatic placement.
	static void Distribute(
		notnull array<ref GDC_LoadoutItem> items,
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		string slotOrder,
		notnull GDC_LoadoutItemCache itemCache,
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

			GDC_LoadoutItemInfo info = itemCache.Get(item.m_Prefab);
			if (info.m_fVolume < 0)
				outReport.Insert(string.Format("%1: volume unknown, counted as 0.", GDC_LoadoutUtils.ShortName(item.m_Prefab)));

			int unplaced;
			for (int copy = 0; copy < item.m_iCount; copy++)
			{
				GDC_LoadoutContainer destination = FindRoom(candidates, containers, nodes, info, itemCache);
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
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		notnull GDC_LoadoutItemInfo info,
		notnull GDC_LoadoutItemCache itemCache)
	{
		// pass 0: pouches with a matching slot, pass 1: volume storages
		for (int pass = 0; pass < 2; pass++)
		{
			foreach (GDC_LoadoutContainer candidate : candidates)
			{
				GDC_LoadoutStorageNode node = FindByPath(nodes, candidate.m_sNodePath, candidate.m_iTargetPurpose);
				if (node.m_bSlotBased != (pass == 0))
					continue;

				if (HasRoom(candidate, node, info, containers, nodes, itemCache))
					return candidate;
			}
		}

		return null;
	}
}
#endif
