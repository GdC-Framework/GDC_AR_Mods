#ifdef WORKBENCH
//! One prefab found in the entity catalog of a faction.
class GDC_MMCatalogEntry
{
	ResourceName m_Prefab;
	EEntityCatalogType m_eType;
	string m_sFactionKey;

	//! Entity class of the prefab, needed to write a child prefab of it.
	string m_sClassName;

	//! Vehicle flying (helicopter, airplane) rather than moving on land or water.
	bool m_bAircraft;
}

//! Reads the prefabs listed in the entity catalogs of every faction config (Configs/Factions/*.conf) of the loaded addons.
//! These catalogs are the lists Bohemia and faction addons (RHS...) maintain for the Game Master, so they only hold usable prefabs.
class GDC_MMFactionCatalogReader
{
	protected static const string FACTIONS_DIRECTORY = "Configs/Factions/";

	//------------------------------------------------------------------------------------------------
	//! Fills result with the enabled entries of the given catalog types, each prefab once per faction and type.
	static void Read(array<EEntityCatalogType> types, notnull array<ref GDC_MMCatalogEntry> result)
	{
		array<ResourceName> factionConfigs = {};
		foreach (ResourceName config : SCR_WorkbenchHelper.SearchWorkbenchResources({ "conf" }))
		{
			if (config.GetPath().StartsWith(FACTIONS_DIRECTORY))
				factionConfigs.Insert(config);
		}

		set<string> known = new set<string>();
		foreach (ResourceName config : factionConfigs)
		{
			ReadFaction(config, types, known, result);
		}

		PrintFormat("[MM Libraries] %1 prefabs read from %2 faction configs.", result.Count(), factionConfigs.Count());
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReadFaction(ResourceName config, array<EEntityCatalogType> types, set<string> known, array<ref GDC_MMCatalogEntry> result)
	{
		Resource resource = Resource.Load(config);
		if (!resource.IsValid())
			return;

		BaseContainer faction = resource.GetResource().ToBaseContainer();
		if (!faction || !faction.GetClassName().ToType() || !faction.GetClassName().ToType().IsInherited(SCR_Faction))
			return;

		string factionKey;
		faction.Get("FactionKey", factionKey);
		if (factionKey.IsEmpty())
			return;

		BaseContainerList catalogs = faction.GetObjectArray("m_aEntityCatalogs");
		if (!catalogs)
			return;

		for (int i, count = catalogs.Count(); i < count; i++)
		{
			BaseContainer catalog = catalogs.Get(i);
			EEntityCatalogType type;
			if (!catalog.Get("m_eEntityCatalogType", type) || !types.Contains(type))
				continue;

			ReadEntries(catalog.GetObjectArray("m_aEntityEntryList"), type, factionKey, known, result);

			// multi-list catalogs keep part of their entries in sub-lists
			BaseContainerList multiLists = catalog.GetObjectArray("m_aMultiLists");
			if (!multiLists)
				continue;

			for (int j, multiCount = multiLists.Count(); j < multiCount; j++)
			{
				ReadEntries(multiLists.Get(j).GetObjectArray("m_aEntities"), type, factionKey, known, result);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReadEntries(BaseContainerList entries, EEntityCatalogType type, string factionKey, set<string> known, array<ref GDC_MMCatalogEntry> result)
	{
		if (!entries)
			return;

		for (int i, count = entries.Count(); i < count; i++)
		{
			BaseContainer entryContainer = entries.Get(i);
			bool enabled;
			if (!entryContainer.Get("m_bEnabled", enabled) || !enabled)
				continue;

			ResourceName prefab;
			if (!entryContainer.Get("m_sEntityPrefab", prefab) || prefab.IsEmpty())
				continue;

			// several configs share a faction key (campaign variants...): keep each prefab once per faction and type
			string key = string.Format("%1|%2|%3", type, factionKey, prefab);
			if (known.Contains(key))
				continue;

			known.Insert(key);

			Resource prefabResource = Resource.Load(prefab);
			if (!prefabResource.IsValid())
				continue;

			BaseContainer prefabContainer = prefabResource.GetResource().ToBaseContainer();

			GDC_MMCatalogEntry entry = new GDC_MMCatalogEntry();
			entry.m_Prefab = prefab;
			entry.m_eType = type;
			entry.m_sFactionKey = factionKey;
			entry.m_sClassName = prefabContainer.GetClassName();
			entry.m_bAircraft = type == EEntityCatalogType.VEHICLE && IsAircraft(prefab, prefabContainer);
			result.Insert(entry);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Helicopters carry the helicopter simulation; the folder convention covers the other aircraft.
	protected static bool IsAircraft(ResourceName prefab, BaseContainer prefabContainer)
	{
		if (SCR_BaseContainerTools.FindComponentSource(prefabContainer, VehicleHelicopterSimulation))
			return true;

		string path = prefab.GetPath();
		return path.Contains("/Helicopters/") || path.Contains("/Airplanes/");
	}
}
#endif
