#ifdef WORKBENCH
//! Block 4, system settings: a setting that breaks or degrades a system on its own (what feeds the mission logic, the
//! flags, belongs to block 5). Classes of addons are looked up by name: no compile dependency on TILW or GDC AR Lib.
//! - 4.1 [TILW] (warning) a PK_EntityDeleter without a prefab filter deletes every root entity in its sphere;
//! - 4.2 [TILW] no TILW_VehicleCrewComponent (dead code with Reforger Lobby since TILW dropped its RL dependency);
//! - 4.3 [GDC] (warning) the values the mission skeleton sets on its entities (game mode, AI world) are kept;
//! - 4.4 a faction key typed in the mission (groups, TILW triggers, fire support marker) exists among the factions of
//!   the world (skipped without faction manager);
//! - 4.5 to 4.10 [GDC] GDC AR Lib systems: teleport destination and rank, fire support usable, areas, ammunition, AI
//!   artillery.
//! "A name nobody holds": no entity of the world has it and no spawner or instruction gives it (GDC_MMCheckWorld).
class GDC_MMCheckBlockSettings : GDC_MMCheckBlock
{
	protected static const string ENTITY_DELETER = "PK_EntityDeleter";
	protected static const string GROUP = "SCR_AIGroup";
	protected static const string VEHICLE_CREW = "TILW_VehicleCrewComponent";
	protected static const string TRIGGER = "TILW_BaseTriggerEntity";
	protected static const string FACTION_TRIGGER = "TILW_FactionTriggerEntity";
	protected static const string TELEPORT = "GDC_TeleportModuleComponent";
	protected static const string SUPPORT = "GDC_SupportModuleComponent";
	protected static const string FIRE_SUPPORT = "GDC_FireSupportModuleComponent";
	protected static const string ARTILLERY = "GDC_ArtilleryFireSupportComponent";
	protected static const int TELEPORT_ALL_PLAYERS = 2; //!< GDC_ETeleportMode: INDIVIDUAL, GROUP, ALL_PLAYERS
	protected static const int AREAS_NONE = 0; //!< GDC_ESupportAreaMode: NONE, WHITELIST, BLACKLIST
	protected static const int AREAS_WHITELIST = 1;

	protected GDC_MMCheckContext m_Context;
	protected GDC_MMCheckReport m_Report;
	protected GDC_MMCheckWorld m_World;

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
		m_World = context.m_World;

		CheckSkeletonValues();
		set<string> factions = m_World.GetFactionKeys();
		bool tilw = context.Uses(GDC_MMCheckOrigin.TILW);
		bool gdc = context.Uses(GDC_MMCheckOrigin.GDC);

		foreach (IEntitySource entity : context.m_aEntities)
		{
			string className = entity.GetClassName();
			if (GDC_MMCheckContext.Inherits(className, GROUP))
				CheckFaction(factions, entity, "m_faction");

			if (tilw && GDC_MMCheckContext.Inherits(className, ENTITY_DELETER))
				CheckEntityDeleter(entity);

			if (tilw && GDC_MMCheckContext.Inherits(className, TRIGGER))
				CheckTriggerFactions(entity, factions);

			for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
			{
				IEntityComponentSource component = entity.GetComponent(i);
				string componentClass = component.GetClassName();
				if (tilw && GDC_MMCheckContext.Inherits(componentClass, VEHICLE_CREW))
					m_Report.Add("4.2", entity);

				if (!gdc)
					continue;

				if (GDC_MMCheckContext.Inherits(componentClass, TELEPORT))
					CheckTeleport(entity, component);

				if (GDC_MMCheckContext.Inherits(componentClass, SUPPORT))
					CheckSupport(entity, component, factions);

				if (GDC_MMCheckContext.Inherits(componentClass, ARTILLERY))
					CheckArtillery(entity, component);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 4.1
	protected void CheckEntityDeleter(IEntitySource entity)
	{
		array<ResourceName> filter = {};
		entity.Get("m_prefabFilterList", filter);
		if (filter.IsEmpty())
			m_Report.Add("4.1", entity);
	}

	//------------------------------------------------------------------------------------------------
	//! 4.3: one finding per entity, listing its values that differ from the skeleton's
	protected void CheckSkeletonValues()
	{
		GDC_MMMissionSkeletonConfig skeleton = m_World.GetSkeleton();
		if (!skeleton || !skeleton.m_aEntities)
			return;

		array<ref GDC_MMSkeletonEntity> entries = {};
		array<string> classes = {};
		set<string> missionOnly = new set<string>();
		foreach (GDC_MMSkeletonEntity entry : skeleton.m_aEntities)
		{
			if (!entry.m_aValues || entry.m_aValues.IsEmpty())
				continue;

			string className = GDC_MMCheckWorld.GetEntryClass(entry);
			if (className.IsEmpty())
				continue;

			entries.Insert(entry);
			classes.Insert(className);
			if (entry.m_bMissionSubSceneOnly)
				missionOnly.Insert(className);
		}

		map<string, IEntitySource> found = m_World.FindEntities(classes, missionOnly);
		foreach (int e, GDC_MMSkeletonEntity expected : entries)
		{
			IEntitySource entity = found.Get(classes[e]);
			if (!entity)
				continue; // rule 1.3 or 1.2

			array<string> differences = {};
			foreach (GDC_MMSkeletonValue value : expected.m_aValues)
			{
				string actual;
				if (ReadValue(entity, value.m_sKey, value.m_sValue, actual))
					continue;

				differences.Insert(string.Format(GDC_MMCheckText.T("%1: %2 instead of %3", "%1 : %2 au lieu de %3"), value.m_sKey, actual, value.m_sValue));
			}

			if (!differences.IsEmpty())
				m_Report.Add("4.3", entity, string.Empty, string.Join(", ", differences, true));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \param[out] actual the value found, as written in a layer file ("1" for true)
	//! \return true when the value equals the expected one, or cannot be compared (unknown property or type)
	protected static bool ReadValue(BaseContainer container, string key, string expected, out string actual)
	{
		int index = container.GetVarIndex(key);
		if (index < 0)
			return true;

		DataVarType type = container.GetDataVarType(index);
		if (type == DataVarType.BOOLEAN)
		{
			bool flag;
			container.Get(key, flag);
			actual = "0";
			if (flag)
				actual = "1";

			return actual == expected;
		}

		if (type == DataVarType.INTEGER)
		{
			int whole;
			container.Get(key, whole);
			actual = whole.ToString();
			return whole == expected.ToInt();
		}

		if (type == DataVarType.SCALAR)
		{
			float real;
			container.Get(key, real);
			actual = real.ToString();
			return Math.AbsFloat(real - expected.ToFloat()) < 0.0001;
		}

		if (type == DataVarType.STRING)
		{
			container.Get(key, actual);
			return actual == expected;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! 4.4 for a single faction key read from the entity
	protected void CheckFaction(set<string> factions, IEntitySource entity, string property)
	{
		string key;
		entity.Get(property, key);
		CheckFactionKey(factions, entity, property, key);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckFactionKey(set<string> factions, IEntitySource entity, string property, string key)
	{
		if (factions && !key.IsEmpty() && !factions.Contains(key))
			m_Report.Add("4.4", entity, property, key);
	}

	//------------------------------------------------------------------------------------------------
	//! 4.4 for a list of faction keys
	protected void CheckFactionList(set<string> factions, IEntitySource entity, string property)
	{
		array<string> keys = {};
		entity.Get(property, keys);
		foreach (string key : keys)
		{
			CheckFactionKey(factions, entity, property, key);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 4.4 for the faction keys of a TILW trigger (its other settings: block 5)
	protected void CheckTriggerFactions(IEntitySource trigger, set<string> factions)
	{
		CheckFactionList(factions, trigger, "m_displayTo");
		if (!GDC_MMCheckContext.Inherits(trigger.GetClassName(), FACTION_TRIGGER))
			return;

		CheckFaction(factions, trigger, "m_ownerFactionKey");
		CheckFactionList(factions, trigger, "m_ignoredFactions");
	}

	//------------------------------------------------------------------------------------------------
	//! \return the names nobody holds, separated by commas, empty if every name is held
	protected string UnknownNames(notnull array<string> names)
	{
		array<string> unknown = {};
		foreach (string name : names)
		{
			if (!m_World.IsNameKnown(name))
				unknown.Insert(name);
		}

		return string.Join(", ", unknown, true);
	}

	//------------------------------------------------------------------------------------------------
	//! 4.5 and 4.6
	protected void CheckTeleport(IEntitySource entity, IEntityComponentSource teleport)
	{
		string destination;
		teleport.Get("m_sDestinationName", destination);
		if (destination.IsEmpty())
			m_Report.Add("4.5", entity, "m_sDestinationName", GDC_MMCheckText.T("no destination name", "aucun nom de destination"));
		else if (!m_World.IsNameKnown(destination))
			m_Report.Add("4.5", entity, "m_sDestinationName", GDC_MMCheckText.T("nothing holds the name ", "rien ne porte le nom ") + destination);

		int mode;
		teleport.Get("m_eMode", mode);
		int minRank;
		teleport.Get("m_eMinRankRequired", minRank);
		if (mode == TELEPORT_ALL_PLAYERS && !m_World.IsRankReachable(minRank))
			m_Report.Add("4.6", entity, string.Empty, GDC_MMCheckWorld.RankName(minRank), GDC_MMCheckWorld.RankName(m_World.GetHighestPlayableRank()));
	}

	//------------------------------------------------------------------------------------------------
	//! 4.4 and 4.7 to 4.9
	protected void CheckSupport(IEntitySource entity, IEntityComponentSource support, set<string> factions)
	{
		// 4.7: some playable slot can call it
		array<string> unusable = {};
		int minRank;
		support.Get("m_eMinRankRequired", minRank);
		if (!m_World.IsRankReachable(minRank))
			unusable.Insert(string.Format(GDC_MMCheckText.T("it asks for the rank %1, the highest slot is %2", "il exige le grade %1, le slot le plus haut est %2"),
				GDC_MMCheckWorld.RankName(minRank), GDC_MMCheckWorld.RankName(m_World.GetHighestPlayableRank())));

		array<ResourceName> items = {};
		support.Get("m_aRequiredItems", items);
		bool carried = items.IsEmpty();
		foreach (ResourceName item : items)
		{
			if (!carried && m_World.IsCarriedBySomeSlot(item))
				carried = true;
		}

		if (!carried)
			unusable.Insert(GDC_MMCheckText.T("no playable slot starts with one of its required items", "aucun slot jouable ne démarre avec un de ses objets requis"));

		if (!unusable.IsEmpty())
			m_Report.Add("4.7", entity, string.Empty, string.Join("; ", unusable, true));

		// 4.8: its areas
		int areaMode;
		support.Get("m_eFireAreaMode", areaMode);
		array<string> areas = {};
		support.Get("m_aAreaNames", areas);
		if (areaMode == AREAS_WHITELIST && areas.IsEmpty())
		{
			m_Report.Add("4.8", entity, "m_aAreaNames", GDC_MMCheckText.T("WHITELIST mode without any area: no target is allowed", "mode WHITELIST sans aucune zone : aucune cible n'est permise"));
		}
		else if (areaMode != AREAS_NONE)
		{
			string unknown = UnknownNames(areas);
			if (!unknown.IsEmpty())
				m_Report.Add("4.8", entity, "m_aAreaNames", GDC_MMCheckText.T("areas nobody holds: ", "zones que rien ne porte : ") + unknown);
		}

		if (!GDC_MMCheckContext.Inherits(support.GetClassName(), FIRE_SUPPORT))
			return;

		// 4.4 and 4.9: fire support
		string markerFaction;
		support.Get("m_sMarkerVisibleToFaction", markerFaction);
		CheckFactionKey(factions, entity, "m_sMarkerVisibleToFaction", markerFaction);

		array<string> problems = {};
		BaseContainerList ammo = support.GetObjectArray("m_aAmmo");
		if (!ammo || ammo.Count() == 0)
			problems.Insert(GDC_MMCheckText.T("no ammunition", "aucune munition"));

		int minDispersion;
		support.Get("m_iMinDispersion", minDispersion);
		int maxDispersion;
		support.Get("m_iMaxDispersion", maxDispersion);
		if (minDispersion > maxDispersion)
			problems.Insert(string.Format(GDC_MMCheckText.T("minimum dispersion %1 m above the maximum %2 m", "dispersion minimale de %1 m supérieure à la maximale de %2 m"), minDispersion, maxDispersion));

		if (!problems.IsEmpty())
			m_Report.Add("4.9", entity, string.Empty, string.Join("; ", problems, true));
	}

	//------------------------------------------------------------------------------------------------
	//! 4.10
	protected void CheckArtillery(IEntitySource entity, IEntityComponentSource artillery)
	{
		array<string> problems = {};
		string group;
		artillery.Get("m_sAIGroupName", group);
		if (group.IsEmpty())
			problems.Insert(GDC_MMCheckText.T("no mortar or group name", "aucun nom de mortier ou de groupe"));
		else if (!m_World.IsNameKnown(group))
			problems.Insert(GDC_MMCheckText.T("nothing holds the name ", "rien ne porte le nom ") + group);

		BaseContainerList orders = artillery.GetObjectArray("m_aFireOrders");
		if (!orders || orders.Count() == 0)
		{
			problems.Insert(GDC_MMCheckText.T("no fire order", "aucun ordre de tir"));
		}
		else
		{
			for (int i = 0, count = orders.Count(); i < count; i++)
			{
				BaseContainer order = orders.Get(i);
				if (!order)
					continue;

				float minRadius;
				order.Get("m_fMinRadius", minRadius);
				float maxRadius;
				order.Get("m_fMaxRadius", maxRadius);
				if (minRadius > maxRadius)
					problems.Insert(string.Format(GDC_MMCheckText.T("fire order %1: inner radius above the outer one", "ordre de tir %1 : rayon intérieur supérieur à l'extérieur"), i + 1));
			}
		}

		if (!problems.IsEmpty())
			m_Report.Add("4.10", entity, string.Empty, string.Join("; ", problems, true));
	}
}
#endif
