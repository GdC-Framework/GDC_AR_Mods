#ifdef WORKBENCH
//! Shared helpers of the GDC loadout editor.
class GDC_LoadoutUtils
{
	//! File name of a prefab without extension, or "empty".
	static string ShortName(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return "empty";

		return FilePath.StripExtension(FilePath.StripPath(prefab.GetPath()));
	}

	//! Case-insensitive comparison, GUID prefixes ignored.
	static bool SamePath(string left, string right)
	{
		ResourceName leftName = left;
		ResourceName rightName = right;
		string leftPath = leftName.GetPath();
		string rightPath = rightName.GetPath();
		leftPath.ToLower();
		rightPath.ToLower();
		return leftPath == rightPath;
	}

	//! Purposes that designate the regular storage of an item (as opposed to special storages such as flashlight holders).
	static bool IsGeneralPurpose(int purpose)
	{
		return purpose == 0 || purpose == 1 || purpose == EStoragePurpose.PURPOSE_UNSET || purpose == EStoragePurpose.PURPOSE_DEPOSIT;
	}

	static bool SamePurpose(int left, int right)
	{
		if (IsGeneralPurpose(left) && IsGeneralPurpose(right))
			return true;

		return left == right;
	}

	static string PurposeName(int purpose)
	{
		string name = typename.EnumToString(EStoragePurpose, purpose);
		if (name.IsEmpty())
			return string.Format("purpose %1", purpose);

		return name;
	}

	//! Same prefabs with the same multiplicity, order ignored.
	static bool SamePrefabSet(notnull array<ResourceName> left, notnull array<ResourceName> right)
	{
		if (left.Count() != right.Count())
			return false;

		array<string> sortedLeft = {};
		array<string> sortedRight = {};
		foreach (ResourceName leftPrefab : left)
		{
			sortedLeft.Insert(leftPrefab);
		}

		foreach (ResourceName rightPrefab : right)
		{
			sortedRight.Insert(rightPrefab);
		}

		sortedLeft.Sort();
		sortedRight.Sort();
		for (int i, count = sortedLeft.Count(); i < count; i++)
		{
			if (sortedLeft[i] != sortedRight[i])
				return false;
		}

		return true;
	}

	static bool SamePrefabOrder(notnull array<ResourceName> left, notnull array<ResourceName> right)
	{
		if (left.Count() != right.Count())
			return false;

		for (int i, count = left.Count(); i < count; i++)
		{
			if (left[i] != right[i])
				return false;
		}

		return true;
	}

	//! Array value in the format expected by WorldEditorAPI.SetVariableValue.
	static string JoinPrefabs(notnull array<ResourceName> prefabs)
	{
		string result;
		foreach (int i, ResourceName prefab : prefabs)
		{
			if (i > 0)
				result += ",";

			result += prefab;
		}

		return result;
	}
}

//! Title "<label>: <prefab file name>" for panel entries holding one prefab.
class GDC_LoadoutEntryTitle : BaseContainerCustomTitle
{
	protected string m_sLabelField;
	protected string m_sPrefabField;

	void GDC_LoadoutEntryTitle(string labelField, string prefabField)
	{
		m_sLabelField = labelField;
		m_sPrefabField = prefabField;
	}

	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		string label;
		source.Get(m_sLabelField, label);

		ResourceName prefab;
		source.Get(m_sPrefabField, prefab);

		title = string.Format("%1: %2", label, GDC_LoadoutUtils.ShortName(prefab));
		return true;
	}
}

//! Title "<count> x <prefab file name>" for inventory items.
class GDC_LoadoutItemTitle : BaseContainerCustomTitle
{
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		ResourceName prefab;
		source.Get("m_Prefab", prefab);

		int count;
		source.Get("m_iCount", count);

		title = string.Format("%1 x %2", count, GDC_LoadoutUtils.ShortName(prefab));
		return true;
	}
}

//! Clothing slot of the character (BaseLoadoutManagerComponent slot).
[BaseContainerProps(), GDC_LoadoutEntryTitle("m_sSlotName", "m_Prefab")]
class GDC_LoadoutWornSlot
{
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sSlotName;

	[Attribute(desc: "Clothing worn in this slot. Leave empty to remove it. Press 'Refresh Containers' after a change to update the inventory containers.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	// No constructor with parameters on panel classes: the Workbench recreates them without arguments
	// when the panel is edited, and such a constructor would overwrite the edited values.
	static GDC_LoadoutWornSlot Create(string slotName, ResourceName prefab)
	{
		GDC_LoadoutWornSlot slot = new GDC_LoadoutWornSlot();
		slot.m_sSlotName = slotName;
		slot.m_Prefab = prefab;
		return slot;
	}
}

//! Weapon or grenade slot component of the character.
[BaseContainerProps(), GDC_LoadoutEntryTitle("m_sLabel", "m_Prefab")]
class GDC_LoadoutWeaponSlot
{
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sKey;

	[Attribute(uiwidget: UIWidgets.None)]
	string m_sLabel;

	[Attribute(desc: "Weapon or throwable in this slot. Leave empty to remove it.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	static GDC_LoadoutWeaponSlot Create(string key, string label, ResourceName prefab)
	{
		GDC_LoadoutWeaponSlot slot = new GDC_LoadoutWeaponSlot();
		slot.m_sKey = key;
		slot.m_sLabel = label;
		slot.m_Prefab = prefab;
		return slot;
	}
}

//! Equipment slot of the character inventory (binoculars, watch...).
[BaseContainerProps(), GDC_LoadoutEntryTitle("m_sSlotName", "m_Prefab")]
class GDC_LoadoutGadgetSlot
{
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sSlotName;

	[Attribute(desc: "Item in this equipment slot. Leave empty to remove it.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	static GDC_LoadoutGadgetSlot Create(string slotName, ResourceName prefab)
	{
		GDC_LoadoutGadgetSlot slot = new GDC_LoadoutGadgetSlot();
		slot.m_sSlotName = slotName;
		slot.m_Prefab = prefab;
		return slot;
	}
}

[BaseContainerProps(), GDC_LoadoutItemTitle()]
class GDC_LoadoutItem
{
	[Attribute(desc: "Item prefab", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_Prefab;

	[Attribute(defvalue: "1", desc: "Number of copies. 0 removes the item.", params: "0 inf 1")]
	int m_iCount;

	static GDC_LoadoutItem Create(ResourceName prefab, int count)
	{
		GDC_LoadoutItem item = new GDC_LoadoutItem();
		item.m_Prefab = prefab;
		item.m_iCount = count;
		return item;
	}
}

//! Inventory container of the loadout: one storage of the worn gear, the automatic placement, or a destination not found on the current gear.
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sLabel")]
class GDC_LoadoutContainer
{
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sLabel;

	//! TargetStorage value written in the prefab.
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sTargetStorage;

	//! TargetPurpose value written in the prefab.
	[Attribute(uiwidget: UIWidgets.None)]
	int m_iTargetPurpose;

	//! Path of the storage node this container is bound to, empty for automatic placement or unknown destinations.
	[Attribute(uiwidget: UIWidgets.None)]
	string m_sNodePath;

	[Attribute(desc: "Items spawned in this container")]
	ref array<ref GDC_LoadoutItem> m_aItems;

	//! Indices of the InitialInventoryItems entries this container was read from.
	[Attribute(uiwidget: UIWidgets.None)]
	ref array<int> m_aSourceIndices;

	void GDC_LoadoutContainer()
	{
		if (!m_aItems)
			m_aItems = {};

		if (!m_aSourceIndices)
			m_aSourceIndices = {};
	}

	static string MakeKey(string path, int purpose)
	{
		ResourceName pathName = path;
		string key = pathName.GetPath();
		key.ToLower();
		int keyPurpose = purpose;
		if (GDC_LoadoutUtils.IsGeneralPurpose(purpose))
			keyPurpose = 0;

		return string.Format("%1|%2", key, keyPurpose);
	}

	string GetKey()
	{
		if (!m_sNodePath.IsEmpty())
			return MakeKey(m_sNodePath, m_iTargetPurpose);

		return MakeKey(m_sTargetStorage, m_iTargetPurpose);
	}

	bool IsAutomatic()
	{
		return m_sNodePath.IsEmpty() && m_sTargetStorage.IsEmpty();
	}

	void AddSource(int index, string targetStorage, int targetPurpose)
	{
		if (m_aSourceIndices.IsEmpty())
		{
			m_sTargetStorage = targetStorage;
			m_iTargetPurpose = targetPurpose;
		}

		m_aSourceIndices.Insert(index);
	}

	void AddItem(ResourceName prefab, int count = 1)
	{
		if (prefab.IsEmpty() || count <= 0)
			return;

		foreach (GDC_LoadoutItem item : m_aItems)
		{
			if (item && item.m_Prefab == prefab)
			{
				item.m_iCount += count;
				return;
			}
		}

		m_aItems.Insert(GDC_LoadoutItem.Create(prefab, count));
	}

	//! Every item copy, as written in PrefabsToSpawn.
	void CollectPrefabs(notnull array<ResourceName> outPrefabs)
	{
		foreach (GDC_LoadoutItem item : m_aItems)
		{
			if (!item || item.m_Prefab.IsEmpty())
				continue;

			for (int i = 0; i < item.m_iCount; i++)
			{
				outPrefabs.Insert(item.m_Prefab);
			}
		}
	}

	int CountItems()
	{
		int total;
		foreach (GDC_LoadoutItem item : m_aItems)
		{
			if (item && !item.m_Prefab.IsEmpty() && item.m_iCount > 0)
				total += item.m_iCount;
		}

		return total;
	}
}

//! Confirmation before a destructive write; Cancel is the default button.
class GDC_LoadoutConfirmDialog
{
	[ButtonAttribute("Write anyway")]
	protected int ButtonWrite()
	{
		return 1;
	}

	[ButtonAttribute("Cancel", true)]
	protected int ButtonCancel()
	{
		return 0;
	}
}

//! Buttons of the report dialog.
class GDC_LoadoutReportDialog
{
	protected string m_sReport;

	void GDC_LoadoutReportDialog(string report)
	{
		m_sReport = report;
	}

	[ButtonAttribute("Copy To Log")]
	void ButtonCopyToLog()
	{
		Print(m_sReport);
	}

	[ButtonAttribute("Close", true)]
	bool ButtonClose()
	{
		return false;
	}
}
#endif
