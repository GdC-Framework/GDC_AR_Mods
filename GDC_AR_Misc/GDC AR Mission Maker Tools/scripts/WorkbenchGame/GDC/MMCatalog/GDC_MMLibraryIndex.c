#ifdef WORKBENCH
//! Index of the MM library prefabs coming from an addon (not from vanilla), written by GDC_MMCatalogGeneratorPlugin
//! into the GDC Mission Maker Libraries addon. It lets GDC_MMCatalogDropPlugin name the addon to load when a library prefab is dropped
//! while its parent's addon is not loaded: the parent prefab, and so the addon, cannot be read then.
[BaseContainerProps(configRoot: true)]
class GDC_MMLibraryIndex
{
	static const string INDEX = "Configs/MM Libraries/LibraryAddons.conf";

	protected static const string VANILLA_ADDON_ID = "ArmaReforger";

	//! Prefab path -> GUID of the first non-vanilla loaded addon holding that file, see FindSourceAddon
	protected static ref map<string, string> s_mFileAddons;
	protected static string s_sVanillaGUID;

	[Attribute()]
	ref array<ref GDC_MMLibraryIndexEntry> m_aEntries;

	//------------------------------------------------------------------------------------------------
	//! \return entry of the given library prefab, null when unknown or vanilla
	static GDC_MMLibraryIndexEntry Find(ResourceName libraryPrefab)
	{
		Resource resource = Resource.Load(INDEX);
		if (!resource.IsValid())
			return null;

		BaseContainerList entries = resource.GetResource().ToBaseContainer().GetObjectArray("m_aEntries");
		if (!entries)
			return null;

		string path = libraryPrefab.GetPath();
		for (int i, count = entries.Count(); i < count; i++)
		{
			BaseContainer container = entries.Get(i);
			string prefab;
			container.Get("m_sPrefab", prefab);
			if (prefab != path)
				continue;

			GDC_MMLibraryIndexEntry entry = new GDC_MMLibraryIndexEntry();
			entry.m_sPrefab = prefab;
			container.Get("m_sAddonID", entry.m_sAddonID);
			container.Get("m_sAddonTitle", entry.m_sAddonTitle);
			return entry;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] libraryPrefab library prefab path, PrefabLibrary/MM .../...
	//! \param[in] prefab its parent
	//! \return entry naming the addon the parent comes from, null for a vanilla prefab (Arma Reforger, core) or when unknown
	static GDC_MMLibraryIndexEntry CreateEntry(string libraryPrefab, ResourceName prefab)
	{
		string addonID, addonTitle;
		bool isVanilla;
		if (!FindSourceAddon(prefab, addonID, addonTitle, isVanilla) || isVanilla)
			return null;

		GDC_MMLibraryIndexEntry entry = new GDC_MMLibraryIndexEntry();
		entry.m_sPrefab = libraryPrefab;
		entry.m_sAddonID = addonID;
		entry.m_sAddonTitle = addonTitle;
		return entry;
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the loaded addon defining the given prefab: the first non-vanilla addon, in load order, holding its file,
	//! else Arma Reforger. BaseContainer.GetSourceAddons() cannot tell it: it gives Arma Reforger first for RHS prefabs.
	//! A mod overriding a vanilla prefab file is taken as its source, which is harmless: without that mod, the vanilla
	//! prefab still loads.
	//! \param[out] isVanilla true for Arma Reforger or core
	//! \return false when no loaded addon is found
	static bool FindSourceAddon(ResourceName prefab, out string addonID, out string addonTitle, out bool isVanilla)
	{
		if (!s_mFileAddons)
			BuildFileAddons();

		string guid = s_mFileAddons.Get(prefab.GetPath());
		isVanilla = guid.IsEmpty();
		if (isVanilla)
			guid = s_sVanillaGUID;

		if (guid.IsEmpty())
			return false;

		addonID = GameProject.GetAddonID(guid);
		addonTitle = GameProject.GetAddonTitle(guid);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Forgets the files of the loaded addons, read again by the next FindSourceAddon (call it at each generation).
	static void ResetFileAddons()
	{
		s_mFileAddons = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void BuildFileAddons()
	{
		s_mFileAddons = new map<string, string>();
		s_sVanillaGUID = string.Empty;

		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);
		foreach (string guid : guids)
		{
			if (GameProject.IsVanillaAddon(guid))
			{
				if (GameProject.GetAddonID(guid) == VANILLA_ADDON_ID)
					s_sVanillaGUID = guid;

				continue;
			}

			foreach (ResourceName file : SCR_WorkbenchHelper.SearchWorkbenchResources({ "et" }, null, "$" + GameProject.GetAddonID(guid) + ":"))
			{
				string path = file.GetPath();
				if (!s_mFileAddons.Contains(path))
					s_mFileAddons.Insert(path, guid);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Writes the index (sorted, to keep diffs readable) into the library addon at addonPath; the caller registers it.
	static bool Save(string addonPath, notnull array<ref GDC_MMLibraryIndexEntry> entries)
	{
		string absolutePath = addonPath + "/" + INDEX;
		FileIO.MakeDirectory(FilePath.StripFileName(absolutePath));
		FileHandle file = FileIO.OpenFile(absolutePath, FileMode.WRITE);
		if (!file)
			return false;

		map<string, GDC_MMLibraryIndexEntry> byPrefab = new map<string, GDC_MMLibraryIndexEntry>();
		array<string> prefabs = {};
		foreach (GDC_MMLibraryIndexEntry entry : entries)
		{
			byPrefab.Set(entry.m_sPrefab, entry);
			prefabs.Insert(entry.m_sPrefab);
		}

		prefabs.Sort();

		file.WriteLine("GDC_MMLibraryIndex {");
		file.WriteLine(" m_aEntries {");
		foreach (string prefab : prefabs)
		{
			GDC_MMLibraryIndexEntry entry = byPrefab.Get(prefab);
			file.WriteLine("  GDC_MMLibraryIndexEntry {");
			file.WriteLine(string.Format("   m_sPrefab \"%1\"", entry.m_sPrefab));
			file.WriteLine(string.Format("   m_sAddonID \"%1\"", entry.m_sAddonID));
			file.WriteLine(string.Format("   m_sAddonTitle \"%1\"", entry.m_sAddonTitle));
			file.WriteLine("  }");
		}

		file.WriteLine(" }");
		file.WriteLine("}");
		file.Close();
		return true;
	}
}

[BaseContainerProps()]
class GDC_MMLibraryIndexEntry
{
	//! Library prefab path, PrefabLibrary/MM .../...
	[Attribute()]
	string m_sPrefab;

	[Attribute()]
	string m_sAddonID;

	[Attribute()]
	string m_sAddonTitle;
}
#endif
