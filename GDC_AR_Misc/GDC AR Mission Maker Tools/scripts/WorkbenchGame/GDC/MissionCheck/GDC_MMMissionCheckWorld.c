#ifdef WORKBENCH
//! What several blocks need to know about the open world, read once per check and cached: the mission skeleton,
//! entities by class (terrain included), names, factions, playable groups and slots, ranks and loadouts of the slots.
//! Classes of addons (Reforger Lobby, TILW, GDC AR Lib) are looked up by name: no compile dependency on them.
class GDC_MMCheckWorld
{
	static const int NO_RANK = -1; //!< below every rank: SCR_ECharacterRank.INVALID is the LAST value of its enum
	protected static const string GROUP = "SCR_AIGroup";
	protected static const string FACTION_MANAGER = "SCR_FactionManager";
	protected static const string RANK_COMPONENT = "SCR_CharacterRankComponent";
	protected static const ref array<string> RUNTIME_NAME_VARS = { "m_setEntityName", "m_setEntityNames" };
	protected static const string CREW_GROUP = "TILW_CrewGroup";

	protected GDC_MMCheckContext m_Context; //!< owner, not a ref: the context holds this object

	protected bool m_bSkeletonRead;
	protected ref GDC_MMMissionSkeletonConfig m_Skeleton;
	protected ref set<string> m_RuntimeNames;
	protected bool m_bFactionsRead;
	protected ref set<string> m_FactionKeys; //!< null when the world has no faction manager
	protected ref array<IEntitySource> m_aPlayableGroups;
	protected ref array<ResourceName> m_aPlayableSlots;
	protected ref map<string, int> m_mPrefabRanks = new map<string, int>();
	protected ref map<string, ref GDC_LoadoutAnalysis> m_mLoadouts = new map<string, ref GDC_LoadoutAnalysis>();

	//------------------------------------------------------------------------------------------------
	void GDC_MMCheckWorld(GDC_MMCheckContext context)
	{
		m_Context = context;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the mission skeleton of the plugin settings, null when it cannot be loaded
	GDC_MMMissionSkeletonConfig GetSkeleton()
	{
		if (m_bSkeletonRead)
			return m_Skeleton;

		m_bSkeletonRead = true;
		ResourceName config = m_Context.m_SkeletonConfig;
		if (config.IsEmpty())
			return null;

		Resource resource = Resource.Load(config);
		if (resource && resource.IsValid())
			m_Skeleton = GDC_MMMissionSkeletonConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));

		return m_Skeleton;
	}

	//------------------------------------------------------------------------------------------------
	//! \return class of the skeleton entry: its unique class, else its prefab's class; empty when its addon is not loaded
	static string GetEntryClass(notnull GDC_MMSkeletonEntity entry)
	{
		if (!entry.m_sUniqueClass.IsEmpty())
			return entry.m_sUniqueClass;

		return GetPrefabClass(entry.m_Prefab);
	}

	//------------------------------------------------------------------------------------------------
	//! \return class of the prefab's entity, empty when the prefab cannot be loaded (its addon is not loaded)
	static string GetPrefabClass(ResourceName prefab)
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
	//! The mission sub-scene first, where the skeleton places its entities; the other sub-scenes (terrain, large) only
	//! for the classes still missing. Each entity class is tested once. The children a prefab defines (the faction
	//! manager of the Reforger Lobby game mode, for example) are not in the world's entity list: they are reached
	//! through their parent.
	//! \param missionOnly classes only looked for in the mission sub-scene
	//! \return first entity found per class inheriting it (empty class names are ignored)
	map<string, IEntitySource> FindEntities(notnull array<string> classes, notnull set<string> missionOnly)
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
		bool missionScene;
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

			missionScene = s == m_Context.m_iSubScene;
			for (int i = 0, count = api.GetEntityCount(s); i < count && found.Count() < wanted.Count(); i++)
			{
				IEntitySource source = api.GetEntity(s, i);
				if (source)
					MatchEntity(source, missionScene, wanted, wantedTypes, missionOnly, matches, found);
			}
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	//! FindEntities for one entity and the children its prefab defines, depth first
	protected static void MatchEntity(IEntitySource source, bool missionScene, notnull array<string> wanted, notnull array<typename> wantedTypes,
		notnull set<string> missionOnly, notnull map<string, ref array<string>> matches, notnull map<string, IEntitySource> found)
	{
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
			if (!found.Contains(inheritedClass) && (missionScene || !missionOnly.Contains(inheritedClass)))
				found.Insert(inheritedClass, source);
		}

		for (int c = 0, children = source.GetNumChildren(); c < children && found.Count() < wanted.Count(); c++)
		{
			IEntitySource child = source.GetChild(c);
			if (child)
				MatchEntity(child, missionScene, wanted, wantedTypes, missionOnly, matches, found);
		}
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
	//! \return whether an entity holds this name: placed in the world (terrain included), or given when it spawns
	//! (m_setEntityName(s) of the TILW spawners and instructions, m_entityName of a TILW vehicle crew group, in any
	//! entity, component or object of the mission)
	bool IsNameKnown(string name)
	{
		if (name.IsEmpty())
			return false;

		if (m_Context.m_Api.FindEntityByName(name))
			return true;

		if (!m_RuntimeNames)
		{
			m_RuntimeNames = new set<string>();
			foreach (IEntitySource entity : m_Context.m_aEntities)
			{
				CollectRuntimeNames(entity);
				for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
				{
					CollectRuntimeNames(entity.GetComponent(i));
				}
			}
		}

		return m_RuntimeNames.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectRuntimeNames(BaseContainer container)
	{
		// the group a TILW vehicle crew creates takes this name
		if (GDC_MMCheckContext.Inherits(container.GetClassName(), CREW_GROUP))
		{
			string groupName;
			container.Get("m_entityName", groupName);
			m_RuntimeNames.Insert(groupName);
		}

		for (int i = 0, count = container.GetNumVars(); i < count; i++)
		{
			string name = container.GetVarName(i);
			DataVarType type = container.GetDataVarType(i);
			if (type == DataVarType.STRING && RUNTIME_NAME_VARS.Contains(name))
			{
				string value;
				container.Get(name, value);
				m_RuntimeNames.Insert(value);
			}
			else if (type == DataVarType.STRING_ARRAY && RUNTIME_NAME_VARS.Contains(name))
			{
				array<string> values = {};
				container.Get(name, values);
				foreach (string value : values)
				{
					m_RuntimeNames.Insert(value);
				}
			}
			else if (type == DataVarType.OBJECT)
			{
				BaseContainer child = container.GetObject(name);
				if (child)
					CollectRuntimeNames(child);
			}
			else if (type == DataVarType.OBJECT_ARRAY && name != "components")
			{
				BaseContainerList children = container.GetObjectArray(name);
				if (!children)
					continue;

				for (int c = 0, childCount = children.Count(); c < childCount; c++)
				{
					BaseContainer element = children.Get(c);
					if (element)
						CollectRuntimeNames(element);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return the faction keys of the world's faction manager (a child of the Reforger Lobby game mode, for example),
	//! null when the world has none: the rules about faction keys are then skipped
	set<string> GetFactionKeys()
	{
		if (m_bFactionsRead)
			return m_FactionKeys;

		m_bFactionsRead = true;
		array<string> classes = {};
		classes.Insert(FACTION_MANAGER);
		IEntitySource manager = FindEntities(classes, new set<string>()).Get(FACTION_MANAGER);
		if (!manager)
			return null;

		m_FactionKeys = new set<string>();
		BaseContainerList factions = manager.GetObjectArray("Factions");
		if (!factions)
			return m_FactionKeys;

		for (int i = 0, count = factions.Count(); i < count; i++)
		{
			BaseContainer faction = factions.Get(i);
			string key;
			if (faction && faction.Get("FactionKey", key) && !key.IsEmpty())
				m_FactionKeys.Insert(key);
		}

		return m_FactionKeys;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the AI groups of the mission with Set Playable ticked
	array<IEntitySource> GetPlayableGroups()
	{
		if (m_aPlayableGroups)
			return m_aPlayableGroups;

		m_aPlayableGroups = {};
		foreach (IEntitySource entity : m_Context.m_aEntities)
		{
			if (!GDC_MMCheckContext.Inherits(entity.GetClassName(), GROUP))
				continue;

			bool isPlayable;
			entity.Get("m_bSetPlayable", isPlayable);
			if (isPlayable)
				m_aPlayableGroups.Insert(entity);
		}

		return m_aPlayableGroups;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the slot prefabs of the playable groups, each once
	array<ResourceName> GetPlayableSlots()
	{
		if (m_aPlayableSlots)
			return m_aPlayableSlots;

		m_aPlayableSlots = {};
		foreach (IEntitySource group : GetPlayableGroups())
		{
			array<ResourceName> slots = {};
			group.Get("m_aUnitPrefabSlots", slots);
			foreach (ResourceName slot : slots)
			{
				if (!slot.IsEmpty() && !m_aPlayableSlots.Contains(slot))
					m_aPlayableSlots.Insert(slot);
			}
		}

		return m_aPlayableSlots;
	}

	//------------------------------------------------------------------------------------------------
	//! \return highest rank of the playable slots, NO_RANK when none has a rank
	int GetHighestPlayableRank()
	{
		int highest = NO_RANK;
		foreach (ResourceName slot : GetPlayableSlots())
		{
			int rank = GetPrefabRank(slot);
			if (rank != SCR_ECharacterRank.INVALID && rank > highest)
				highest = rank;
		}

		return highest;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "CAPTAIN", or "none" for NO_RANK
	static string RankName(int rank)
	{
		if (rank == NO_RANK)
			return GDC_MMCheckText.T("none", "aucun");

		return typename.EnumToString(SCR_ECharacterRank, rank);
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether a module asking for this minimum rank can be used by some playable slot (INVALID: no requirement)
	bool IsRankReachable(int minRank)
	{
		return minRank == SCR_ECharacterRank.INVALID || minRank <= GetHighestPlayableRank();
	}

	//------------------------------------------------------------------------------------------------
	//! \return rank set on the character prefab (SCR_CharacterRankComponent.m_iRank, inherited values included), read once
	int GetPrefabRank(ResourceName prefab)
	{
		int rank;
		if (m_mPrefabRanks.Find(prefab, rank))
			return rank;

		rank = SCR_ECharacterRank.INVALID;
		Resource resource = Resource.Load(prefab);
		if (resource && resource.IsValid())
		{
			IEntitySource source = resource.GetResource().ToEntitySource();
			if (source)
			{
				for (int i = 0, count = source.GetComponentCount(); i < count; i++)
				{
					IEntityComponentSource component = source.GetComponent(i);
					if (GDC_MMCheckContext.Inherits(component.GetClassName(), RANK_COMPONENT))
					{
						component.Get("m_iRank", rank);
						break;
					}
				}
			}
		}

		m_mPrefabRanks.Insert(prefab, rank);
		return rank;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the loadout of a slot prefab (GDC_LoadoutAnalysis, as the Loadout Editor reads it), null when not readable
	GDC_LoadoutAnalysis GetLoadout(ResourceName slot)
	{
		GDC_LoadoutAnalysis loadout;
		if (m_mLoadouts.Find(slot, loadout))
			return loadout;

		loadout = GDC_LoadoutAnalysis.ReadPrefab(slot);
		m_mLoadouts.Insert(slot, loadout);
		return loadout;
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether some playable slot starts with this exact prefab (worn, weapon, equipment slot or inventory)
	bool IsCarriedBySomeSlot(ResourceName prefab)
	{
		foreach (ResourceName slot : GetPlayableSlots())
		{
			GDC_LoadoutAnalysis loadout = GetLoadout(slot);
			if (loadout && loadout.Carries(prefab))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static string PrefabName(ResourceName prefab)
	{
		return FilePath.StripExtension(FilePath.StripPath(prefab.GetPath()));
	}
}
#endif
