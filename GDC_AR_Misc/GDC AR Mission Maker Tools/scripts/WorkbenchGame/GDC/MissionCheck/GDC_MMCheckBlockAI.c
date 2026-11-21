#ifdef WORKBENCH
//! Block 6, AI: groups, waypoints and the AI budget, as the World Editor can read them. Whether a waypoint can be
//! reached is not checked: in the World Editor the navmesh answers on Arland but not on terrains with a tiled navmesh
//! (Zimnitrita, Gulfcoast), so paths belong to the tests in Play (Tools/MissionTests). TILW classes are looked up by
//! name: no compile dependency on TILW.
//! - 6.1 (error) AI group without faction, placed or spawned by a TILW spawner or instruction;
//! - 6.2 (error) waypoint or waypoint target named by a group, a cycle or an entity waypoint, that no entity holds;
//! - 6.3 (error) group vehicle (m_aStaticVehicles) missing or not usable by the AI;
//! - 6.4 (info) waypoints nothing names;
//! - 6.5 (warning) AI group, AI character or waypoint with nothing (ground or object) just below it;
//! - 6.6 (info) AI budget: at start, spawned later, against the active AI limit;
//! - 6.7 (warning) more AI at start than the active AI limit.
class GDC_MMCheckBlockAI : GDC_MMCheckBlock
{
	protected static const string GROUP = "SCR_AIGroup";
	protected static const string WAYPOINT = "AIWaypoint";
	protected static const string CYCLE = "AIWaypointCycle";
	protected static const string CHARACTER = "SCR_ChimeraCharacter";
	protected static const string AI_WORLD = "SCR_AIWorld";
	protected static const string AI_LIMIT = "Active AI limit";
	protected static const string VEHICLE_USAGE = "SCR_AIVehicleUsageComponent";
	protected static const string SCENARIO_FRAMEWORK = "SCR_ScenarioFramework";
	protected static const string SPAWNER = "TILW_PrefabSpawnerEntity";
	protected static const string FRAMEWORK = "TILW_MissionFrameworkEntity";
	protected static const string INSTRUCTION = "TILW_BaseInstruction";
	protected static const string RUN_EVENTS = "TILW_RunEventsInstruction";
	protected static const string REACTIVATE_EVENTS = "TILW_ReactivateEventsInstruction";
	protected static const float SEARCH_ABOVE = 0.5; //!< 6.5: the trace starts this high above the entity...
	protected static const float SEARCH_BELOW = 1.5; //!< ...and looks for a surface down to this depth below it

	protected GDC_MMCheckContext m_Context;
	protected GDC_MMCheckReport m_Report;
	protected GDC_MMCheckWorld m_World;

	protected int m_iStartAI;
	protected int m_iLaterAI;
	protected bool m_bRepeats;
	protected ref map<string, int> m_mPrefabMembers = new map<string, int>();
	protected ref map<string, bool> m_mPrefabWithoutFaction = new map<string, bool>();
	protected ref set<string> m_ReferencedNames = new set<string>();

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

		array<IEntitySource> waypoints = {};
		foreach (IEntitySource entity : context.m_aEntities)
		{
			string className = entity.GetClassName();
			if (GDC_MMCheckContext.Inherits(className, GROUP))
			{
				CheckGroup(entity);
			}
			else if (GDC_MMCheckContext.Inherits(className, WAYPOINT))
			{
				waypoints.Insert(entity);
				CheckWaypoint(entity, className);
			}
			else if (GDC_MMCheckContext.Inherits(className, CHARACTER))
			{
				m_iStartAI++;
				if (IsOverVoid(entity, true))
					m_Report.Add("6.5", entity, string.Empty, SEARCH_BELOW.ToString());
			}
			else if (GDC_MMCheckContext.Inherits(className, SPAWNER))
			{
				CountSpawner(entity);
			}
			else if (GDC_MMCheckContext.Inherits(className, FRAMEWORK))
			{
				CountMissionEvents(entity);
			}

			CollectNames(entity);
			for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
			{
				CollectNames(entity.GetComponent(i));
			}
		}

		ReportOrphanWaypoints(waypoints);
		ReportBudget();
	}

	// ---- groups and waypoints ------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! 6.1 (not for a playable group: rule 3.2), 6.2, 6.3, 6.5
	protected void CheckGroup(IEntitySource group)
	{
		bool isPlayable;
		group.Get("m_bSetPlayable", isPlayable);
		array<ResourceName> slots = {};
		group.Get("m_aUnitPrefabSlots", slots);
		if (!isPlayable)
		{
			m_iStartAI += slots.Count();
			string faction;
			group.Get("m_faction", faction);
			if (faction.IsEmpty() && !slots.IsEmpty())
				m_Report.Add("6.1", group, "m_faction", GDC_MMCheckText.T("this group", "ce groupe"));
		}

		array<string> waypoints = {};
		group.Get("m_aStaticWaypoints", waypoints);
		ReportMissingWaypoints(group, "m_aStaticWaypoints", GDC_MMCheckText.T("this group", "ce groupe"), waypoints, true);

		array<string> vehicles = {};
		group.Get("m_aStaticVehicles", vehicles);
		array<string> unusable = {};
		foreach (string vehicle : vehicles)
		{
			if (!vehicle.IsEmpty() && !unusable.Contains(vehicle) && !IsUsableVehicle(vehicle))
				unusable.Insert(vehicle);
		}

		if (!unusable.IsEmpty())
			m_Report.Add("6.3", group, "m_aStaticVehicles", string.Join(", ", unusable, true));

		if (IsOverVoid(group, false))
			m_Report.Add("6.5", group, string.Empty, SEARCH_BELOW.ToString());
	}

	//------------------------------------------------------------------------------------------------
	//! 6.2 for the waypoints a cycle lists and the target of an entity waypoint, 6.5
	protected void CheckWaypoint(IEntitySource waypoint, string className)
	{
		if (GDC_MMCheckContext.Inherits(className, CYCLE))
		{
			array<string> cycled = {};
			waypoint.Get("CycleWaypoints", cycled);
			ReportMissingWaypoints(waypoint, "CycleWaypoints", GDC_MMCheckText.T("this cycle waypoint", "ce waypoint Cycle"), cycled, true);
		}

		BaseContainer parameters = waypoint.GetObject("m_EntityWaypointParameters");
		if (parameters)
		{
			string target;
			parameters.Get("m_sEntityName", target);
			array<string> targets = {};
			if (!target.IsEmpty())
				targets.Insert(target);

			ReportMissingWaypoints(waypoint, "m_sEntityName", GDC_MMCheckText.T("this waypoint", "ce waypoint"), targets, false);
		}

		if (IsOverVoid(waypoint, false))
			m_Report.Add("6.5", waypoint, string.Empty, SEARCH_BELOW.ToString());
	}

	//------------------------------------------------------------------------------------------------
	//! 6.2: one finding listing the names no entity holds (or, for a waypoint list, held by an entity that is not one)
	protected void ReportMissingWaypoints(IEntitySource where, string property, string who, notnull array<string> names, bool mustBeWaypoint)
	{
		array<string> missing = {};
		foreach (string name : names)
		{
			if (name.IsEmpty() || missing.Contains(name))
				continue;

			IEntitySource holder = m_Context.m_Api.FindEntityByName(name);
			if (holder)
			{
				if (mustBeWaypoint && !GDC_MMCheckContext.Inherits(holder.GetClassName(), WAYPOINT))
					missing.Insert(name);
			}
			else if (!m_World.IsNameKnown(name))
			{
				missing.Insert(name);
			}
		}

		if (!missing.IsEmpty())
			m_Report.Add("6.2", where, property, who, string.Join(", ", missing, true));
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether the AI can use the vehicle of this name (SCR_AIGroup.AddVehiclesStatic looks for
	//! SCR_AIVehicleUsageComponent on the entity or a parent); a name only given at spawn counts as usable
	protected bool IsUsableVehicle(string name)
	{
		IEntitySource vehicle = m_Context.m_Api.FindEntityByName(name);
		if (!vehicle)
			return m_World.IsNameKnown(name);

		while (vehicle)
		{
			for (int i = 0, count = vehicle.GetComponentCount(); i < count; i++)
			{
				if (GDC_MMCheckContext.Inherits(vehicle.GetComponent(i).GetClassName(), VEHICLE_USAGE))
					return true;
			}

			vehicle = vehicle.GetParent();
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! 6.5: a trace from just above the entity finds no surface (ground or object) just below it
	//! \param excludeSelf the entity has a body of its own (a character) that the trace must ignore
	protected bool IsOverVoid(IEntitySource source, bool excludeSelf)
	{
		IEntity entity = m_Context.m_Api.SourceToEntity(source);
		if (!entity)
			return false;

		vector origin = entity.GetOrigin();
		TraceParam trace = new TraceParam();
		trace.Start = origin + Vector(0, SEARCH_ABOVE, 0);
		trace.End = origin - Vector(0, SEARCH_BELOW, 0);
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		if (excludeSelf)
			trace.Exclude = entity;

		return m_Context.m_Api.GetWorld().TraceMove(trace, null) >= 1;
	}

	//------------------------------------------------------------------------------------------------
	//! 6.4: one finding for the whole mission. A waypoint counts as used when any setting of the mission holds its
	//! name (groups, cycles, TILW, scripted components...), or when it belongs to a Scenario Framework entity.
	protected void ReportOrphanWaypoints(notnull array<IEntitySource> waypoints)
	{
		array<string> orphans = {};
		int unnamed;
		foreach (IEntitySource waypoint : waypoints)
		{
			if (IsInScenarioFramework(waypoint))
				continue;

			string name = waypoint.GetName();
			if (name.IsEmpty())
				unnamed++;
			else if (!m_ReferencedNames.Contains(name))
				orphans.Insert(name);
		}

		int total = orphans.Count() + unnamed;
		if (unnamed > 0)
			orphans.Insert(string.Format(GDC_MMCheckText.T("%1 without a name", "%1 sans nom"), unnamed));

		if (total > 0)
			m_Report.Add("6.4", null, string.Empty, total.ToString(), string.Join(", ", orphans, true));
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsInScenarioFramework(IEntitySource entity)
	{
		IEntitySource parent = entity.GetParent();
		while (parent)
		{
			if (parent.GetClassName().StartsWith(SCENARIO_FRAMEWORK))
				return true;

			parent = parent.GetParent();
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Every name typed in a setting of the mission (strings and lists of strings, objects included), for 6.4
	protected void CollectNames(BaseContainer container)
	{
		for (int i = 0, count = container.GetNumVars(); i < count; i++)
		{
			string name = container.GetVarName(i);
			DataVarType type = container.GetDataVarType(i);
			if (type == DataVarType.STRING)
			{
				string value;
				container.Get(name, value);
				if (!value.IsEmpty())
					m_ReferencedNames.Insert(value);
			}
			else if (type == DataVarType.STRING_ARRAY)
			{
				array<string> values = {};
				container.Get(name, values);
				foreach (string value : values)
				{
					m_ReferencedNames.Insert(value);
				}
			}
			else if (type == DataVarType.OBJECT)
			{
				BaseContainer child = container.GetObject(name);
				if (child)
					CollectNames(child);
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
						CollectNames(element);
				}
			}
		}
	}

	// ---- budget and spawned groups --------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! A TILW prefab spawner: at start without a condition flag, later with one; once per spawn timing
	protected void CountSpawner(IEntitySource spawner)
	{
		ResourceName prefab;
		spawner.Get("m_prefab", prefab);
		int members = CountSpawned(spawner, prefab);
		if (members == 0)
			return;

		array<float> timings = {};
		spawner.Get("m_spawnTimings", timings);
		if (timings.Count() > 1)
			members *= timings.Count();
		string condition;
		spawner.Get("m_conditionFlag", condition);
		if (condition.IsEmpty())
			m_iStartAI += members;
		else
			m_iLaterAI += members;
	}

	//------------------------------------------------------------------------------------------------
	//! The TILW spawn instructions of the mission events: later; repeatable when the event is reactivated or rerun
	protected void CountMissionEvents(IEntitySource framework)
	{
		BaseContainerList events = framework.GetObjectArray("m_missionEvents");
		if (!events)
			return;

		set<string> repeated = new set<string>();
		for (int e = 0, count = events.Count(); e < count; e++)
		{
			BaseContainer missionEvent = events.Get(e);
			BaseContainerList instructions;
			if (missionEvent)
				instructions = missionEvent.GetObjectArray("m_instructions");

			for (int i = 0, instructionCount = CountOf(instructions); i < instructionCount; i++)
			{
				BaseContainer instruction = instructions.Get(i);
				if (!instruction)
					continue;

				string className = instruction.GetClassName();
				bool rerun;
				instruction.Get("m_allowRerun", rerun);
				if (GDC_MMCheckContext.Inherits(className, REACTIVATE_EVENTS) || (GDC_MMCheckContext.Inherits(className, RUN_EVENTS) && rerun))
				{
					array<string> names = {};
					instruction.Get("m_eventNames", names);
					foreach (string name : names)
					{
						repeated.Insert(name);
					}
				}
			}
		}

		for (int e = 0, count = events.Count(); e < count; e++)
		{
			BaseContainer missionEvent = events.Get(e);
			if (!missionEvent)
				continue;

			string eventName;
			missionEvent.Get("m_name", eventName);
			BaseContainerList instructions = missionEvent.GetObjectArray("m_instructions");
			for (int i = 0, instructionCount = CountOf(instructions); i < instructionCount; i++)
			{
				BaseContainer instruction = instructions.Get(i);
				if (!instruction || !GDC_MMCheckContext.Inherits(instruction.GetClassName(), INSTRUCTION) || instruction.GetVarIndex("m_prefab") < 0)
					continue;

				ResourceName prefab;
				instruction.Get("m_prefab", prefab);
				int members = CountSpawned(framework, prefab);
				m_iLaterAI += members;
				if (members > 0 && repeated.Contains(eventName))
					m_bRepeats = true;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return AI characters the prefab brings (group members, or one character); 6.1 for a group prefab without
	//! faction, reported once per prefab on the entity that spawns it
	protected int CountSpawned(IEntitySource spawner, ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return 0;

		int members;
		if (!m_mPrefabMembers.Find(prefab, members))
		{
			bool withoutFaction;
			Resource resource = Resource.Load(prefab);
			IEntitySource source;
			if (resource && resource.IsValid())
				source = resource.GetResource().ToEntitySource();

			if (source)
			{
				string className = source.GetClassName();
				if (GDC_MMCheckContext.Inherits(className, GROUP))
				{
					array<ResourceName> slots = {};
					source.Get("m_aUnitPrefabSlots", slots);
					members = slots.Count();
					string faction;
					source.Get("m_faction", faction);
					withoutFaction = faction.IsEmpty() && members > 0;
				}
				else if (GDC_MMCheckContext.Inherits(className, CHARACTER))
				{
					members = 1;
				}
			}

			m_mPrefabMembers.Insert(prefab, members);
			m_mPrefabWithoutFaction.Insert(prefab, withoutFaction);
			if (withoutFaction)
				m_Report.Add("6.1", spawner, string.Empty, string.Format(GDC_MMCheckText.T("the group prefab %1, spawned by this entity,",
					"le prefab de groupe %1, apparu par cette entité,"), GDC_MMCheckWorld.PrefabName(prefab)));
		}

		return members;
	}

	//------------------------------------------------------------------------------------------------
	//! 6.6 and 6.7, when the mission has an AI world and some AI
	protected void ReportBudget()
	{
		if (m_iStartAI + m_iLaterAI == 0)
			return;

		array<string> classes = {};
		classes.Insert(AI_WORLD);
		IEntitySource aiWorld = m_World.FindEntities(classes, new set<string>()).Get(AI_WORLD);
		if (!aiWorld || aiWorld.GetVarIndex(AI_LIMIT) < 0)
			return;

		int limit;
		aiWorld.Get(AI_LIMIT, limit);
		string later = m_iLaterAI.ToString();
		if (m_bRepeats)
			later += GDC_MMCheckText.T(" (some spawns can repeat)", " (certaines apparitions peuvent se répéter)");

		m_Report.Add("6.6", aiWorld, string.Empty, m_iStartAI.ToString(), later, limit.ToString());
		if (m_iStartAI > limit)
			m_Report.Add("6.7", aiWorld, AI_LIMIT, m_iStartAI.ToString(), limit.ToString());
	}

	//------------------------------------------------------------------------------------------------
	protected static int CountOf(BaseContainerList list)
	{
		if (!list)
			return 0;

		return list.Count();
	}
}
#endif
