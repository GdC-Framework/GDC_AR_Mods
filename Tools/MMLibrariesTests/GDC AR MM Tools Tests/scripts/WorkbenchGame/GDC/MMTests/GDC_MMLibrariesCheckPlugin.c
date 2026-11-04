#ifdef WORKBENCH
//! Gives the tests access to the protected helpers of the drop plugin, so that they check the code used on drop.
//! No WorkbenchPluginAttribute: not a plugin of its own.
class GDC_MMDropPluginTestAccess : GDC_MMCatalogDropPlugin
{
	//------------------------------------------------------------------------------------------------
	static bool TestIsCatalogPrefab(ResourceName prefab)
	{
		return IsCatalogPrefab(prefab);
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName TestGetParentPrefab(ResourceName catalogPrefab)
	{
		return GetParentPrefab(catalogPrefab);
	}
}

//! Automated test of a generated MM library, run by Tools/MMLibrariesTests/run_tests.py, never by hand:
//! -wbModule=ResourceManager -plugin=GDC_MMLibrariesCheckPlugin -run -mmTestReport="<file>"
//! For every library prefab registered by the Workbench, writes one tab-separated line to the report:
//! library path, parent prefab as seen on drop (empty when its addon is not loaded), whether the drop plugin
//! intercepts it, addon ID and title found in the index. The Python runner compares it with the files on disk.
[WorkbenchPluginAttribute(name: "MM Libraries Check (tests)", wbModules: { "ResourceManager" }, category: "GDC/Tests")]
class GDC_MMLibrariesCheckPlugin : WorkbenchPlugin
{
	protected static const string LOG_PREFIX = "[MM Tests] ";
	protected static const string LIBRARY_ROOT = "$GDC AR MM Libraries:PrefabLibrary/";

	//------------------------------------------------------------------------------------------------
	override void RunCommandline()
	{
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		string reportPath;
		resourceManager.GetCmdLine("-mmTestReport", reportPath);
		if (reportPath.IsEmpty())
		{
			Print(LOG_PREFIX + "no -mmTestReport given", LogLevel.ERROR);
			Workbench.Exit(1);
			return;
		}

		FileHandle report = FileIO.OpenFile(reportPath, FileMode.WRITE);
		if (!report)
		{
			Print(LOG_PREFIX + "cannot write " + reportPath, LogLevel.ERROR);
			Workbench.Exit(1);
			return;
		}

		array<ResourceName> prefabs = SCR_WorkbenchHelper.SearchWorkbenchResources({ "et" }, null, LIBRARY_ROOT);
		foreach (ResourceName prefab : prefabs)
		{
			ResourceName parent = GDC_MMDropPluginTestAccess.TestGetParentPrefab(prefab);
			string intercepted = "0";
			if (GDC_MMDropPluginTestAccess.TestIsCatalogPrefab(prefab))
				intercepted = "1";

			string addonID, addonTitle;
			GDC_MMLibraryIndexEntry entry = GDC_MMLibraryIndex.Find(prefab);
			if (entry)
			{
				addonID = entry.m_sAddonID;
				addonTitle = entry.m_sAddonTitle;
			}

			report.WriteLine(string.Format("%1\t%2\t%3\t%4\t%5", prefab, parent, intercepted, addonID, addonTitle));
		}

		report.Close();
		PrintFormat(LOG_PREFIX + "%1 registered library prefabs reported", prefabs.Count());
		Workbench.Exit(0);
	}
}
#endif
