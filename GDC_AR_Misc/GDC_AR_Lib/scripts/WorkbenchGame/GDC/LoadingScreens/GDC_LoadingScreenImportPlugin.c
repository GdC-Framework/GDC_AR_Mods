#ifdef WORKBENCH
//! Resource Manager plugin importing the loading screens prepared by _Media/Tools/prepare_loading_screens.py.
//! Every PNG of _Media/LoadingScreens/Exports is copied into GDC AR Core (UI/MissionLoadingScreens),
//! registered (new image) or kept with its GUID (existing image) and rebuilt as .edds. The PNG copy stays next to the .edds as its source.
//! GDC AR Core must be opened from the Git repository, next to the _Media folder.
[WorkbenchPluginAttribute("GDC Import Loading Screens", "Import or re-import the loading screens of _Media/LoadingScreens/Exports into GDC AR Core (UI/MissionLoadingScreens)", "", "", { "ResourceManager" }, "", 0xf03e)]
class GDC_LoadingScreenImportPlugin : WorkbenchPlugin
{
	protected static const string LOG_PREFIX = "[GDC LoadingScreens] ";
	protected static const string TARGET_FOLDER = "$GDC AR Core:UI/MissionLoadingScreens";
	//! Relative to TARGET_FOLDER: GDC_AR_ModPack/GDC_AR_Core/UI/MissionLoadingScreens -> repository root
	protected static const string EXPORT_FOLDER = "/../../../../_Media/LoadingScreens/Exports";

	protected ref array<string> m_aFiles = {};

	//------------------------------------------------------------------------------------------------
	protected void OnFileFound(string fileName, FileAttribute attributes = 0, string filesystem = string.Empty)
	{
		m_aFiles.Insert(FilePath.StripPath(fileName));
	}

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		string targetFolder;
		if (!Workbench.GetAbsolutePath(TARGET_FOLDER, targetFolder, true))
		{
			ShowError("folder " + TARGET_FOLDER + " not found. Open GDC AR Core (or a project depending on it) in the Workbench.");
			return;
		}

		string exportFolder = targetFolder + EXPORT_FOLDER;
		m_aFiles.Clear();
		FileIO.FindFiles(OnFileFound, exportFolder, ".png");
		if (m_aFiles.IsEmpty())
		{
			ShowError("no PNG found in " + exportFolder + ". GDC AR Core must be opened from the Git repository; run _Media/Tools/Preparer_LoadingScreens.cmd first.");
			return;
		}

		m_aFiles.Sort();
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		WBProgressDialog progress = new WBProgressDialog("GDC Import Loading Screens", resourceManager);
		int added, updated;
		array<string> failures = {};

		foreach (int index, string fileName : m_aFiles)
		{
			progress.SetProgress(index / (float)m_aFiles.Count());

			string pngPath = targetFolder + "/" + fileName;
			if (!FileIO.CopyFile(exportFolder + "/" + fileName, pngPath))
			{
				failures.Insert(fileName + " (copy failed)");
				continue;
			}

			// Registering reuses an existing meta-file, so the GUID referenced by missions is kept
			if (FileIO.FileExists(FilePath.StripExtension(pngPath) + ".edds.meta"))
				updated++;
			else
				added++;

			resourceManager.RegisterResourceFile(pngPath, false);
			MetaFile meta = resourceManager.GetMetaFile(pngPath);
			if (!meta)
			{
				failures.Insert(fileName + " (registration failed)");
				continue;
			}

			// Interface image: sRGB colors (the default texture config converts to linear) and no mipmaps (lighter .edds)
			BaseContainerList configurations = meta.GetObjectArray("Configurations");
			for (int i = 0, count = configurations.Count(); i < count; i++)
			{
				BaseContainer configuration = configurations.Get(i);
				configuration.Set("ColorSpace", MetaEddsColorSpaceConversion.ToSrgb);
				configuration.Set("GenerateMips", false);
			}

			meta.Save();
			meta.Release();

			// The PNG copy must stay: it is the source of the .edds, deleting it makes the Workbench delete the .edds
			resourceManager.RebuildResourceFile(pngPath, "PC", false);
		}

		string summary = string.Format("%1 new, %2 re-imported, %3 failed.", added, updated, failures.Count());
		Print(LOG_PREFIX + summary);
		foreach (string failure : failures)
		{
			Print(LOG_PREFIX + "failed: " + failure, LogLevel.ERROR);
		}

		Workbench.Dialog("GDC Import Loading Screens", summary);
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowError(string message)
	{
		Print(LOG_PREFIX + message, LogLevel.ERROR);
		Workbench.Dialog("GDC Import Loading Screens", message);
	}
}
#endif
