#ifdef WORKBENCH
//! Shared helpers of the GDC loadout editor.
class GDC_LoadoutUtils
{
	//! Separates the name part (upper case) from the details part (lower case) of panel titles.
	static const string SEPARATOR = "  ·  ";

	//! Title icons: an item will not be spawned (capacity, weight, size, type), storage not on the worn gear, items placed by the game.
	static const string ICON_OVER_LIMIT = "❗";
	static const string ICON_WARNING = "⚠";
	static const string ICON_AUTOMATIC = "🟡";

	//! Dialog icons, shown at the start of the text: Workbench dialogs have no icon option.
	static const string DIALOG_WARNING = "⚠️";
	static const string DIALOG_INFORMATION = "ℹ️";
	static const string DIALOG_QUESTION = "❓";

	static string DialogText(string icon, string text)
	{
		return icon + "  " + text;
	}

	static string Upper(string text)
	{
		string result = text;
		result.ToUpper();
		return result;
	}

	//! "equipment attachment" for PURPOSE_EQUIPMENT_ATTACHMENT, combinations joined with " + ".
	static string ReadablePurpose(int purpose)
	{
		string name = PurposeName(purpose);
		name.Replace("PURPOSE_", string.Empty);
		name.Replace("_", " ");
		name.ToLower();
		return name;
	}

	//! True when an item of the given dimensions fits within the size limit, whatever its orientation.
	//! A limit of zero on every axis means no limit.
	static bool FitsIn(vector dimensions, vector maxSize)
	{
		if (maxSize[0] <= 0 && maxSize[1] <= 0 && maxSize[2] <= 0)
			return true;

		array<float> item = { dimensions[0], dimensions[1], dimensions[2] };
		array<float> limit = { maxSize[0], maxSize[1], maxSize[2] };
		item.Sort();
		limit.Sort();
		for (int axis = 0; axis < 3; axis++)
		{
			if (item[axis] > limit[axis])
				return false;
		}

		return true;
	}

	static string ItemCountText(int count)
	{
		if (count == 1)
			return "1 item";

		return string.Format("%1 items", count);
	}

	//! File name of a prefab without extension, or "empty".
	static string ShortName(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return "empty";

		return FilePath.StripExtension(FilePath.StripPath(prefab.GetPath()));
	}

	//! Removes a leading "{GUID}" from a path. Done on the text: storage paths such as "Vest/Vest.et/radio/Pouch.et"
	//! are not resources, and converting them to ResourceName makes the engine log a "Wrong GUID/name" error.
	static string StripGuid(string path)
	{
		if (!path.StartsWith("{"))
			return path;

		int closing = path.IndexOf("}");
		if (closing < 0)
			return path;

		return path.Substring(closing + 1, path.Length() - closing - 1);
	}

	//! Case-insensitive comparison, GUID prefixes ignored.
	static bool SamePath(string left, string right)
	{
		string leftPath = StripGuid(left);
		string rightPath = StripGuid(right);
		leftPath.ToLower();
		rightPath.ToLower();
		return leftPath == rightPath;
	}

	//! Purposes that designate the regular storage of an item (as opposed to special storages such as flashlight holders).
	//! StoragePurpose is a bit field: a storage flagged for regular storage plus other uses (e.g. 0x41 on RHS pouches) is still regular.
	static bool IsGeneralPurpose(int purpose)
	{
		if (purpose == 0 || purpose == 1)
			return true;

		int generalBits = EStoragePurpose.PURPOSE_UNSET | EStoragePurpose.PURPOSE_DEPOSIT;
		return (purpose & generalBits) != 0;
	}

	static bool SamePurpose(int left, int right)
	{
		if (IsGeneralPurpose(left) && IsGeneralPurpose(right))
			return true;

		return left == right;
	}

	//! Enum name of a purpose, or the names of its bits joined with " + " for a combination.
	static string PurposeName(int purpose)
	{
		array<int> values = {
			EStoragePurpose.PURPOSE_UNSET,
			EStoragePurpose.PURPOSE_DEPOSIT,
			EStoragePurpose.PURPOSE_WEAPON_PROXY,
			EStoragePurpose.PURPOSE_GADGET_PROXY,
			EStoragePurpose.PURPOSE_LOADOUT_PROXY,
			EStoragePurpose.PURPOSE_ATTACHMENT_PROXY,
			EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT,
			EStoragePurpose.PURPOSE_ANY
		};

		string names;
		foreach (int value : values)
		{
			if (value == purpose)
				return typename.EnumToString(EStoragePurpose, value);

			if (value == 0 || (purpose & value) != value)
				continue;

			if (!names.IsEmpty())
				names += " + ";

			names += typename.EnumToString(EStoragePurpose, value);
		}

		if (names.IsEmpty())
			return string.Format("purpose %1", purpose);

		return names;
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

//! Title "LABEL  ·  prefab file name" for panel entries holding one prefab.
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

		title = GDC_LoadoutUtils.Upper(label) + GDC_LoadoutUtils.SEPARATOR + GDC_LoadoutUtils.ShortName(prefab);
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
		string key = GDC_LoadoutUtils.StripGuid(path);
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

//! Offered after Apply: the tool changes the prefab in memory, the Workbench save writes the file.
class GDC_LoadoutSaveDialog
{
	[ButtonAttribute("Save world", true)]
	protected int ButtonSave()
	{
		return 1;
	}

	[ButtonAttribute("Later")]
	protected int ButtonLater()
	{
		return 0;
	}
}

//! Confirmation before emptying the inventory; Cancel is the default button.
class GDC_LoadoutClearDialog
{
	[ButtonAttribute("Clear")]
	protected int ButtonClear()
	{
		return 1;
	}

	[ButtonAttribute("Cancel", true)]
	protected int ButtonCancel()
	{
		return 0;
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
#endif
