#ifdef WORKBENCH
//! A folder of an addon whose prefabs go into the MM Systems library, sub-folders included.
class GDC_MMSystemsRule
{
	string m_sAddonID;
	string m_sSourceFolder;

	//! Folder in MM Systems; the sub-folders of the source folder are kept below it
	string m_sTargetFolder;

	//------------------------------------------------------------------------------------------------
	void GDC_MMSystemsRule(string addonID, string sourceFolder, string targetFolder)
	{
		m_sAddonID = addonID;
		m_sSourceFolder = sourceFolder;
		m_sTargetFolder = targetFolder;
	}
}

//! One prefab of the MM Systems library.
class GDC_MMSystemsEntry
{
	ResourceName m_Prefab;

	//! Entity class of the prefab, needed to write a child prefab of it.
	string m_sClassName;

	//! Folder relative to the MM Systems library, ending with a slash.
	string m_sFolder;
}

//! Finds the prefabs of the MM Systems library: a short list of well identified systems, not every logic prefab
//! (managers, game modes, Scenario Framework... stay in the Resource Browser). Rules of addons not loaded are ignored.
class GDC_MMSystemsReader
{
	//------------------------------------------------------------------------------------------------
	protected static array<ref GDC_MMSystemsRule> GetRules()
	{
		return {
			new GDC_MMSystemsRule("ArmaReforger", "Prefabs/AI/Waypoints/", "Waypoints/Arma Reforger/"),
			// the other triggers belong to the Scenario Framework
			new GDC_MMSystemsRule("TilWMissionFramework", "Prefabs/Logic/Triggers/", "Triggers/TilW Mission Framework/"),
			new GDC_MMSystemsRule("TilWMissionFramework", "Prefabs/Utils/", "Systems/TilW Mission Framework/"),
			new GDC_MMSystemsRule("GDC AR Lib", "Prefabs/AI/", "Systems/GDC AR Lib/AI/"),
			new GDC_MMSystemsRule("GDC AR Lib", "Prefabs/Support/", "Systems/GDC AR Lib/Support/"),
			new GDC_MMSystemsRule("GDC AR Lib", "Prefabs/Teleport/", "Systems/GDC AR Lib/Teleport/"),
		};
	}

	//------------------------------------------------------------------------------------------------
	static void Read(notnull array<ref GDC_MMSystemsEntry> result)
	{
		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);

		set<string> loadedIDs = new set<string>();
		foreach (string guid : guids)
		{
			loadedIDs.Insert(GameProject.GetAddonID(guid));
		}

		foreach (GDC_MMSystemsRule rule : GetRules())
		{
			if (!loadedIDs.Contains(rule.m_sAddonID))
				continue;

			string root = "$" + rule.m_sAddonID + ":" + rule.m_sSourceFolder;
			foreach (ResourceName prefab : SCR_WorkbenchHelper.SearchWorkbenchResources({ "et" }, null, root))
			{
				Resource resource = Resource.Load(prefab);
				if (!resource.IsValid())
					continue;

				BaseContainer container = resource.GetResource().ToBaseContainer();
				if (!container)
					continue;

				string folder = FilePath.StripFileName(prefab.GetPath());
				if (!folder.EndsWith("/"))
					folder += "/";

				int length = rule.m_sSourceFolder.Length();

				GDC_MMSystemsEntry entry = new GDC_MMSystemsEntry();
				entry.m_Prefab = prefab;
				entry.m_sClassName = container.GetClassName();
				entry.m_sFolder = rule.m_sTargetFolder + folder.Substring(length, folder.Length() - length);
				result.Insert(entry);
			}
		}

		PrintFormat("[MM Libraries] %1 systems found.", result.Count());
	}
}
#endif
