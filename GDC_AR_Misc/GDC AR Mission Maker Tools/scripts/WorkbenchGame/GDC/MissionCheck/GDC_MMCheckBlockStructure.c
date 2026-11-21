#ifdef WORKBENCH
//! Block 1, mission structure, compared with the mission skeleton (GDC_MMMissionSkeletonConfig, chosen in the plugin
//! settings; the same as Create Mission Skeleton's):
//! - 1.1 its root layers and folders exist (any case), the missing ones in a single finding;
//! - 1.2 exactly one game mode in the mission;
//! - 1.3 its entities exist (managers, AI world, framework), the game mode being 1.2's;
//! - 1.4 every navmesh of the AI world has a file;
//! - 1.5 to 1.7 its briefing sections exist (the missing ones in a single finding), have a text, and no longer hold the
//!   skeleton's hint.
class GDC_MMCheckBlockStructure : GDC_MMCheckBlock
{
	protected static const string GAME_MODE = "SCR_BaseGameMode";
	protected static const string NAVMESH_COMPONENT = "NavmeshWorldComponent";

	protected GDC_MMCheckContext m_Context;
	protected GDC_MMCheckReport m_Report;

	//------------------------------------------------------------------------------------------------
	override bool IsImplemented()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		m_Context = context;
		m_Report = report;

		CheckGameMode();

		GDC_MMMissionSkeletonConfig skeleton = LoadSkeleton(context.m_SkeletonConfig);
		if (!skeleton)
		{
			GDC_MMCheckLog.Write(GDC_MMCheckText.T(
				"  The mission skeleton cannot be loaded (" + context.m_SkeletonConfig.GetPath() + "): rules 1.1, 1.3 and 1.5 to 1.7 skipped. Select it in Plugins > Settings > Check Mission.",
				"  Le squelette de mission ne peut pas être chargé (" + context.m_SkeletonConfig.GetPath() + ") : règles 1.1, 1.3 et 1.5 à 1.7 ignorées. Choisissez-le dans Plugins > Settings > Check Mission."), LogLevel.WARNING);
			return;
		}

		CheckLayers(skeleton);
		CheckEntities(skeleton);
		if (skeleton.m_Briefing)
			CheckBriefing(skeleton.m_Briefing);
	}

	//------------------------------------------------------------------------------------------------
	protected static GDC_MMMissionSkeletonConfig LoadSkeleton(ResourceName config)
	{
		if (config.IsEmpty())
			return null;

		Resource resource = Resource.Load(config);
		if (!resource || !resource.IsValid())
			return null;

		return GDC_MMMissionSkeletonConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
	}

	//------------------------------------------------------------------------------------------------
	//! 1.2: the game modes of the mission sub-scene (the terrain's do not count)
	protected void CheckGameMode()
	{
		array<IEntitySource> gameModes = {};
		foreach (IEntitySource entity : m_Context.m_aEntities)
		{
			if (GDC_MMCheckContext.Inherits(entity.GetClassName(), GAME_MODE))
				gameModes.Insert(entity);
		}

		int count = gameModes.Count();
		if (count == 0)
			m_Report.Add("1.2", null, string.Empty, "0");

		if (count < 2)
			return;

		foreach (IEntitySource gameMode : gameModes)
		{
			m_Report.Add("1.2", gameMode, string.Empty, count.ToString());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 1.1: root layers and folders of the skeleton, matched whatever their case
	protected void CheckLayers(GDC_MMMissionSkeletonConfig skeleton)
	{
		array<string> items = {};
		m_Context.m_Api.GetSubsceneFolders(m_Context.m_iSubScene, items); // root items and "folder/layer"
		set<string> existing = new set<string>();
		foreach (string item : items)
		{
			string name = item;
			name.Replace("\\", "/");
			name.ToLower();
			if (name.EndsWith(".layer"))
				name = name.Substring(0, name.Length() - 6);

			existing.Insert(name);
		}

		array<string> expected = {};
		if (skeleton.m_aLayers)
			expected.InsertAll(skeleton.m_aLayers);

		if (skeleton.m_aFolders)
		{
			foreach (GDC_MMSkeletonFolder folder : skeleton.m_aFolders)
			{
				expected.Insert(folder.m_sName);
			}
		}

		array<string> missing = {};
		foreach (string layer : expected)
		{
			string key = layer;
			key.ToLower();
			if (!existing.Contains(key))
				missing.Insert(layer);
		}

		if (!missing.IsEmpty())
			m_Report.Add("1.1", null, string.Empty, string.Join(", ", missing, true));
	}

	//------------------------------------------------------------------------------------------------
	//! 1.3 and 1.4: each skeleton entity, by its unique class (else its prefab's), in the whole world (terrain included)
	//! unless it is looked for in the mission only
	protected void CheckEntities(GDC_MMMissionSkeletonConfig skeleton)
	{
		if (!skeleton.m_aEntities)
			return;

		array<ref GDC_MMSkeletonEntity> entries = {};
		array<string> classes = {};
		set<string> missionOnly = new set<string>();
		foreach (GDC_MMSkeletonEntity entry : skeleton.m_aEntities)
		{
			if (entry.m_eRole == GDC_EMMSkeletonEntityRole.GAME_MODE)
				continue; // rule 1.2

			string className = entry.m_sUniqueClass;
			if (className.IsEmpty())
				className = GetPrefabClass(entry.m_Prefab);

			entries.Insert(entry);
			classes.Insert(className);
			if (entry.m_bMissionSubSceneOnly && !className.IsEmpty())
				missionOnly.Insert(className);
		}

		map<string, IEntitySource> found = FindEntities(classes, missionOnly);
		foreach (int i, GDC_MMSkeletonEntity expected : entries)
		{
			IEntitySource entity = found.Get(classes[i]);
			if (!entity)
			{
				string shown = classes[i];
				if (shown.IsEmpty())
					shown = FilePath.StripExtension(FilePath.StripPath(expected.m_Prefab.GetPath()));

				m_Report.Add("1.3", null, string.Empty, shown, expected.m_sAddon);
				continue;
			}

			if (expected.m_eRole == GDC_EMMSkeletonEntityRole.AI_WORLD)
				CheckNavmeshFiles(entity);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 1.4: each navmesh of the AI world (NavmeshProject Soldiers, BTRlike, LowRes) has a navmesh file
	protected void CheckNavmeshFiles(IEntitySource aiWorld)
	{
		for (int i = 0, count = aiWorld.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = aiWorld.GetComponent(i);
			if (!GDC_MMCheckContext.Inherits(component.GetClassName(), NAVMESH_COMPONENT))
				continue;

			BaseContainer settings = component.GetObject("NavmeshSettings");
			if (!settings)
				continue;

			string project;
			settings.Get("NavmeshProject", project);

			ResourceName file;
			BaseContainer files = settings.GetObject("NavmeshFilesConfig");
			if (files)
				files.Get("NavmeshFile", file);

			if (file.IsEmpty())
				m_Report.Add("1.4", aiWorld, string.Empty, project);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 1.5 to 1.7: the briefing entities of the mission, matched with the skeleton sections by title
	protected void CheckBriefing(GDC_MMSkeletonBriefing briefing)
	{
		if (!briefing.m_aSections || briefing.m_aSections.IsEmpty())
			return;

		string briefingClass = GetPrefabClass(briefing.m_Prefab);
		if (briefingClass.IsEmpty())
			return; // its addon is not loaded: rule 1.3 does not cover it, rule 2.1 reports the mission's own references

		map<string, IEntitySource> sections = new map<string, IEntitySource>(); // title -> entity
		array<string> missing = {};
		foreach (IEntitySource entity : m_Context.m_aEntities)
		{
			if (!GDC_MMCheckContext.Inherits(entity.GetClassName(), briefingClass))
				continue;

			string title;
			entity.Get(briefing.m_sTitleKey, title);
			string text;
			entity.Get(briefing.m_sTextKey, text);
			sections.Insert(title, entity);
			if (Normalize(text).IsEmpty())
				m_Report.Add("1.6", entity, string.Empty, title);
		}

		foreach (GDC_MMSkeletonBriefingSection section : briefing.m_aSections)
		{
			IEntitySource entity = sections.Get(section.m_sTitle);
			if (!entity)
			{
				missing.Insert(GDC_MMCheckText.T("\"" + section.m_sTitle + "\"", "« " + section.m_sTitle + " »"));
				continue;
			}

			string text;
			entity.Get(briefing.m_sTextKey, text);
			string written = Normalize(text);
			if (!written.IsEmpty() && written == Normalize(section.m_sText))
				m_Report.Add("1.7", entity, string.Empty, section.m_sTitle);
		}

		if (!missing.IsEmpty())
			m_Report.Add("1.5", null, string.Empty, string.Join(", ", missing, true));
	}

	//------------------------------------------------------------------------------------------------
	//! \return the text without any white space: line breaks may be stored differently in the world and the config
	protected static string Normalize(string text)
	{
		string normalized = text;
		normalized.Replace("\r", string.Empty);
		normalized.Replace("\n", string.Empty);
		normalized.Replace("\t", string.Empty);
		normalized.Replace(" ", string.Empty);
		return normalized;
	}

	//------------------------------------------------------------------------------------------------
	//! The mission sub-scene first, where the skeleton places its entities; the other sub-scenes (terrain, large) only
	//! for the classes still missing. Each entity class is tested once.
	//! \param missionOnly classes only looked for in the mission sub-scene
	//! \return first entity found per class inheriting it (empty class names are ignored)
	protected map<string, IEntitySource> FindEntities(notnull array<string> classes, notnull set<string> missionOnly)
	{
		array<string> wanted = {};
		array<typename> wantedTypes = {};
		foreach (string className : classes)
		{
			typename type = className.ToType();
			if (type && !wanted.Contains(className))
			{
				wanted.Insert(className);
				wantedTypes.Insert(type);
			}
		}

		map<string, IEntitySource> found = new map<string, IEntitySource>();
		map<string, ref array<string>> matches = new map<string, ref array<string>>(); // entity class -> wanted classes it inherits
		WorldEditorAPI api = m_Context.m_Api;
		array<int> subScenes = { m_Context.m_iSubScene };
		for (int other = 0, subSceneCount = api.GetNumSubScenes(); other < subSceneCount; other++)
		{
			if (other != m_Context.m_iSubScene)
				subScenes.Insert(other);
		}

		foreach (int s : subScenes)
		{
			if (found.Count() == wanted.Count() || (s != m_Context.m_iSubScene && !LooksBeyondMission(wanted, found, missionOnly)))
				break;

			for (int i = 0, count = api.GetEntityCount(s); i < count && found.Count() < wanted.Count(); i++)
			{
				IEntitySource source = api.GetEntity(s, i);
				if (!source)
					continue;

				string sourceClass = source.GetClassName();
				array<string> inherited = matches.Get(sourceClass);
				if (!inherited)
				{
					inherited = {};
					typename sourceType = sourceClass.ToType();
					if (sourceType)
					{
						foreach (int w, typename wantedType : wantedTypes)
						{
							if (sourceType.IsInherited(wantedType))
								inherited.Insert(wanted[w]);
						}
					}

					matches.Insert(sourceClass, inherited);
				}

				foreach (string inheritedClass : inherited)
				{
					if (!found.Contains(inheritedClass) && (s == m_Context.m_iSubScene || !missionOnly.Contains(inheritedClass)))
						found.Insert(inheritedClass, source);
				}
			}
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether a class still missing may be found outside the mission sub-scene
	protected static bool LooksBeyondMission(notnull array<string> wanted, notnull map<string, IEntitySource> found, notnull set<string> missionOnly)
	{
		foreach (string className : wanted)
		{
			if (!found.Contains(className) && !missionOnly.Contains(className))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! \return class of the prefab's entity, empty when the prefab cannot be loaded (its addon is not loaded)
	protected static string GetPrefabClass(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return string.Empty;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return string.Empty;

		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source)
			return string.Empty;

		return source.GetClassName();
	}
}
#endif
