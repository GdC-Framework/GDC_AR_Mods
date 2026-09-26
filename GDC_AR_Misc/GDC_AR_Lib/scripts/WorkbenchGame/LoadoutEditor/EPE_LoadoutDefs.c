#ifdef WORKBENCH
class EPE_LoadoutSlotNames
{
	static const string HAT = "Hat";
	static const string JACKET = "Jacket";
	static const string PANTS = "Pants";
	static const string BOOTS = "Boots";
	static const string VEST = "Vest";
	static const string ARMORED_VEST = "ArmoredVest";
	static const string BACK = "Back";
	static const string HANDWEAR = "HandWear";

	static array<string> GetAll()
	{
		return { HAT, JACKET, VEST, ARMORED_VEST, BACK, HANDWEAR, PANTS, BOOTS };
	}

	static array<string> GetStorageCandidates()
	{
		return GetAll();
	}

	static bool SameSlot(string a, string b)
	{
		string left = a;
		string right = b;
		left.ToLower();
		right.ToLower();
		return left == right;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_Prefab", true)]
class EPE_LoadoutItemEntry
{
	[Attribute(desc: "Item prefab to spawn", params: "et")]
	ResourceName m_Prefab;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "How many copies to place", params: "1 inf 1")]
	int m_iCount;

	void EPE_LoadoutItemEntry(ResourceName prefab = ResourceName.Empty, int count = 1)
	{
		m_Prefab = prefab;
		m_iCount = count;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sDisplayLabel")]
class EPE_LoadoutStorageEntry
{
	[Attribute(defvalue: "-1", uiwidget: UIWidgets.None)]
	int m_iSourceIndex;

	[Attribute(uiwidget: UIWidgets.None)]
	string m_sTargetStorage;

	[Attribute(desc: "Where these items go. Filled in automatically from the gear you wear.")]
	string m_sDisplayLabel;

	[Attribute(desc: "Items to place into this storage")]
	ref array<ref EPE_LoadoutItemEntry> m_aItems;

	[Attribute(uiwidget: UIWidgets.None)]
	int m_iTargetPurpose;

	void EPE_LoadoutStorageEntry()
	{
		if (!m_aItems)
			m_aItems = {};
	}

	void SetDestination(string targetStorage, int targetPurpose = 0)
	{
		m_sTargetStorage = targetStorage;
		m_iTargetPurpose = targetPurpose;
		RefreshDisplayLabel();
	}

	void RefreshDisplayLabel()
	{
		m_sDisplayLabel = BuildDisplayLabel(m_sTargetStorage, m_iTargetPurpose);
	}

	static string BuildDisplayLabel(string targetStorage, int targetPurpose = 0)
	{
		if (targetStorage.IsEmpty())
			return string.Format("(automatic destination, purpose %1)", targetPurpose);

		array<string> parts = {};
		targetStorage.Split("/", parts, true);
		if (parts.IsEmpty())
			return targetStorage;

		string container = parts[parts.Count() - 1];
		container.Replace(".et", string.Empty);

		string label;
		if (parts.Count() < 2)
			label = container;
		else
			label = string.Format("%1 - %2", parts[parts.Count() - 2], container);

		if (targetPurpose != 0 && targetPurpose != 1)
			label += string.Format(" [purpose %1]", targetPurpose);

		return label;
	}

	int BuildPrefabsToSpawn(notnull array<ResourceName> outPrefabs)
	{
		foreach (EPE_LoadoutItemEntry item : m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty())
				continue;

			const int count = Math.Max(item.m_iCount, 1);
			for (int i = 0; i < count; i++)
			{
				outPrefabs.Insert(item.m_Prefab);
			}
		}

		return outPrefabs.Count();
	}

	void AddOrMergeItem(ResourceName prefab, int count = 1)
	{
		if (prefab.IsEmpty())
			return;

		const int normalizedCount = Math.Max(count, 1);
		if (!m_aItems.IsEmpty())
		{
			EPE_LoadoutItemEntry item = m_aItems[m_aItems.Count() - 1];
			if (item && item.m_Prefab == prefab)
			{
				item.m_iCount += normalizedCount;
				return;
			}
		}

		m_aItems.Insert(new EPE_LoadoutItemEntry(prefab, normalizedCount));
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_Prefab", true)]
class EPE_LoadoutWeaponEntry
{
	[Attribute(desc: "Weapon prefab", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	[Attribute(defvalue: "0", desc: "Weapon slot index (0 = primary)")]
	int m_iSlotIndex;

	void EPE_LoadoutWeaponEntry(ResourceName prefab = ResourceName.Empty, int slotIndex = 0)
	{
		m_Prefab = prefab;
		m_iSlotIndex = slotIndex;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_Prefab", true)]
class EPE_LoadoutWornEntry
{
	[Attribute(desc: "Slot name, e.g. Jacket or Vest")]
	string m_sSlotName;

	[Attribute(desc: "Worn item prefab, empty to clear the slot", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	void EPE_LoadoutWornEntry(string slotName = string.Empty, ResourceName prefab = ResourceName.Empty)
	{
		m_sSlotName = slotName;
		m_Prefab = prefab;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({ "m_sSlotName", "m_Prefab" }, "%1: %2")]
class EPE_LoadoutGadgetEntry
{
	[Attribute(desc: "Slot name as it exists on the character; see 'List Destinations'")]
	string m_sSlotName;

	[Attribute(desc: "Gadget prefab, empty to clear the slot", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	void EPE_LoadoutGadgetEntry(string slotName = string.Empty, ResourceName prefab = ResourceName.Empty)
	{
		m_sSlotName = slotName;
		m_Prefab = prefab;
	}
}

[BaseContainerProps(configRoot: true)]
class EPE_LoadoutDefinition
{
	int m_iStorageEntryCount;

	[Attribute(desc: "Character prefab this loadout is based on")]
	ResourceName m_BaseCharacter;

	[Attribute(desc: "Worn clothing and armour")]
	ref array<ref EPE_LoadoutWornEntry> m_aWorn;

	[Attribute(desc: "Weapons by slot index")]
	ref array<ref EPE_LoadoutWeaponEntry> m_aWeapons;

	[Attribute(desc: "Grenades and throwables by weapon slot index")]
	ref array<ref EPE_LoadoutWeaponEntry> m_aGrenades;

	[Attribute(desc: "Gadgets carried on the character")]
	ref array<ref EPE_LoadoutGadgetEntry> m_aGadgets;

	[Attribute(desc: "Items placed into storages")]
	ref array<ref EPE_LoadoutStorageEntry> m_aStorages;

	void EPE_LoadoutDefinition()
	{
		if (!m_aWorn)
			m_aWorn = {};

		if (!m_aWeapons)
			m_aWeapons = {};

		if (!m_aGrenades)
			m_aGrenades = {};

		if (!m_aGadgets)
			m_aGadgets = {};

		if (!m_aStorages)
			m_aStorages = {};
	}

	EPE_LoadoutWornEntry FindWorn(string slotName)
	{
		foreach (EPE_LoadoutWornEntry worn : m_aWorn)
		{
			if (worn && EPE_LoadoutSlotNames.SameSlot(worn.m_sSlotName, slotName))
				return worn;
		}

		return null;
	}

	void SetWorn(string slotName, ResourceName prefab)
	{
		EPE_LoadoutWornEntry existing = FindWorn(slotName);
		if (existing)
		{
			existing.m_Prefab = prefab;
			return;
		}

		m_aWorn.Insert(new EPE_LoadoutWornEntry(slotName, prefab));
	}
}
#endif
