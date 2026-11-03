#ifdef WORKBENCH
//! Registers the files written into the libraries addon. When that addon is not loaded (first run), the Workbench cannot
//! register them and does not do it either when it opens the addon later: the meta file is then written here,
//! in the format the Workbench writes, so that the libraries are usable as soon as the addon is loaded.
class GDC_MMResourceFile
{
	static const string PREFAB_CLASS = "EntityTemplateResourceClass";
	static const string CONFIG_CLASS = "CONFResourceClass";

	protected static const string HEX_DIGITS = "0123456789ABCDEF";

	//! Platform configurations inheriting from PC, as in the meta files the Workbench writes
	protected static const ref array<string> PLATFORMS = { "XBOX_ONE", "XBOX_SERIES", "PS4", "PS5", "HEADLESS" };

	protected static ref RandomGenerator s_Random;

	//------------------------------------------------------------------------------------------------
	//! \param[in] absolutePath file written into the libraries addon
	//! \param[in] relativePath its path inside the addon, for the meta file
	//! \param[in] loaded true when the libraries addon is loaded
	//! \param[in] resourceClass PREFAB_CLASS or CONFIG_CLASS
	static bool Register(string absolutePath, string relativePath, bool loaded, string resourceClass)
	{
		if (FileIO.FileExists(absolutePath + ".meta"))
			return true;

		if (loaded)
		{
			ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
			return resourceManager.RegisterResourceFile(absolutePath, false);
		}

		FileHandle file = FileIO.OpenFile(absolutePath + ".meta", FileMode.WRITE);
		if (!file)
			return false;

		file.WriteLine("MetaFileClass {");
		file.WriteLine(string.Format(" Name \"{%1}%2\"", CreateRandomGUID(), relativePath));
		file.WriteLine(" Configurations {");
		file.WriteLine(string.Format("  %1 PC {", resourceClass));
		file.WriteLine("  }");
		foreach (string platform : PLATFORMS)
		{
			file.WriteLine(string.Format("  %1 %2 : PC {", resourceClass, platform));
			file.WriteLine("  }");
		}

		file.WriteLine(" }");
		file.WriteLine("}");
		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! 16 random hexadecimal digits, the format of resource and addon GUIDs.
	static string CreateRandomGUID()
	{
		if (!s_Random)
		{
			s_Random = new RandomGenerator();
			s_Random.SetSeed(System.GetUnixTime() + System.GetTickCount());
		}

		string guid;
		for (int i; i < 16; i++)
		{
			guid += HEX_DIGITS.Get(s_Random.RandInt(0, 16));
		}

		return guid;
	}
}
#endif
