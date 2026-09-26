#ifdef WORKBENCH
class EPE_StorageNode
{
	string m_sPath;
	string m_sParentPath;
	ResourceName m_Prefab;
	string m_sSlotName;
	string m_sDisplayName;
	bool m_bHasStorage;
	bool m_bCapacityKnown;
	float m_fMaxVolume;
	float m_fLargestCompartment;
	int m_iCompartments;
	bool m_bChildrenTruncated;
	int m_iTargetPurpose;
	int m_iDepth;

	void EPE_StorageNode(string path, ResourceName prefab, string slotName, int depth)
	{
		m_sPath = path;
		m_Prefab = prefab;
		m_sSlotName = slotName;
		m_iDepth = depth;
		m_iTargetPurpose = EStoragePurpose.PURPOSE_DEPOSIT;
	}

	bool IsContainer()
	{
		return m_bHasStorage && m_iTargetPurpose <= EStoragePurpose.PURPOSE_DEPOSIT;
	}

	bool RejectsItemVolume(float itemVolume)
	{
		return m_bCapacityKnown && itemVolume > m_fLargestCompartment;
	}

	bool IsNestedIn(notnull EPE_StorageNode other)
	{
		if (!m_sParentPath.IsEmpty())
			return m_sParentPath == other.m_sPath || m_sParentPath.StartsWith(other.m_sPath + "/");

		return m_sPath != other.m_sPath && m_sPath.StartsWith(other.m_sPath + "/");
	}
}

class EPE_StoragePathResolver
{
	protected static const int MAX_DEPTH = 4;

	protected static const string SLOTS_FIELD = "Slots";
	protected static const string PREFAB_FIELD = "Prefab";
	protected static const string MAX_VOLUME_FIELD = "MaxCumulativeVolume";
	protected static const string ITEM_VOLUME_FIELD = "ItemVolume";
	protected static const string ATTRIBUTES_FIELD = "Attributes";
	protected static const string PHYS_ATTRIBUTES_FIELD = "ItemPhysAttributes";
	protected static const string STORAGE_PURPOSE_FIELD = "StoragePurpose";
	protected static const string COMPONENTS_FIELD = "components";

	static int CollectPathsForWornItem(
		string slotName,
		ResourceName itemPrefab,
		notnull array<ref EPE_StorageNode> outNodes)
	{
		if (slotName.IsEmpty() || itemPrefab.IsEmpty())
			return 0;

		Resource itemResource = Resource.Load(itemPrefab);
		if (!itemResource || !itemResource.IsValid())
			return 0;

		string fileName = FilePath.StripPath(itemPrefab.GetPath());
		if (fileName.IsEmpty())
			return 0;

		string rootPath = string.Format("%1/%2", slotName, fileName);

		EPE_StorageNode root = new EPE_StorageNode(rootPath, itemPrefab, slotName, 0);
		ReadPrimaryStorage(itemResource, root);
		root.m_sDisplayName = ReadDisplayName(itemResource, ReadableFileName(fileName));
		outNodes.Insert(root);
		CollectEquipmentDestinations(itemResource, root, outNodes);

		IEntityComponentSource clothComponent = SCR_BaseContainerTools.FindComponentSource(itemResource, BaseLoadoutClothComponent);
		if (clothComponent)
		{
			array<ResourceName> branch = {};
			branch.Insert(itemPrefab);
			CollectFromSlotList(clothComponent, root, 1, branch, outNodes);
		}

		return outNodes.Count();
	}

	static string ReadableFileName(string fileName)
	{
		string name = fileName;
		name.Replace(".et", string.Empty);
		return name;
	}

	static string DescribeArrayInheritance(notnull BaseContainer container, string fieldName)
	{
		string report;
		int level = 0;
		BaseContainer current = container;
		while (current)
		{
			BaseContainerList entries = current.GetObjectArray(fieldName);
			string count = "absent";
			if (entries)
				count = string.Format("%1 entries", entries.Count());

			string authored = "inherited";
			if (current.IsVariableSetDirectly(fieldName))
				authored = "set here";

			report += string.Format("    level %1 (%2): %3, %4\n", level, current.GetClassName(), count, authored);
			level++;
			current = current.GetAncestor();
		}

		if (report.IsEmpty())
			return string.Format("    %1: no container\n", fieldName);

		return report;
	}

	static void GatherSlots(notnull BaseContainer slotHolder, notnull map<string, BaseContainer> outSlots)
	{
		BaseContainer current = slotHolder;
		while (current)
		{
			BaseContainerList slots = current.GetObjectArray(SLOTS_FIELD);
			if (slots)
			{
				for (int i, count = slots.Count(); i < count; i++)
				{
					BaseContainer slot = slots.Get(i);
					if (!slot)
						continue;

					string slotName = slot.GetName();
					if (slotName.IsEmpty())
						continue;

					if (!outSlots.Contains(slotName))
						outSlots.Set(slotName, slot);
				}
			}

			current = current.GetAncestor();
		}
	}

	static ResourceName ResolvePrefab(notnull BaseContainer slot)
	{
		BaseContainer current = slot;
		while (current)
		{
			ResourceName prefab;
			if (current.Get(PREFAB_FIELD, prefab))
				return prefab;

			current = current.GetAncestor();
		}

		return ResourceName.Empty;
	}

	protected static void CollectFromSlotList(
		notnull BaseContainer slotHolder,
		notnull EPE_StorageNode parentNode,
		int depth,
		notnull array<ResourceName> branch,
		notnull array<ref EPE_StorageNode> outNodes)
	{
		map<string, BaseContainer> slotsByName = new map<string, BaseContainer>();
		GatherSlots(slotHolder, slotsByName);
		if (slotsByName.IsEmpty())
			return;

		if (depth >= MAX_DEPTH)
		{
			parentNode.m_bChildrenTruncated = true;
			PrintFormat("EPE_StoragePathResolver: stopped at depth %1 under '%2'; deeper containers are not listed",
				depth, parentNode.m_sPath, level: LogLevel.WARNING);
			return;
		}

		foreach (string slotName, BaseContainer slot : slotsByName)
		{
			ResourceName slotPrefab = ResolvePrefab(slot);
			if (slotPrefab.IsEmpty())
				continue;

			string fileName = FilePath.StripPath(slotPrefab.GetPath());
			if (fileName.IsEmpty())
				continue;

			if (branch.Contains(slotPrefab))
			{
				parentNode.m_bChildrenTruncated = true;
				PrintFormat("EPE_StoragePathResolver: '%1' contains itself through slot '%2'; that branch is not listed",
					slotPrefab, slotName, level: LogLevel.WARNING);
				continue;
			}

			string path = string.Format("%1/%2/%3", parentNode.m_sPath, slotName, fileName);

			EPE_StorageNode node = new EPE_StorageNode(path, slotPrefab, slotName, depth);
			node.m_sParentPath = parentNode.m_sPath;

			Resource slotResource = Resource.Load(slotPrefab);
			if (!slotResource || !slotResource.IsValid())
			{
				outNodes.Insert(node);
				continue;
			}

			ReadPrimaryStorage(slotResource, node);
			node.m_sDisplayName = ReadDisplayName(slotResource, ReadableFileName(fileName));
			outNodes.Insert(node);
			CollectEquipmentDestinations(slotResource, node, outNodes);

			IEntityComponentSource clothComponent = SCR_BaseContainerTools.FindComponentSource(slotResource, BaseLoadoutClothComponent);
			if (!clothComponent)
				continue;

			branch.Insert(slotPrefab);
			CollectFromSlotList(clothComponent, node, depth + 1, branch, outNodes);
			branch.RemoveItem(slotPrefab);
		}
	}

	protected static void ReadPrimaryStorage(notnull Resource itemResource, notnull EPE_StorageNode node)
	{
		node.m_iTargetPurpose = EStoragePurpose.PURPOSE_DEPOSIT;
		node.m_bHasStorage = false;
		node.m_bCapacityKnown = false;
		node.m_fMaxVolume = 0;
		node.m_fLargestCompartment = 0;
		node.m_iCompartments = 0;

		float clothVolume;
		bool clothVolumeKnown;
		IEntityComponentSource clothStorage = SCR_BaseContainerTools.FindComponentSource(itemResource, ClothNodeStorageComponent);
		if (clothStorage && IsEnabled(clothStorage))
		{
			node.m_bHasStorage = true;
			clothVolumeKnown = TryReadInheritedFloat(clothStorage, MAX_VOLUME_FIELD, clothVolume);
			if (clothVolumeKnown && clothVolume > 0)
			{
				node.m_bCapacityKnown = true;
				node.m_fMaxVolume = clothVolume;
				node.m_fLargestCompartment = clothVolume;
				node.m_iCompartments = 1;
				return;
			}
		}

		array<IEntityComponentSource> storages = {};
		CollectTopLevelStorages(itemResource, storages);
		if (!storages.IsEmpty())
		{
			node.m_bHasStorage = true;
			node.m_iCompartments = storages.Count();
			node.m_bCapacityKnown = true;
			foreach (IEntityComponentSource storage : storages)
			{
				float volume;
				if (!TryReadInheritedFloat(storage, MAX_VOLUME_FIELD, volume))
				{
					node.m_bCapacityKnown = false;
					continue;
				}

				node.m_fMaxVolume += volume;
				if (volume > node.m_fLargestCompartment)
					node.m_fLargestCompartment = volume;
			}

			return;
		}

		node.m_bCapacityKnown = clothVolumeKnown;
		node.m_fMaxVolume = clothVolume;
		node.m_fLargestCompartment = clothVolume;
		if (node.m_bHasStorage)
			node.m_iCompartments = 1;
	}

	protected static void CollectEquipmentDestinations(
		notnull Resource itemResource,
		notnull EPE_StorageNode ownerNode,
		notnull array<ref EPE_StorageNode> outNodes)
	{
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(itemResource);
		if (!entitySource)
			return;

		for (int i, count = entitySource.GetComponentCount(); i < count; i++)
		{
			CollectEquipmentDestinationsFromContainer(
				entitySource.GetComponent(i),
				ownerNode,
				outNodes);
		}
	}

	protected static void CollectEquipmentDestinationsFromContainer(
		notnull BaseContainer container,
		notnull EPE_StorageNode ownerNode,
		notnull array<ref EPE_StorageNode> outNodes,
		int depth = 0)
	{
		const int MAX_COMPONENT_DEPTH = 5;
		if (depth > MAX_COMPONENT_DEPTH)
			return;

		typename componentType = container.GetClassName().ToType();
		if (componentType
			&& componentType.IsInherited(SCR_EquipmentStorageComponent)
			&& IsEnabled(container))
		{
			const int purpose = ReadStoragePurpose(container);
			if (purpose != 0 && !ContainsDestination(outNodes, ownerNode.m_sPath, purpose))
			{
				EPE_StorageNode special = new EPE_StorageNode(
					ownerNode.m_sPath,
					ownerNode.m_Prefab,
					ownerNode.m_sSlotName,
					ownerNode.m_iDepth);
				special.m_sDisplayName = ownerNode.m_sDisplayName;
				special.m_sParentPath = ownerNode.m_sParentPath;
				special.m_iTargetPurpose = purpose;
				special.m_bHasStorage = true;
				special.m_bCapacityKnown = false;
				special.m_iCompartments = 1;
				outNodes.Insert(special);
			}
		}

		BaseContainerList nested = container.GetObjectArray(COMPONENTS_FIELD);
		if (!nested)
			return;

		for (int i, count = nested.Count(); i < count; i++)
		{
			if (nested.Get(i))
				CollectEquipmentDestinationsFromContainer(nested.Get(i), ownerNode, outNodes, depth + 1);
		}
	}

	protected static bool ContainsDestination(
		notnull array<ref EPE_StorageNode> nodes,
		string path,
		int targetPurpose)
	{
		foreach (EPE_StorageNode node : nodes)
		{
			if (node
				&& node.m_sPath == path
				&& node.m_iTargetPurpose == targetPurpose)
			{
				return true;
			}
		}

		return false;
	}

	protected static int ReadStoragePurpose(notnull BaseContainer container)
	{
		BaseContainer current = container;
		while (current)
		{
			if (current.IsVariableSetDirectly(STORAGE_PURPOSE_FIELD))
			{
				int purpose;
				if (current.Get(STORAGE_PURPOSE_FIELD, purpose))
					return purpose;
			}

			current = current.GetAncestor();
		}

		return 0;
	}

	protected static void CollectTopLevelStorages(
		notnull Resource itemResource,
		notnull array<IEntityComponentSource> outStorages)
	{
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(itemResource);
		if (!entitySource)
			return;

		for (int i, count = entitySource.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = entitySource.GetComponent(i);
			typename componentType = component.GetClassName().ToType();
			if (!componentType)
				continue;

			if (componentType.IsInherited(BaseEquipmentStorageComponent))
				continue;

			if (!componentType.IsInherited(UniversalInventoryStorageComponent) || !IsEnabled(component))
				continue;

			outStorages.Insert(component);
		}
	}

	protected static bool IsEnabled(notnull BaseContainer container)
	{
		BaseContainer current = container;
		while (current)
		{
			if (current.IsVariableSetDirectly("Enabled"))
			{
				bool enabled;
				if (current.Get("Enabled", enabled))
					return enabled;
			}

			current = current.GetAncestor();
		}

		return true;
	}

	protected static bool TryReadInheritedFloat(notnull BaseContainer container, string fieldName, out float value)
	{
		value = 0;

		BaseContainer current = container;
		while (current)
		{
			if (current.IsVariableSetDirectly(fieldName) && current.Get(fieldName, value))
				return true;

			current = current.GetAncestor();
		}

		return false;
	}

	protected static string ReadDisplayName(notnull Resource itemResource, string fallback)
	{
		IEntityComponentSource itemComponent = SCR_BaseContainerTools.FindComponentSource(itemResource, InventoryItemComponent);
		if (!itemComponent)
			return fallback;

		BaseContainer current = itemComponent;
		while (current)
		{
			BaseContainer attributes = current.GetObject(ATTRIBUTES_FIELD);
			if (attributes)
			{
				BaseContainer displayName = attributes.GetObject("ItemDisplayName");
				if (displayName)
				{
					string name;
					if (displayName.Get("Name", name) && !name.IsEmpty() && !name.StartsWith("#"))
						return name;
				}
			}

			current = current.GetAncestor();
		}

		return fallback;
	}

	static bool DescribeItem(ResourceName itemPrefab, out float volume, out string displayName)
	{
		volume = 0;
		displayName = string.Empty;

		if (itemPrefab.IsEmpty())
			return false;

		displayName = ReadableFileName(FilePath.StripPath(itemPrefab.GetPath()));

		Resource itemResource = Resource.Load(itemPrefab);
		if (!itemResource || !itemResource.IsValid())
			return false;

		displayName = ReadDisplayName(itemResource, displayName);
		return ReadItemVolume(itemResource, volume);
	}

	protected static bool ReadItemVolume(notnull Resource itemResource, out float volume)
	{
		volume = 0;

		IEntityComponentSource itemComponent = SCR_BaseContainerTools.FindComponentSource(itemResource, InventoryItemComponent);
		if (!itemComponent)
			return false;

		bool haveInherited;
		float inherited;

		BaseContainer current = itemComponent;
		while (current)
		{
			BaseContainer attributes = current.GetObject(ATTRIBUTES_FIELD);
			if (attributes)
			{
				BaseContainer physAttributes = attributes.GetObject(PHYS_ATTRIBUTES_FIELD);
				if (physAttributes)
				{
					if (physAttributes.IsVariableSetDirectly(ITEM_VOLUME_FIELD) && physAttributes.Get(ITEM_VOLUME_FIELD, volume))
						return true;

					float plain;
					if (!haveInherited && physAttributes.Get(ITEM_VOLUME_FIELD, plain) && plain != 0)
					{
						haveInherited = true;
						inherited = plain;
					}
				}
			}

			current = current.GetAncestor();
		}

		volume = inherited;
		return haveInherited;
	}
}
#endif
