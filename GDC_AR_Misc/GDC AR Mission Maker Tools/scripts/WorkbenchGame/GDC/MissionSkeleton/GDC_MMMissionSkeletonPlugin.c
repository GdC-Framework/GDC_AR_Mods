#ifdef WORKBENCH
//! Mission maker plugin adding the mission skeleton described by a GDC_MMMissionSkeletonConfig to the open (and saved)
//! world. It creates only what is missing, so it can be run again safely:
//! - root layers, and folders of layers each holding at least one layer;
//! - entities (managers, game mode, framework...) unless the world already has one of their class; the AI world is the
//!   terrain's own when it ships one, its empty navmeshes are filled from the config and any left empty are reported;
//! - briefing sections;
//! - an empty Coalition VON frequency config next to the world, linked on the game mode.
//! Prefabs of other addons are referenced by name only: no compile dependency.
[WorkbenchPluginAttribute(
	name: "Create Mission Skeleton",
	description: "Add the layers, managers and briefing sections missing from the open world (skeleton config in Plugins > Settings)",
	wbModules: { "WorldEditor" },
	category: "GDC",
	awesomeFontCode: 0xF0E8)]
class GDC_MMMissionSkeletonPlugin : WorkbenchPlugin
{
	[Attribute("{5C3D8A2E9F71B046}Configs/GDC MM/MissionSkeleton.conf", desc: "Skeleton content. To change it, create a config inheriting GDC's in your GDC Mission Maker Local addon (Resource Browser > Create Inherited File), change what you need and select it here", params: "conf class=GDC_MMMissionSkeletonConfig")]
	protected ResourceName m_SkeletonConfig;

	protected static const string DEFAULT_CONFIG = "{5C3D8A2E9F71B046}Configs/GDC MM/MissionSkeleton.conf";
	protected static const string LOG_PREFIX = "[Mission Skeleton] ";
	protected static const string CAPTION = "Create Mission Skeleton";

	protected static const string NAVMESH_COMPONENT = "NavmeshWorldComponent";
	protected static const string NAVMESH_SOLDIERS = "Soldiers";
	protected static const string NAVMESH_VEHICLES = "BTRlike";
	protected static const string CVON_COMPONENT = "CVON_VONGameModeComponent";
	protected static const string CVON_FREQ_KEY = "m_sFreqConfig";
	protected static const string VANILLA_ADDON_ID = "ArmaReforger";

	protected ref GDC_MMMissionSkeletonConfig m_Skeleton;
	protected ref array<string> m_aCreated;
	protected ref array<string> m_aKept;
	protected ref array<string> m_aWarnings;

	//------------------------------------------------------------------------------------------------
	override void Configure()
	{
		Workbench.ScriptDialog(CAPTION + " - settings", "Skeleton config used by the plugin.", this);
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("OK", true)]
	protected bool ButtonOK()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		m_aCreated = {};
		m_aKept = {};
		m_aWarnings = {};

		if (!LoadSkeleton())
			return;

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = worldEditor.GetApi();

		string worldFile;
		if (!GetWorldFile(api, worldFile))
		{
			ShowError("open the mission world and save it first (it must exist on disk).");
			return;
		}

		// prefab class per entity, also checking that every prefab loads
		array<string> missingAddons = {};
		array<string> uniqueClasses = {};
		foreach (GDC_MMSkeletonEntity entry : m_Skeleton.m_aEntities)
		{
			string prefabClass = GetPrefabClass(entry.m_Prefab);
			if (prefabClass.IsEmpty())
				missingAddons.Insert(entry.m_sAddon);

			if (entry.m_sUniqueClass.IsEmpty())
				uniqueClasses.Insert(prefabClass);
			else
				uniqueClasses.Insert(entry.m_sUniqueClass);
		}

		string briefingClass;
		if (m_Skeleton.m_Briefing)
		{
			briefingClass = GetPrefabClass(m_Skeleton.m_Briefing.m_Prefab);
			if (briefingClass.IsEmpty())
				missingAddons.Insert(m_Skeleton.m_Briefing.m_sAddon);
		}

		if (!missingAddons.IsEmpty())
		{
			ShowError("prefabs of the following addons cannot be loaded: " + JoinUnique(missingAddons) + ".\n\nLoad these addons, then run the plugin again.");
			return;
		}

		int subScene = api.GetCurrentSubScene();
		map<string, IEntitySource> existing = FindExistingEntities(api, subScene, uniqueClasses, briefingClass);
		ResourceName parentWorld = GetParentWorld(worldFile);
		string mapAddonGUID = FindAddonOfFile(parentWorld);

		// file first: entity sources must not be kept across an entity action, so the frequency config is linked
		// in the same action that creates the game mode
		ResourceName frequencyConfig = PrepareFrequencyConfig(existing, uniqueClasses, worldFile);

		api.BeginEntityAction(CAPTION);
		map<string, string> layers = CreateLayers(api, subScene, worldFile);
		vector origin = GetOrigin(worldEditor, api);

		foreach (int i, GDC_MMSkeletonEntity entry : m_Skeleton.m_aEntities)
		{
			IEntitySource entity = CreateSkeletonEntity(api, entry, uniqueClasses[i], existing, GetLayerId(api, subScene, layers, entry.m_sLayer), origin + Vector(4 * i, 0, 0), mapAddonGUID, parentWorld);
			if (entity && entry.m_eRole == GDC_EMMSkeletonEntityRole.GAME_MODE)
				LinkFrequencyConfig(api, entity, frequencyConfig);
		}

		if (m_Skeleton.m_Briefing)
			CreateBriefing(api, existing, briefingClass, GetLayerId(api, subScene, layers, m_Skeleton.m_Briefing.m_sLayer), origin + Vector(-8, 0, 0));

		api.EndEntityAction(CAPTION);

		ShowSummary();
	}

	//------------------------------------------------------------------------------------------------
	protected bool LoadSkeleton()
	{
		ResourceName skeletonConfig = m_SkeletonConfig;
		if (skeletonConfig.IsEmpty())
			skeletonConfig = DEFAULT_CONFIG;

		Resource resource = Resource.Load(skeletonConfig);
		if (!resource || !resource.IsValid())
		{
			ShowError("the skeleton config cannot be loaded: " + skeletonConfig.GetPath() + ".\n\nSelect one in Plugins > Settings > " + CAPTION + ".");
			return false;
		}

		m_Skeleton = GDC_MMMissionSkeletonConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		if (!m_Skeleton)
		{
			ShowError(skeletonConfig.GetPath() + " is not a mission skeleton config (GDC_MMMissionSkeletonConfig).");
			return false;
		}

		if (!m_Skeleton.m_aLayers)
			m_Skeleton.m_aLayers = {};

		if (!m_Skeleton.m_aFolders)
			m_Skeleton.m_aFolders = {};

		if (!m_Skeleton.m_aEntities)
			m_Skeleton.m_aEntities = {};

		if (!m_Skeleton.m_aTerrainNavmeshes)
			m_Skeleton.m_aTerrainNavmeshes = {};

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \param[out] worldFile absolute path of the open world's .ent file, with forward slashes
	//! \return false when no world is open or it was never saved
	protected static bool GetWorldFile(WorldEditorAPI api, out string worldFile)
	{
		string worldPath;
		api.GetWorldPath(worldPath);
		if (worldPath.IsEmpty())
			return false;

		// GetWorldPath gives "$<addon ID>:worlds/...": the absolute path is needed to locate the addon folder
		if (!Workbench.GetAbsolutePath(worldPath, worldFile, true))
			worldFile = worldPath;

		worldFile.Replace("\\", "/");
		return FileIO.FileExists(worldFile);
	}

	//------------------------------------------------------------------------------------------------
	//! \return class of the prefab's entity, empty when the prefab cannot be loaded
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

	//------------------------------------------------------------------------------------------------
	//! One pass over every entity of every sub-scene (parent terrain included).
	//! \return first entity found per looked-up base class; the briefing class and the classes of entries marked
	//! m_bMissionSubSceneOnly are only looked for in subScene
	protected map<string, IEntitySource> FindExistingEntities(WorldEditorAPI api, int subScene, notnull array<string> uniqueClasses, string briefingClass)
	{
		array<string> lookedUp = {};
		set<string> subSceneOnly = new set<string>();
		foreach (int i, string uniqueClass : uniqueClasses)
		{
			if (!lookedUp.Contains(uniqueClass))
				lookedUp.Insert(uniqueClass);

			if (m_Skeleton.m_aEntities[i].m_bMissionSubSceneOnly)
				subSceneOnly.Insert(uniqueClass);
		}

		if (!briefingClass.IsEmpty())
		{
			if (!lookedUp.Contains(briefingClass))
				lookedUp.Insert(briefingClass);

			subSceneOnly.Insert(briefingClass);
		}

		array<typename> lookedUpTypes = {};
		foreach (string className : lookedUp)
		{
			lookedUpTypes.Insert(className.ToType());
		}

		map<string, IEntitySource> found = new map<string, IEntitySource>();
		map<string, ref array<string>> classBases = new map<string, ref array<string>>(); // entity class -> looked-up bases it inherits
		for (int s = 0, subScenes = api.GetNumSubScenes(); s < subScenes; s++)
		{
			for (int i = 0, count = api.GetEntityCount(s); i < count; i++)
			{
				IEntitySource source = api.GetEntity(s, i);
				if (!source)
					continue;

				string sourceClass = source.GetClassName();
				array<string> bases = classBases.Get(sourceClass);
				if (!bases)
				{
					bases = {};
					typename type = sourceClass.ToType();
					if (type)
					{
						foreach (int b, typename lookedUpType : lookedUpTypes)
						{
							if (lookedUpType && type.IsInherited(lookedUpType))
								bases.Insert(lookedUp[b]);
						}
					}

					classBases.Insert(sourceClass, bases);
				}

				foreach (string base : bases)
				{
					if (found.Contains(base) || (s != subScene && subSceneOnly.Contains(base)))
						continue;

					found.Insert(base, source);
				}
			}
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	//! Creates the missing root layers and folders; existing ones are matched case-insensitively.
	//! \return layer name (lowercase) -> layer path, for the root layers of the config
	protected map<string, string> CreateLayers(WorldEditorAPI api, int subScene, string worldFile)
	{
		array<string> items = {};
		api.GetSubsceneFolders(subScene, items); // root items and "folder/layer"
		Print(LOG_PREFIX + "existing layers and folders: " + JoinUnique(items), LogLevel.NORMAL);

		map<string, string> layers = new map<string, string>();
		foreach (string layer : m_Skeleton.m_aLayers)
		{
			string path = FindRootItem(items, layer);
			if (path.IsEmpty())
			{
				path = api.CreateSubsceneLayer(subScene, layer);
				m_aCreated.Insert("layer " + layer);
			}
			else
			{
				m_aKept.Insert("layer " + path);
			}

			string key = layer;
			key.ToLower();
			layers.Insert(key, path);
		}

		// git does not keep empty folders: each folder gets a first layer, unless it already holds one
		string layersDirectory = FilePath.StripExtension(worldFile) + "_Layers/";
		foreach (GDC_MMSkeletonFolder folder : m_Skeleton.m_aFolders)
		{
			string path = FindRootItem(items, folder.m_sName);
			if (path.IsEmpty())
				path = api.CreateSubsceneFolder(subScene, folder.m_sName);
			else if (folder.m_sFirstLayer.IsEmpty() || FolderHasLayer(items, path, layersDirectory))
			{
				m_aKept.Insert("folder " + path);
				continue;
			}

			if (folder.m_sFirstLayer.IsEmpty())
			{
				m_aCreated.Insert("folder " + folder.m_sName);
				continue;
			}

			api.CreateSubsceneLayer(subScene, folder.m_sFirstLayer, path);
			m_aCreated.Insert("layer " + folder.m_sName + "/" + folder.m_sFirstLayer);
		}

		return layers;
	}

	//------------------------------------------------------------------------------------------------
	//! \return id of the root layer called name; when the config names no known layer, the first one, with a warning
	protected int GetLayerId(WorldEditorAPI api, int subScene, map<string, string> layers, string name)
	{
		string key = name;
		key.ToLower();
		string path = layers.Get(key);
		if (path.IsEmpty() && !m_Skeleton.m_aLayers.IsEmpty())
		{
			m_aWarnings.Insert("layer \"" + name + "\" is not among the config's layers: " + m_Skeleton.m_aLayers[0] + " used instead.");
			string first = m_Skeleton.m_aLayers[0];
			first.ToLower();
			path = layers.Get(first);
		}

		return api.GetSubsceneLayerId(subScene, path);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true when the folder holds a layer, in the editor (item below folderPath) or on disk (.layer file below it)
	protected static bool FolderHasLayer(notnull array<string> items, string folderPath, string layersDirectory)
	{
		string prefix = folderPath + "/";
		prefix.Replace("\\", "/");
		prefix.ToLower();
		foreach (string item : items)
		{
			string candidate = item;
			candidate.Replace("\\", "/");
			candidate.ToLower();
			if (candidate.StartsWith(prefix))
				return true;
		}

		array<string> files = {};
		FileIO.FindFiles(files.Insert, layersDirectory + folderPath + "/", ".layer");
		return !files.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! \return path of the root layer or folder called name (any case, with or without .layer), empty if none
	protected static string FindRootItem(notnull array<string> items, string name)
	{
		string wanted = name;
		wanted.ToLower();
		foreach (string item : items)
		{
			string candidate = item;
			candidate.Replace("\\", "/");
			candidate.ToLower();
			if (candidate.EndsWith(".layer"))
				candidate = candidate.Substring(0, candidate.Length() - 6);

			if (candidate == wanted)
				return item;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Centre of the terrain, on the ground: managers have no position of their own.
	protected static vector GetOrigin(WorldEditor worldEditor, WorldEditorAPI api)
	{
		vector min, max;
		if (!worldEditor.GetTerrainBounds(min, max))
			return vector.Zero;

		vector origin = (min + max) * 0.5;
		origin[1] = api.GetTerrainSurfaceY(origin[0], origin[2]);
		return origin;
	}

	//------------------------------------------------------------------------------------------------
	//! Places one entity of the config unless the world already has one of its unique class.
	//! \return the created entity, or for the GAME_MODE role the existing game mode when it fits; null otherwise
	protected IEntitySource CreateSkeletonEntity(WorldEditorAPI api, GDC_MMSkeletonEntity entry, string uniqueClass, map<string, IEntitySource> existing, int layerId, vector position, string mapAddonGUID, ResourceName parentWorld)
	{
		string label = entry.m_sName;
		if (label.IsEmpty())
			label = FilePath.StripExtension(FilePath.StripPath(entry.m_Prefab.GetPath()));

		IEntitySource entity = existing.Get(uniqueClass);
		if (entity)
		{
			m_aKept.Insert(label + ": " + DescribeEntity(api, entity) + " (settings unchanged)");
			if (entry.m_eRole == GDC_EMMSkeletonEntityRole.AI_WORLD)
				CheckNavmeshFiles(entity);

			if (entry.m_eRole != GDC_EMMSkeletonEntityRole.GAME_MODE)
				return null;

			typename expected = GetPrefabClass(entry.m_Prefab).ToType();
			typename type = entity.GetClassName().ToType();
			if (!expected || !type || !type.IsInherited(expected))
			{
				m_aWarnings.Insert("the existing game mode is a " + entity.GetClassName() + ", the skeleton expects " + entry.m_Prefab.GetPath() + ".");
				return null;
			}

			return entity;
		}

		ResourceName prefab = entry.m_Prefab;
		if (entry.m_eRole == GDC_EMMSkeletonEntityRole.AI_WORLD)
		{
			ResourceName terrainPrefab = FindMapAIWorldPrefab(mapAddonGUID, parentWorld, entry.m_Prefab);
			if (!terrainPrefab.IsEmpty())
				prefab = terrainPrefab;
		}

		string name = entry.m_sName;
		if (!name.IsEmpty() && api.FindEntityByName(name))
			name = string.Empty; // let the editor name it

		entity = api.CreateEntity(prefab, name, layerId, null, position, vector.Zero);
		if (!entity)
		{
			m_aWarnings.Insert(label + " could not be created from " + prefab.GetPath() + ".");
			return null;
		}

		string values;
		if (entry.m_aValues)
		{
			foreach (GDC_MMSkeletonValue value : entry.m_aValues)
			{
				if (!SetValue(api, entity, value.m_sKey, value.m_sValue))
					m_aWarnings.Insert(label + ": " + value.m_sKey + " could not be set to " + value.m_sValue + ".");
				else if (values.IsEmpty())
					values = value.m_sKey + " " + value.m_sValue;
				else
					values += ", " + value.m_sKey + " " + value.m_sValue;
			}
		}

		string created = label + ": " + prefab.GetPath();
		if (!values.IsEmpty())
			created += " (" + values + ")";

		m_aCreated.Insert(created);

		if (entry.m_eRole == GDC_EMMSkeletonEntityRole.AI_WORLD)
		{
			SetTerrainNavmeshFiles(api, entity, parentWorld);
			CheckNavmeshFiles(entity);
		}

		return entity;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetValue(WorldEditorAPI api, IEntitySource entity, string key, string value)
	{
		return api.SetVariableValue(entity, null, key, value);
	}

	//------------------------------------------------------------------------------------------------
	//! Fills the empty Soldiers and BTRlike navmeshes of a new AI world with the config's files for this terrain.
	protected void SetTerrainNavmeshFiles(WorldEditorAPI api, IEntitySource aiWorld, ResourceName parentWorld)
	{
		if (parentWorld.IsEmpty())
			return;

		string terrain = FilePath.StripExtension(FilePath.StripPath(parentWorld.GetPath()));
		terrain.ToLower();
		GDC_MMSkeletonTerrainNavmeshes navmeshes;
		foreach (GDC_MMSkeletonTerrainNavmeshes candidate : m_Skeleton.m_aTerrainNavmeshes)
		{
			string candidateTerrain = candidate.m_sTerrain;
			candidateTerrain.ToLower();
			if (candidateTerrain == terrain)
			{
				navmeshes = candidate;
				break;
			}
		}

		if (!navmeshes)
			return;

		for (int i = 0, count = aiWorld.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = aiWorld.GetComponent(i);
			if (component.GetClassName() != NAVMESH_COMPONENT)
				continue;

			BaseContainer settings = component.GetObject("NavmeshSettings");
			if (!settings)
				continue;

			string project;
			settings.Get("NavmeshProject", project);
			ResourceName navmesh;
			if (project == NAVMESH_SOLDIERS)
				navmesh = navmeshes.m_SoldiersNavmesh;
			else if (project == NAVMESH_VEHICLES)
				navmesh = navmeshes.m_VehiclesNavmesh;

			if (navmesh.IsEmpty())
				continue;

			ResourceName current;
			BaseContainer files = settings.GetObject("NavmeshFilesConfig");
			if (files)
				files.Get("NavmeshFile", current);

			if (!current.IsEmpty())
				continue;

			array<ref ContainerIdPathEntry> settingsPath = { new ContainerIdPathEntry("components", i), new ContainerIdPathEntry("NavmeshSettings") };
			if (!files)
				api.CreateObjectVariableMember(aiWorld, settingsPath, "NavmeshFilesConfig", "BaseNavmeshFilesConfig");

			array<ref ContainerIdPathEntry> filesPath = { new ContainerIdPathEntry("components", i), new ContainerIdPathEntry("NavmeshSettings"), new ContainerIdPathEntry("NavmeshFilesConfig") };
			if (api.SetVariableValue(aiWorld, filesPath, "NavmeshFile", navmesh))
				m_aCreated.Insert("navmesh " + project + ": " + navmesh.GetPath());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Warns about each navmesh of the AI world (NavmeshProject Soldiers, BTRlike, LowRes) without a navmesh file.
	protected void CheckNavmeshFiles(IEntitySource aiWorld)
	{
		array<string> missing = {};
		for (int i = 0, count = aiWorld.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = aiWorld.GetComponent(i);
			if (component.GetClassName() != NAVMESH_COMPONENT)
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
				missing.Insert(project);
		}

		if (!missing.IsEmpty())
			m_aWarnings.Insert("AI world navmeshes without a file: " + JoinUnique(missing) + ". Set their NavmeshFile (AI world > NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig), otherwise AI cannot move there.");
	}

	//------------------------------------------------------------------------------------------------
	//! The terrain's AI world prefab, which holds its navmesh files: a prefab of the map addon (vanilla: Prefabs/AI only)
	//! whose file name contains "AIWorld" and whose class inherits the default prefab's class, other than the default one.
	//! Several candidates (vanilla: Arland, Cain, Eden): the one whose file name contains the terrain world's name.
	//! \return empty if none or ambiguous
	protected static ResourceName FindMapAIWorldPrefab(string mapAddonGUID, ResourceName parentWorld, ResourceName defaultPrefab)
	{
		if (mapAddonGUID.IsEmpty() || parentWorld.IsEmpty())
			return ResourceName.Empty;

		typename aiWorldType = GetPrefabClass(defaultPrefab).ToType();
		if (!aiWorldType)
			return ResourceName.Empty;

		string root = "$" + GameProject.GetAddonID(mapAddonGUID) + ":";
		if (GameProject.IsVanillaAddon(mapAddonGUID))
			root += "Prefabs/AI";

		array<ResourceName> candidates = {};
		foreach (ResourceName prefab : SCR_WorkbenchHelper.SearchWorkbenchResources({ "et" }, null, root))
		{
			if (prefab.GetPath() == defaultPrefab.GetPath())
				continue;

			string fileName = FilePath.StripPath(prefab.GetPath());
			fileName.ToLower();
			if (!fileName.Contains("aiworld"))
				continue;

			typename type = GetPrefabClass(prefab).ToType();
			if (type && type.IsInherited(aiWorldType))
				candidates.Insert(prefab);
		}

		if (candidates.Count() == 1)
			return candidates[0];

		string terrainName = FilePath.StripExtension(FilePath.StripPath(parentWorld.GetPath()));
		terrainName.ToLower();
		foreach (ResourceName candidate : candidates)
		{
			string fileName = FilePath.StripPath(candidate.GetPath());
			fileName.ToLower();
			if (fileName.Contains(terrainName))
				return candidate;
		}

		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateBriefing(WorldEditorAPI api, map<string, IEntitySource> existing, string briefingClass, int layerId, vector position)
	{
		GDC_MMSkeletonBriefing briefing = m_Skeleton.m_Briefing;
		if (!briefing.m_aSections || briefing.m_aSections.IsEmpty())
			return;

		if (existing.Contains(briefingClass))
		{
			m_aKept.Insert("briefing (the mission already has " + briefingClass + " entities)");
			return;
		}

		array<string> titles = {};
		foreach (int i, GDC_MMSkeletonBriefingSection section : briefing.m_aSections)
		{
			string name = section.m_sName;
			if (!name.IsEmpty() && api.FindEntityByName(name))
				name = string.Empty;

			IEntitySource entity = api.CreateEntity(briefing.m_Prefab, name, layerId, null, position + Vector(0, 0, 2 * i), vector.Zero);
			if (!entity)
				continue;

			SetValue(api, entity, briefing.m_sTitleKey, section.m_sTitle);
			SetValue(api, entity, briefing.m_sTextKey, section.m_sText);
			if (!briefing.m_sOrderKey.IsEmpty())
				SetValue(api, entity, briefing.m_sOrderKey, i.ToString());

			if (briefing.m_aValues)
			{
				foreach (GDC_MMSkeletonValue value : briefing.m_aValues)
				{
					SetValue(api, entity, value.m_sKey, value.m_sValue);
				}
			}

			titles.Insert(section.m_sTitle);
		}

		if (titles.Count() == briefing.m_aSections.Count())
			m_aCreated.Insert("briefing: " + JoinUnique(titles));
		else
			m_aWarnings.Insert(string.Format("only %1 of %2 briefing sections could be created.", titles.Count(), briefing.m_aSections.Count()));
	}

	//------------------------------------------------------------------------------------------------
	//! Writes <world folder>/Configs/<world>_Frequencies.conf (empty CVON config) if needed and registers it.
	//! Called before any entity action: registering a resource must not happen while entity sources are kept.
	//! \return the config to link on the game mode, empty when there is nothing to link
	protected ResourceName PrepareFrequencyConfig(map<string, IEntitySource> existing, notnull array<string> uniqueClasses, string worldFile)
	{
		if (!m_Skeleton.m_bFrequencyConfig)
			return ResourceName.Empty;

		int gameModeIndex = -1;
		foreach (int i, GDC_MMSkeletonEntity entry : m_Skeleton.m_aEntities)
		{
			if (entry.m_eRole == GDC_EMMSkeletonEntityRole.GAME_MODE)
			{
				gameModeIndex = i;
				break;
			}
		}

		if (gameModeIndex < 0)
			return ResourceName.Empty;

		if (!CVON_COMPONENT.ToType())
		{
			m_aWarnings.Insert("Coalition VON is not loaded: no frequency config created.");
			return ResourceName.Empty;
		}

		IEntitySource gameMode = existing.Get(uniqueClasses[gameModeIndex]);
		if (gameMode)
		{
			typename expected = GetPrefabClass(m_Skeleton.m_aEntities[gameModeIndex].m_Prefab).ToType();
			typename type = gameMode.GetClassName().ToType();
			if (!expected || !type || !type.IsInherited(expected))
				return ResourceName.Empty; // reported by CreateSkeletonEntity

			int componentIndex = FindComponentIndex(gameMode, CVON_COMPONENT);
			if (componentIndex < 0)
			{
				m_aWarnings.Insert("the game mode has no " + CVON_COMPONENT + ": no frequency config linked.");
				return ResourceName.Empty;
			}

			ResourceName current;
			gameMode.GetComponent(componentIndex).Get(CVON_FREQ_KEY, current);
			if (!current.IsEmpty())
			{
				m_aKept.Insert("frequency config " + current.GetPath());
				return ResourceName.Empty;
			}
		}

		string worldName = FilePath.StripExtension(FilePath.StripPath(worldFile));
		string folder = FilePath.StripFileName(worldFile);
		if (!folder.EndsWith("/"))
			folder += "/";

		folder += "Configs";
		string configFile = folder + "/" + worldName + "_Frequencies.conf";
		if (FileIO.FileExists(configFile))
		{
			m_aKept.Insert("file " + configFile);
		}
		else
		{
			FileIO.MakeDirectory(folder);
			FileHandle file = FileIO.OpenFile(configFile, FileMode.WRITE);
			if (!file)
			{
				m_aWarnings.Insert("cannot write " + configFile + ".");
				return ResourceName.Empty;
			}

			file.WriteLine("CVON_FreqConfig {");
			file.WriteLine("}");
			file.Close();
			m_aCreated.Insert("frequency config " + configFile);
		}

		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!FileIO.FileExists(configFile + ".meta"))
			resourceManager.RegisterResourceFile(configFile, false);

		MetaFile meta = resourceManager.GetMetaFile(configFile);
		if (!meta)
		{
			m_aWarnings.Insert(configFile + " could not be registered: link it by hand on the game mode (" + CVON_COMPONENT + " > Freq Config).");
			return ResourceName.Empty;
		}

		return meta.GetResourceID();
	}

	//------------------------------------------------------------------------------------------------
	//! Links config on the game mode's CVON component; call it inside the entity action that created or found gameMode.
	protected void LinkFrequencyConfig(WorldEditorAPI api, IEntitySource gameMode, ResourceName config)
	{
		if (!gameMode || config.IsEmpty())
			return;

		int componentIndex = FindComponentIndex(gameMode, CVON_COMPONENT);
		if (componentIndex < 0)
		{
			m_aWarnings.Insert("the game mode has no " + CVON_COMPONENT + ": link " + config.GetPath() + " by hand.");
			return;
		}

		if (api.SetVariableValue(gameMode, { new ContainerIdPathEntry("components", componentIndex) }, CVON_FREQ_KEY, config))
			m_aCreated.Insert("frequency config linked on the game mode");
		else
			m_aWarnings.Insert(config.GetPath() + " not linked: set it by hand on the game mode (" + CVON_COMPONENT + " > Freq Config).");
	}

	//------------------------------------------------------------------------------------------------
	//! \return index of the first component of that class, -1 if none
	protected static int FindComponentIndex(IEntitySource entity, string className)
	{
		for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
		{
			if (entity.GetComponent(i).GetClassName() == className)
				return i;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the parent world of a sub-scene .ent ("SubScene { Parent ... }"), empty if the world is not a sub-scene
	protected static ResourceName GetParentWorld(string worldFile)
	{
		FileHandle file = FileIO.OpenFile(worldFile, FileMode.READ);
		if (!file)
			return ResourceName.Empty;

		string line;
		ResourceName parent;
		while (file.ReadLine(line) >= 0)
		{
			line.TrimInPlace();
			if (!line.StartsWith("Parent "))
				continue;

			int start = line.IndexOf("\"");
			int end = line.LastIndexOf("\"");
			if (start >= 0 && end > start)
				parent = line.Substring(start + 1, end - start - 1);

			break;
		}

		file.Close();
		return parent;
	}

	//------------------------------------------------------------------------------------------------
	//! \return GUID of the first loaded non-vanilla addon (load order) holding that file, else the vanilla addon's, empty if none
	protected static string FindAddonOfFile(ResourceName resource)
	{
		if (resource.IsEmpty())
			return string.Empty;

		string path = resource.GetPath();
		string extension;
		FilePath.StripExtension(path, extension);

		array<string> guids = {};
		GameProject.GetLoadedAddons(guids);
		string vanillaGUID; // "vanilla" is an Enforce keyword
		foreach (string guid : guids)
		{
			if (GameProject.IsVanillaAddon(guid))
			{
				// the base game data (worlds, prefabs), not core
				if (vanillaGUID.IsEmpty() || GameProject.GetAddonID(guid) == VANILLA_ADDON_ID)
					vanillaGUID = guid;

				continue;
			}

			foreach (ResourceName file : SCR_WorkbenchHelper.SearchWorkbenchResources({ extension }, null, "$" + GameProject.GetAddonID(guid) + ":"))
			{
				if (file.GetPath() == path)
					return guid;
			}
		}

		return vanillaGUID;
	}

	//------------------------------------------------------------------------------------------------
	protected static string DescribeEntity(WorldEditorAPI api, IEntitySource entity)
	{
		string name = entity.GetName();
		if (name.IsEmpty())
			name = entity.GetClassName();

		if (entity.GetSubScene() != api.GetCurrentSubScene())
			return name + " (from the terrain)";

		return name;
	}

	//------------------------------------------------------------------------------------------------
	protected static string JoinUnique(notnull array<string> values)
	{
		array<string> unique = {};
		foreach (string value : values)
		{
			if (!unique.Contains(value))
				unique.Insert(value);
		}

		string joined;
		foreach (int i, string value : unique)
		{
			if (i > 0)
				joined += ", ";

			joined += value;
		}

		return joined;
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowSummary()
	{
		string message;
		if (m_aCreated.IsEmpty())
			message = "Nothing to add: the world already has the whole skeleton.";
		else
			message = "Added:\n- " + JoinLines(m_aCreated);

		if (!m_aKept.IsEmpty())
			message += "\n\nAlready there, left unchanged:\n- " + JoinLines(m_aKept);

		if (!m_aWarnings.IsEmpty())
			message += "\n\nTo check:\n- " + JoinLines(m_aWarnings);

		if (!m_aCreated.IsEmpty())
			message += "\n\nSave the world (Ctrl+S) to keep the changes.";

		Print(LOG_PREFIX + message, LogLevel.NORMAL);
		Workbench.Dialog(CAPTION, message);
	}

	//------------------------------------------------------------------------------------------------
	protected static string JoinLines(notnull array<string> lines)
	{
		string joined;
		foreach (int i, string line : lines)
		{
			if (i > 0)
				joined += "\n- ";

			joined += line;
		}

		return joined;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ShowError(string message)
	{
		Print(LOG_PREFIX + "error: " + message, LogLevel.WARNING);
		Workbench.Dialog(CAPTION, "Error: " + message);
	}
}
#endif
