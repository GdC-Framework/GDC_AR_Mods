#ifdef WORKBENCH
//! One map marker prefab found in the loaded addons.
class GDC_MMMarkerEntry
{
	ResourceName m_Prefab;

	//! Entity class of the prefab, needed to write a child prefab of it.
	string m_sClassName;

	//! Area drawn with a closed polyline, rather than an icon.
	bool m_bArea;
}

//! Finds the map marker prefabs of the loaded addons, for the MM Markers library: briefing indications and
//! reference entities of modules (support areas...). Addons share no folder convention for them, so prefabs are
//! selected by class. Classes are given by name, as the addons defining them (Reforger Lobby, Coalition Lobby, TILW,
//! GDC AR Lib) are optional and may come and go: the class of an addon not loaded is ignored.
//! Vanilla has no map marker meant for mission makers, so it is not scanned.
class GDC_MMMarkerReader
{
	//! Icon markers, children included: Reforger Lobby (PSCore), Coalition Lobby
	protected static const ref array<string> ICON_CLASSES = { "PS_ManualMarker", "COA_ManualMarker" };

	//! Areas: polylines carrying one of these components (GDC AR Lib area, TILW map shape, cover map and AO limit)
	protected static const ref array<string> AREA_COMPONENTS = { "GDC_MarkerAreaComponent", "TILW_MapShapeComponent", "TILW_CoverMapComponent", "TILW_AOLimitComponent" };

	//------------------------------------------------------------------------------------------------
	//! Fills result with the marker prefabs of the loaded addons, except excludedAddonID (the libraries addon).
	static void Read(string excludedAddonID, notnull array<ref GDC_MMMarkerEntry> result)
	{
		array<typename> iconTypes = {};
		foreach (string className : ICON_CLASSES)
		{
			typename iconType = className.ToType();
			if (iconType)
				iconTypes.Insert(iconType);
		}

		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);

		set<ResourceName> known = new set<ResourceName>(); // a prefab modified by another addon is found in both
		int scanned;
		foreach (string guid : guids)
		{
			string addonID = GameProject.GetAddonID(guid);
			if (GameProject.IsVanillaAddon(guid) || addonID == excludedAddonID)
				continue;

			foreach (ResourceName prefab : SCR_WorkbenchHelper.SearchWorkbenchResources({ "et" }, null, "$" + addonID + ":"))
			{
				// prefab libraries only hold shortcuts to prefabs found elsewhere
				if (prefab.GetPath().StartsWith("PrefabLibrary/") || known.Contains(prefab))
					continue;

				known.Insert(prefab);
				scanned++;
				ReadPrefab(prefab, iconTypes, result);
			}
		}

		PrintFormat("[MM Libraries] %1 markers found in %2 prefabs.", result.Count(), scanned);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReadPrefab(ResourceName prefab, array<typename> iconTypes, array<ref GDC_MMMarkerEntry> result)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource.IsValid())
			return;

		BaseContainer container = resource.GetResource().ToBaseContainer();
		if (!container)
			return;

		string className = container.GetClassName();
		typename type = className.ToType();
		if (!type)
			return;

		bool area = type.IsInherited(PolylineShapeEntity) && HasAreaComponent(resource);
		if (!area && !IsIconType(type, iconTypes))
			return;

		GDC_MMMarkerEntry entry = new GDC_MMMarkerEntry();
		entry.m_Prefab = prefab;
		entry.m_sClassName = className;
		entry.m_bArea = area;
		result.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasAreaComponent(Resource resource)
	{
		foreach (string componentClassName : AREA_COMPONENTS)
		{
			if (SCR_BaseContainerTools.FindComponentSource(resource, componentClassName))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsIconType(typename type, array<typename> iconTypes)
	{
		foreach (typename iconType : iconTypes)
		{
			if (type.IsInherited(iconType))
				return true;
		}

		return false;
	}
}
#endif
