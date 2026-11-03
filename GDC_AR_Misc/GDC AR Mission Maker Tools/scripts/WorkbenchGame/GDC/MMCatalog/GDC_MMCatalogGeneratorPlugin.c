#ifdef WORKBENCH
//! One prefab of an MM library to write: an empty child prefab of m_Prefab at m_sPath.
class GDC_MMLibraryItem
{
	string m_sPath;
	ResourceName m_Prefab;
	string m_sClassName;
}

//! Mission maker plugin generating the MM prefab libraries shown in the World Editor's Prefab Library panel,
//! one empty child prefab per usable prefab of the loaded addons:
//! - MM Forces: PrefabLibrary/MM Forces/<Category>/<Faction>/, from the faction catalogs;
//! - MM Markers: PrefabLibrary/MM Markers/<Icons|Areas>/<Addon>/, see GDC_MMMarkerReader;
//! - MM Systems: PrefabLibrary/MM Systems/<Waypoints|Triggers|Systems>/<Addon>/, see GDC_MMSystemsReader.
//! Dropping one into the viewport places its parent (see GDC_MMCatalogDropPlugin).
//! The libraries go into a local addon of the mission maker, "GDC AR MM Libraries" (GDC AR Mission Maker Tools is packed, so read-only),
//! created by the first run; regenerating it mirrors the loaded addons. Never published, missions do not depend on it.
//! Also writes GDC_MMLibraryIndex, the addon each library prefab comes from, named to mission makers dropping it without that addon.
[WorkbenchPluginAttribute(
	name: "Update MM Libraries",
	description: "Create or update the MM prefab libraries from the loaded addons (run it again after changing them)",
	wbModules: { "WorldEditor", "ResourceManager" },
	category: "GDC/Mission Maker",
	awesomeFontCode: 0xF0CB)]
class GDC_MMCatalogGeneratorPlugin : WorkbenchPlugin
{
	protected static const string LOG_PREFIX = "[MM Libraries] ";
	protected static const string CAPTION = "Update MM Libraries";
	protected static const string FORCES_DIRECTORY = "PrefabLibrary/MM Forces/";
	protected static const string MARKERS_DIRECTORY = "PrefabLibrary/MM Markers/";
	protected static const string SYSTEMS_DIRECTORY = "PrefabLibrary/MM Systems/";
	protected static const ref array<string> LIBRARY_DIRECTORIES = { FORCES_DIRECTORY, MARKERS_DIRECTORY, SYSTEMS_DIRECTORY };

	//! Local addon receiving the libraries; depends on GDC AR Mission Maker Tools so that ticking it in the Launcher is enough
	protected static const string LIBRARY_ADDON_ID = "GDC AR MM Libraries";
	protected static const string LIBRARY_ADDON_TITLE = "GDC AR MM Libraries";
	protected static const string MMTOOLS_ADDON_GUID = "6AC540B09694CF5F";

	[Attribute(desc: "Folder of the new GDC AR MM Libraries addon. Keep the default unless you store your Workbench projects elsewhere.")]
	protected string m_sLibraryFolder;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		string addonPath;
		if (Workbench.GetAbsolutePath("$" + LIBRARY_ADDON_ID + ":", addonPath, true) && IsLibraryAddonLoaded())
		{
			Generate(addonPath, true);
			return;
		}

		CreateLibraryAddon();
	}

	//------------------------------------------------------------------------------------------------
	//! First run, or library addon not loaded: let the mission maker choose its folder, create it and explain how to load it.
	protected void CreateLibraryAddon()
	{
		if (m_sLibraryFolder.IsEmpty())
			m_sLibraryFolder = GetDefaultLibraryFolder();

		string text = "The " + LIBRARY_ADDON_TITLE + " addon is not loaded.\n\n"
			+ "If you have never created it: choose its folder below and click Create. The default folder is the one of your Workbench projects.\n\n"
			+ "If you have already created it: click Cancel, close the Workbench and open your project from the Launcher with \"Open with Addons\", ticking \"" + LIBRARY_ADDON_TITLE + "\".";
		if (!Workbench.ScriptDialog(CAPTION, text, this))
			return;

		string folder = m_sLibraryFolder.Trim();
		folder.Replace("\\", "/");
		while (folder.EndsWith("/"))
		{
			folder = folder.Substring(0, folder.Length() - 1);
		}

		if (folder.IsEmpty())
		{
			ShowError("no folder given.");
			return;
		}

		if (FileIO.FileExists(folder + "/addon.gproj"))
		{
			ShowError("an addon already exists in " + folder + ".\n\nIf it is your " + LIBRARY_ADDON_TITLE + ", close the Workbench and open your project from the Launcher with \"Open with Addons\", ticking \"" + LIBRARY_ADDON_TITLE + "\". Otherwise choose another folder.");
			return;
		}

		if (!FileIO.MakeDirectory(folder) || !WriteProjectFile(folder + "/addon.gproj"))
		{
			ShowError("cannot write in " + folder + ".");
			return;
		}

		// the addon is not loaded: files are registered when the Workbench opens it
		if (!Generate(folder, false))
			return;

		string message = LIBRARY_ADDON_TITLE + " created in:\n" + folder + "\n\n"
			+ "To see it in the Prefab Library panel:\n"
			+ "1. close the Workbench;\n"
			+ "2. in the Launcher, open your project with \"Open with Addons\" and tick \"" + LIBRARY_ADDON_TITLE + "\" (it brings GDC AR Mission Maker Tools with it).";

		if (!IsInDefaultAddonsFolder(folder))
			message += "\n\nThis folder is not the default one: first add it to the Launcher with \"Add Existing Project\" (its addon.gproj file).";

		message += "\n\nAfter changing your loaded addons, run Update MM Libraries again (Plugins > GDC > Mission Maker)."
			+ " Run it again as well if, once the addon is loaded, a library prefab cannot be dropped into the viewport (forbidden sign).";
		Workbench.Dialog(CAPTION, message);
	}

	//------------------------------------------------------------------------------------------------
	//! Writes the libraries and the index into the library addon at addonPath, mirroring the loaded addons.
	//! \param[in] loaded true when the library addon is loaded: new files are registered and obsolete ones deleted
	protected bool Generate(string addonPath, bool loaded)
	{
		GDC_MMLibraryIndex.ResetFileAddons(); // the loaded addons may have changed since the last run

		set<string> generated = new set<string>(); // relative paths, PrefabLibrary/...
		array<ref GDC_MMLibraryItem> items = {};
		CollectForces(items, generated);
		CollectMarkers(items, generated);
		CollectSystems(items, generated);
		if (items.IsEmpty())
		{
			ShowError("no prefab found in the loaded addons.");
			return false;
		}

		WBProgressDialog progress = new WBProgressDialog(CAPTION, Workbench.GetModule(ResourceManager));

		array<ref GDC_MMLibraryIndexEntry> indexEntries = {};
		int added, kept;
		array<string> failures = {};
		foreach (int index, GDC_MMLibraryItem item : items)
		{
			progress.SetProgress(index / (float)items.Count());

			string relativePath = item.m_sPath;

			GDC_MMLibraryIndexEntry indexEntry = GDC_MMLibraryIndex.CreateEntry(relativePath, item.m_Prefab);
			if (indexEntry)
				indexEntries.Insert(indexEntry);

			string absolutePath = addonPath + "/" + relativePath;

			// an existing file keeps its GUID; registered here when its meta file is missing (older library)
			if (FileIO.FileExists(absolutePath))
			{
				GDC_MMResourceFile.Register(absolutePath, relativePath, loaded, GDC_MMResourceFile.PREFAB_CLASS);
				kept++;
				continue;
			}

			FileIO.MakeDirectory(FilePath.StripFileName(absolutePath));
			FileHandle file = FileIO.OpenFile(absolutePath, FileMode.WRITE);
			if (!file)
			{
				failures.Insert(relativePath + " (write failed)");
				continue;
			}

			file.WriteLine(string.Format("%1 : \"%2\" {", item.m_sClassName, item.m_Prefab));
			file.WriteLine("}");
			file.Close();

			if (!GDC_MMResourceFile.Register(absolutePath, relativePath, loaded, GDC_MMResourceFile.PREFAB_CLASS))
			{
				failures.Insert(relativePath + " (registration failed)");
				continue;
			}

			added++;
		}

		int removed;
		if (loaded)
			removed = RemoveObsolete(addonPath, generated);

		if (!GDC_MMLibraryIndex.Save(addonPath, indexEntries))
			failures.Insert(GDC_MMLibraryIndex.INDEX + " (write failed)");
		else if (!GDC_MMResourceFile.Register(addonPath + "/" + GDC_MMLibraryIndex.INDEX, GDC_MMLibraryIndex.INDEX, loaded, GDC_MMResourceFile.CONFIG_CLASS))
			failures.Insert(GDC_MMLibraryIndex.INDEX + " (registration failed)");

		string summary = string.Format("%1 new, %2 kept, %3 removed, %4 failed.", added, kept, removed, failures.Count());
		Print(LOG_PREFIX + summary);
		foreach (string failure : failures)
		{
			Print(LOG_PREFIX + "failed: " + failure, LogLevel.ERROR);
		}

		if (loaded)
			Workbench.Dialog(CAPTION, "Libraries updated from the loaded addons: " + summary);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsLibraryAddonLoaded()
	{
		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);
		foreach (string guid : guids)
		{
			if (GameProject.GetAddonID(guid) == LIBRARY_ADDON_ID)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Workbench projects folder (Documents/My Games/ArmaReforgerWorkbench/addons), where the Launcher finds local addons.
	protected static string GetDefaultAddonsFolder()
	{
		string profilePath;
		if (!Workbench.GetAbsolutePath("$profile:", profilePath, true))
			return string.Empty;

		profilePath.Replace("\\", "/");
		while (profilePath.EndsWith("/"))
		{
			profilePath = profilePath.Substring(0, profilePath.Length() - 1);
		}

		return FilePath.StripFileName(profilePath) + "addons";
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetDefaultLibraryFolder()
	{
		string addonsFolder = GetDefaultAddonsFolder();
		if (addonsFolder.IsEmpty())
			return string.Empty;

		return addonsFolder + "/" + LIBRARY_ADDON_TITLE;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsInDefaultAddonsFolder(string folder)
	{
		string addonsFolder = GetDefaultAddonsFolder();
		return !addonsFolder.IsEmpty() && FilePath.StripFileName(folder) == addonsFolder + "/";
	}

	//------------------------------------------------------------------------------------------------
	//! Project file of the library addon, with a random GUID: each mission maker has their own local addon.
	protected static bool WriteProjectFile(string path)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;

		file.WriteLine("GameProject {");
		file.WriteLine(string.Format(" ID \"%1\"", LIBRARY_ADDON_ID));
		file.WriteLine(string.Format(" GUID \"%1\"", GDC_MMResourceFile.CreateRandomGUID()));
		file.WriteLine(string.Format(" TITLE \"%1\"", LIBRARY_ADDON_TITLE));
		file.WriteLine(" Dependencies {");
		file.WriteLine(string.Format("  \"%1\"", MMTOOLS_ADDON_GUID));
		file.WriteLine(" }");
		file.WriteLine("}");
		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! MM Forces: groups, static weapons and vehicles of the faction catalogs, PrefabLibrary/MM Forces/<Category>/<Faction>/.
	protected static void CollectForces(notnull array<ref GDC_MMLibraryItem> items, notnull set<string> generated)
	{
		array<ref GDC_MMCatalogEntry> entries = {};
		// no single characters: only groups get an AI brain when placed
		GDC_MMFactionCatalogReader.Read({ EEntityCatalogType.GROUP, EEntityCatalogType.WEAPONS_TRIPOD, EEntityCatalogType.VEHICLE }, entries);
		foreach (GDC_MMCatalogEntry entry : entries)
		{
			string folder = string.Format("%1%2/%3/", FORCES_DIRECTORY, GetCategoryName(entry), entry.m_sFactionKey);
			AddItem(items, generated, folder, entry.m_Prefab, entry.m_sClassName);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! MM Markers: PrefabLibrary/MM Markers/<Icons|Areas>/<title of the addon defining the prefab>/.
	protected static void CollectMarkers(notnull array<ref GDC_MMLibraryItem> items, notnull set<string> generated)
	{
		array<ref GDC_MMMarkerEntry> entries = {};
		GDC_MMMarkerReader.Read(LIBRARY_ADDON_ID, entries);
		foreach (GDC_MMMarkerEntry entry : entries)
		{
			string addonID, addonTitle;
			bool isVanilla;
			GDC_MMLibraryIndex.FindSourceAddon(entry.m_Prefab, addonID, addonTitle, isVanilla);
			string addonFolder = SCR_FileIOHelper.SanitiseFileName(addonTitle);
			if (addonFolder.IsEmpty())
				addonFolder = "Other";

			string kind = "Icons";
			if (entry.m_bArea)
				kind = "Areas";

			string folder = string.Format("%1%2/%3/", MARKERS_DIRECTORY, kind, addonFolder);
			AddItem(items, generated, folder, entry.m_Prefab, entry.m_sClassName);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! MM Systems: waypoints, triggers and systems listed by GDC_MMSystemsReader.
	protected static void CollectSystems(notnull array<ref GDC_MMLibraryItem> items, notnull set<string> generated)
	{
		array<ref GDC_MMSystemsEntry> entries = {};
		GDC_MMSystemsReader.Read(entries);
		foreach (GDC_MMSystemsEntry entry : entries)
		{
			AddItem(items, generated, SYSTEMS_DIRECTORY + entry.m_sFolder, entry.m_Prefab, entry.m_sClassName);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddItem(notnull array<ref GDC_MMLibraryItem> items, notnull set<string> generated, string folder, ResourceName prefab, string className)
	{
		GDC_MMLibraryItem item = new GDC_MMLibraryItem();
		item.m_sPath = GetFreePath(folder, prefab, generated);
		item.m_Prefab = prefab;
		item.m_sClassName = className;
		generated.Insert(item.m_sPath);
		items.Insert(item);
	}

	//------------------------------------------------------------------------------------------------
	//! <folder><prefab file name>.et, suffixed when two prefabs share a file name in the same folder.
	protected static string GetFreePath(string folder, ResourceName prefab, set<string> generated)
	{
		string fileName = FilePath.StripExtension(FilePath.StripPath(prefab.GetPath()));

		string path = folder + fileName + ".et";
		for (int suffix = 2; generated.Contains(path); suffix++)
		{
			path = string.Format("%1%2_%3.et", folder, fileName, suffix);
		}

		return path;
	}

	//------------------------------------------------------------------------------------------------
	//! Names sorted in the panel's alphabetical order: Groups, Static Weapons, Vehicles - Air, Vehicles - Land.
	protected static string GetCategoryName(GDC_MMCatalogEntry entry)
	{
		if (entry.m_eType == EEntityCatalogType.GROUP)
			return "Groups";

		if (entry.m_eType == EEntityCatalogType.WEAPONS_TRIPOD)
			return "Static Weapons";

		if (entry.m_bAircraft)
			return "Vehicles - Air";

		return "Vehicles - Land";
	}

	//------------------------------------------------------------------------------------------------
	//! Deletes the library prefabs (and their meta files) that were not generated by this run, addons no longer loaded included:
	//! the library mirrors the loaded addons.
	protected static int RemoveObsolete(string addonPath, set<string> generated)
	{
		int removed;
		foreach (string directory : LIBRARY_DIRECTORIES)
		{
			string libraryRoot = "$" + LIBRARY_ADDON_ID + ":" + directory;
			foreach (ResourceName existing : SCR_WorkbenchHelper.SearchWorkbenchResources({ "et" }, null, libraryRoot))
			{
				string relativePath = existing.GetPath();
				if (generated.Contains(relativePath))
					continue;

				string absolutePath = addonPath + "/" + relativePath;
				FileIO.DeleteFile(absolutePath);
				FileIO.DeleteFile(absolutePath + ".meta");
				Print(LOG_PREFIX + "removed " + relativePath);
				removed++;
			}

			// faction or addon folders of the addons no longer loaded
			RemoveEmptyFolders(addonPath + "/" + directory.Substring(0, directory.Length() - 1));
		}

		return removed;
	}

	//------------------------------------------------------------------------------------------------
	//! Deletes the empty folders under directory, deepest first.
	protected static void RemoveEmptyFolders(string directory)
	{
		array<ref SCR_FileInfo> content = SCR_FileIOHelper.GetDirectoryContent(directory);
		if (!content)
			return;

		foreach (SCR_FileInfo info : content)
		{
			if (!(info.m_eAttributes & FileAttribute.DIRECTORY))
				continue;

			RemoveEmptyFolders(info.m_sFilePath);

			// DeleteFile fails on a folder that is not empty
			if (FileIO.DeleteFile(info.m_sFilePath))
				Print(LOG_PREFIX + "removed empty folder " + info.m_sFilePath);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowError(string message)
	{
		Print(LOG_PREFIX + message, LogLevel.ERROR);
		Workbench.Dialog(CAPTION, message);
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Create")]
	protected bool ButtonCreate()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}
}
#endif
