#ifdef WORKBENCH
//! Loadout of a character prefab, read and checked the way the Loadout Editor shows it. Shared by the Loadout Editor
//! and Check Mission (rules 3.6 to 3.10): one reading of the prefab and one capacity check for both tools.
class GDC_LoadoutAnalysis
{
	static const string NO_SPARE_MAGAZINE = "⚠ NO SPARE MAGAZINE"; //!< GDC_LoadoutUtils.ICON_WARNING
	protected static const string MAGAZINE_WELL = "MagazineWell";
	protected static const string PRIMARY_WEAPON_INDEX = ":0"; //!< key suffix of the primary weapon slot (WeaponSlotIndex 0)

	ref GDC_LoadoutSnapshot m_Snapshot = new GDC_LoadoutSnapshot();
	ref array<ref GDC_LoadoutWornSlot> m_aWorn = {};
	ref array<ref GDC_LoadoutWeaponSlot> m_aWeapons = {};
	ref array<ref GDC_LoadoutGadgetSlot> m_aGadgets = {};
	ref array<ref GDC_LoadoutStorageNode> m_aNodes = {};
	ref array<ref GDC_LoadoutContainer> m_aContainers = {};
	ref array<string> m_aWarnings = {};		//!< every remark, as the editor prints them
	ref array<string> m_aOverLimit = {};	//!< storages whose items will not all be spawned (capacity, weight, size, type)
	ref array<string> m_aNotOnGear = {};	//!< items targeting a storage that is not on the worn gear
	ref array<ResourceName> m_aMissing = {};	//!< prefabs of the loadout no loaded addon provides
	float m_fMaxWeight;						//!< weight limit of the character's inventory, 0 when none

	//------------------------------------------------------------------------------------------------
	//! \return the analysis of the prefab, null when it cannot be loaded or is not a character
	static GDC_LoadoutAnalysis ReadPrefab(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		IEntitySource source = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!source)
			return null;

		GDC_LoadoutAnalysis analysis = new GDC_LoadoutAnalysis();
		if (!analysis.Read(source, prefab))
			return null;

		return analysis;
	}

	//------------------------------------------------------------------------------------------------
	//! Reads the worn gear, weapons, equipment slots and inventory of the prefab, then checks them.
	bool Read(notnull IEntitySource source, ResourceName prefab)
	{
		if (!GDC_LoadoutPrefabIO.Read(source, prefab, m_Snapshot))
			return false;

		m_aWorn.Clear();
		foreach (GDC_LoadoutSlotRef worn : m_Snapshot.m_aWorn)
		{
			m_aWorn.Insert(GDC_LoadoutWornSlot.Create(worn.m_sKey, worn.m_Prefab));
		}

		m_aWeapons.Clear();
		foreach (GDC_LoadoutSlotRef weapon : m_Snapshot.m_aWeapons)
		{
			m_aWeapons.Insert(GDC_LoadoutWeaponSlot.Create(weapon.m_sKey, weapon.m_sLabel, weapon.m_Prefab));
		}

		m_aGadgets.Clear();
		foreach (GDC_LoadoutSlotRef gadget : m_Snapshot.m_aGadgets)
		{
			m_aGadgets.Insert(GDC_LoadoutGadgetSlot.Create(gadget.m_sKey, gadget.m_Prefab));
		}

		GDC_LoadoutStorageTree.CollectNodes(m_aWorn, m_aNodes);
		GDC_LoadoutStorageTree.BuildContainers(m_Snapshot.m_aEntries, m_aNodes, m_aContainers);
		Check(m_aContainers, m_aNodes, m_aWeapons, m_aWarnings, m_aOverLimit, m_aNotOnGear);
		CollectMissingPrefabs(m_Snapshot, m_aMissing);

		m_fMaxWeight = 0;
		int inventoryIndex = SCR_BaseContainerTools.FindComponentIndex(source, CharacterInventoryStorageComponent);
		if (inventoryIndex >= 0)
			source.GetComponent(inventoryIndex).Get("m_fMaxWeight", m_fMaxWeight);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \return weight of the worn gear, the weapons, the equipment slots and the inventory items, in kg; unknown weights
	//! count as 0 (the pouches attached to the gear are not counted)
	float GetTotalWeight()
	{
		GDC_LoadoutItemCache cache = new GDC_LoadoutItemCache();
		float total;
		foreach (GDC_LoadoutWornSlot worn : m_aWorn)
		{
			if (worn)
				total += Math.Max(cache.Get(worn.m_Prefab).m_fWeight, 0);
		}

		foreach (GDC_LoadoutWeaponSlot weapon : m_aWeapons)
		{
			if (weapon)
				total += Math.Max(cache.Get(weapon.m_Prefab).m_fWeight, 0);
		}

		foreach (GDC_LoadoutGadgetSlot gadget : m_aGadgets)
		{
			if (gadget)
				total += Math.Max(cache.Get(gadget.m_Prefab).m_fWeight, 0);
		}

		foreach (GDC_LoadoutContainer container : m_aContainers)
		{
			if (container)
				total += cache.UsedWeight(container);
		}

		return total;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefabs used by the loadout (worn gear, weapons, equipment slots, inventory items) that cannot be loaded,
	//! typically because their addon is not loaded.
	static void CollectMissingPrefabs(notnull GDC_LoadoutSnapshot snapshot, notnull array<ResourceName> outMissing)
	{
		array<ResourceName> used = {};
		foreach (GDC_LoadoutSlotRef worn : snapshot.m_aWorn)
		{
			used.Insert(worn.m_Prefab);
		}

		foreach (GDC_LoadoutSlotRef weapon : snapshot.m_aWeapons)
		{
			used.Insert(weapon.m_Prefab);
		}

		foreach (GDC_LoadoutSlotRef gadget : snapshot.m_aGadgets)
		{
			used.Insert(gadget.m_Prefab);
		}

		foreach (GDC_LoadoutEntryRef entry : snapshot.m_aEntries)
		{
			used.InsertAll(entry.m_aPrefabs);
		}

		outMissing.Clear();
		foreach (ResourceName candidate : used)
		{
			if (candidate.IsEmpty() || outMissing.Contains(candidate))
				continue;

			Resource resource = Resource.Load(candidate);
			if (!resource || !resource.IsValid())
				outMissing.Insert(candidate);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return spare magazines of the primary weapon in the inventory; -1 without a primary weapon or when it takes
	//! no magazine (single-shot launcher, unknown weapon)
	int GetPrimarySpareMagazines()
	{
		return CountSpareMagazines(FindPrimaryWeapon(m_aWeapons), m_aContainers, new map<string, string>());
	}

	//------------------------------------------------------------------------------------------------
	//! \return the primary weapon, empty without one
	ResourceName GetPrimaryWeapon()
	{
		GDC_LoadoutWeaponSlot primary = FindPrimaryWeapon(m_aWeapons);
		if (!primary)
			return ResourceName.Empty;

		return primary.m_Prefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Capacity check of every container (labels and warnings) and spare magazine count of the primary weapon (its
	//! label, a warning when there is none). The Loadout Editor runs it after every change.
	//! \param outOverLimit receives the warnings of the storages that will not spawn every item (may be null)
	//! \param outNotOnGear receives the warnings of the items targeting a storage not on the gear (may be null)
	static void Check(
		notnull array<ref GDC_LoadoutContainer> containers,
		notnull array<ref GDC_LoadoutStorageNode> nodes,
		notnull array<ref GDC_LoadoutWeaponSlot> weapons,
		notnull array<string> outWarnings,
		array<string> outOverLimit,
		array<string> outNotOnGear = null)
	{
		outWarnings.Clear();
		if (outOverLimit)
			outOverLimit.Clear();

		if (outNotOnGear)
			outNotOnGear.Clear();

		GDC_LoadoutStorageTree.UpdateLabels(containers, nodes, new GDC_LoadoutItemCache(), outWarnings, outOverLimit, outNotOnGear);

		map<string, string> wells = new map<string, string>();
		foreach (GDC_LoadoutWeaponSlot weapon : weapons)
		{
			if (!weapon)
				continue;

			string label = BaseLabel(weapon.m_sLabel);
			if (weapon == FindPrimaryWeapon(weapons))
			{
				int spare = CountSpareMagazines(weapon, containers, wells);
				if (spare == 0)
				{
					label += GDC_LoadoutUtils.SEPARATOR + NO_SPARE_MAGAZINE;
					outWarnings.Insert(string.Format("%1: no spare magazine for %2 in the inventory.", BaseLabel(weapon.m_sLabel), GDC_LoadoutUtils.ShortName(weapon.m_Prefab)));
				}
				else if (spare == 1)
				{
					label += GDC_LoadoutUtils.SEPARATOR + "1 spare magazine";
				}
				else if (spare > 1)
				{
					label += GDC_LoadoutUtils.SEPARATOR + string.Format("%1 spare magazines", spare);
				}
			}

			weapon.m_sLabel = label;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return the label without what Check appended ("Weapon slot 0 (primary)")
	protected static string BaseLabel(string label)
	{
		int separator = label.IndexOf(GDC_LoadoutUtils.SEPARATOR);
		if (separator < 0)
			return label;

		return label.Substring(0, separator);
	}

	//------------------------------------------------------------------------------------------------
	//! \return the weapon slot of index 0 holding a weapon, null if none
	protected static GDC_LoadoutWeaponSlot FindPrimaryWeapon(notnull array<ref GDC_LoadoutWeaponSlot> weapons)
	{
		foreach (GDC_LoadoutWeaponSlot weapon : weapons)
		{
			if (weapon && !weapon.m_Prefab.IsEmpty() && weapon.m_sKey.EndsWith(PRIMARY_WEAPON_INDEX))
				return weapon;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! A magazine fits when its MagazineWell class inherits the one of the weapon's first muzzle.
	//! \return number of fitting magazines in the containers, -1 when there is no weapon or it takes no magazine
	protected static int CountSpareMagazines(GDC_LoadoutWeaponSlot weapon, notnull array<ref GDC_LoadoutContainer> containers, notnull map<string, string> wells)
	{
		if (!weapon)
			return -1;

		typename weaponWell = GetWell(weapon.m_Prefab, wells).ToType();
		if (!weaponWell)
			return -1;

		int count;
		foreach (GDC_LoadoutContainer container : containers)
		{
			if (!container)
				continue;

			foreach (GDC_LoadoutItem item : container.m_aItems)
			{
				if (!item || item.m_iCount <= 0)
					continue;

				typename itemWell = GetWell(item.m_Prefab, wells).ToType();
				if (itemWell && itemWell.IsInherited(weaponWell))
					count += item.m_iCount;
			}
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! \return class of the first MagazineWell of the prefab (a magazine's, or the first muzzle of a weapon), empty if
	//! none; read once per prefab
	protected static string GetWell(ResourceName prefab, notnull map<string, string> wells)
	{
		string well;
		if (wells.Find(prefab, well))
			return well;

		Resource resource = Resource.Load(prefab);
		if (resource && resource.IsValid())
		{
			IEntitySource source = SCR_BaseContainerTools.FindEntitySource(resource);
			if (source)
			{
				for (int i = 0, count = source.GetComponentCount(); i < count && well.IsEmpty(); i++)
				{
					well = FindWell(source.GetComponent(i));
				}
			}
		}

		wells.Insert(prefab, well);
		return well;
	}

	//------------------------------------------------------------------------------------------------
	//! Depth first in the container and its sub-components ("components"), in their order.
	protected static string FindWell(BaseContainer container)
	{
		if (container.GetVarIndex(MAGAZINE_WELL) >= 0)
		{
			BaseContainer well = container.GetObject(MAGAZINE_WELL);
			if (well)
				return well.GetClassName();
		}

		if (container.GetVarIndex("components") < 0)
			return string.Empty;

		BaseContainerList children = container.GetObjectArray("components");
		if (!children)
			return string.Empty;

		for (int c = 0, count = children.Count(); c < count; c++)
		{
			BaseContainer child = children.Get(c);
			if (!child)
				continue;

			string found = FindWell(child);
			if (!found.IsEmpty())
				return found;
		}

		return string.Empty;
	}
}
#endif
