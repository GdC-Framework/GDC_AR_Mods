#ifdef WORKBENCH
//! Block 5, mission logic: what sets or reads the mission flags (triggers, interactions, mission events, conditioned
//! spawners). A setting that breaks a system on its own belongs to block 4. Classes are looked up by name: no compile
//! dependency on TILW.
//! - 5.1 [TILW] (warning) a TILW interaction is not the child of another entity (its label may break on a dedicated
//!   server);
//! - 5.2 to 5.5 [TILW] triggers: flag, owner faction, status messages, presence targets (their faction keys: rule 4.4);
//! - 5.6 to 5.14 [TILW] mission events of the TILW framework entity: conditions, flags nothing sets, events and entities
//!   the instructions aim at, several framework entities, pregame events, game end suppressed.
class GDC_MMCheckBlockLogic : GDC_MMCheckBlock
{
	protected static const string TILW_INTERACTION = "TILW_BaseInteraction";
	protected static const string ACTIONS_MANAGER = "ActionsManagerComponent";
	protected static const string TRIGGER = "TILW_BaseTriggerEntity";
	protected static const string FACTION_TRIGGER = "TILW_FactionTriggerEntity";
	protected static const string PRESENCE_TRIGGER = "TILW_PresenceTriggerEntity";
	protected static const int STATUS_MESSAGES = 6; //!< TILW_BaseTriggerEntity: exactly 6 status messages

	protected static const string FRAMEWORK = "TILW_MissionFrameworkEntity";
	protected static const string INSTRUCTION = "TILW_BaseInstruction";
	protected static const string RUN_EVENTS = "TILW_RunEventsInstruction";
	protected static const string REACTIVATE_EVENTS = "TILW_ReactivateEventsInstruction";
	protected static const string DEACTIVATE_EVENTS = "TILW_DeactivateEventsInstruction";
	protected static const string END_GAME = "TILW_EndGameInstruction";
	protected static const string OPERATION = "TILW_OperationTerm";
	protected static const string LITERAL = "TILW_LiteralTerm";
	protected static const string SPAWNER = "TILW_PrefabSpawnerEntity";
	protected static const string VEHICLE_CREW = "TILW_VehicleCrewComponent";
	protected static const string GDC_ARTILLERY = "GDC_ArtilleryFireSupportComponent";
	protected static const string DEFAULT_EVENT_NAME = "Mission Event"; //!< TILW_MissionEvent.m_name default value

	//! classes whose m_flagName (or m_flagNames) is a flag they set; TILW_ClearFlagInstruction only clears one
	protected static const ref array<string> FLAG_SETTERS = {
		"TILW_BaseTriggerEntity", "TILW_BaseInteraction", "TILW_SetFlagInstruction", "TILW_RandomFlagInstruction",
		"TILW_FlagSamplingInstruction", "TILW_MetaFlag", "TILW_BaseCasualtyFlag", "TILW_BaseRandomFlag",
		"TILW_Flag_EntityDamage", "TILW_ControlModeFlag"
	};
	//! prefabs the mission makes appear at runtime: the flag setters they hold count (TILW spawners and instructions,
	//! group members, vehicle crews, area spawning)
	protected static const ref array<string> SPAWNED_PREFAB_VARS = { "m_prefab", "m_prefabs", "m_aUnitPrefabSlots", "m_characters", "m_customCrew" };
	//! names of existing entities that TILW instructions, spawners and crews look up
	protected static const ref array<string> NAME_VARS = { "m_locationName", "m_groupName" };
	protected static const ref array<string> NAME_ARRAY_VARS = { "m_waypointNames", "m_entityNames", "m_itemNames", "m_aVehicleNames" };

	protected GDC_MMCheckReport m_Report;
	protected GDC_MMCheckWorld m_World;

	protected ref set<string> m_SetFlags = new set<string>();
	protected ref set<string> m_VisitedPrefabs = new set<string>();
	protected ref array<ref GDC_MMCheckFlagReader> m_aReaders = {};

	//------------------------------------------------------------------------------------------------
	override bool IsImplemented()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		if (!context.Uses(GDC_MMCheckOrigin.TILW))
			return;

		m_Report = report;
		m_World = context.m_World;
		array<IEntitySource> frameworks = {};
		foreach (IEntitySource entity : context.m_aEntities)
		{
			if (entity.GetParent() && HasTilwInteraction(entity))
				report.Add("5.1", entity);

			string className = entity.GetClassName();
			if (GDC_MMCheckContext.Inherits(className, TRIGGER))
				CheckTrigger(entity);

			if (GDC_MMCheckContext.Inherits(className, FRAMEWORK))
				frameworks.Insert(entity);

			CollectFlags(entity, entity);
			CheckEntityNames(entity);
		}

		CheckMissionEvents(frameworks);
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether one of the entity's additional actions is a TILW interaction
	protected static bool HasTilwInteraction(IEntitySource entity)
	{
		for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = entity.GetComponent(i);
			if (!GDC_MMCheckContext.Inherits(component.GetClassName(), ACTIONS_MANAGER))
				continue;

			BaseContainerList actions = component.GetObjectArray("additionalActions");
			if (!actions)
				continue;

			for (int a = 0, actionCount = actions.Count(); a < actionCount; a++)
			{
				BaseContainer action = actions.Get(a);
				if (action && GDC_MMCheckContext.Inherits(action.GetClassName(), TILW_INTERACTION))
					return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! 5.2 to 5.5
	protected void CheckTrigger(IEntitySource trigger)
	{
		string flag;
		trigger.Get("m_flagName", flag);
		if (flag.IsEmpty())
			m_Report.Add("5.2", trigger);

		bool sendMessages;
		trigger.Get("m_sendStatusMessages", sendMessages);
		array<string> messages = {};
		trigger.Get("m_statusMessages", messages);
		if (sendMessages && messages.Count() != STATUS_MESSAGES)
			m_Report.Add("5.4", trigger, "m_statusMessages", messages.Count().ToString());

		string className = trigger.GetClassName();
		if (GDC_MMCheckContext.Inherits(className, FACTION_TRIGGER))
		{
			string owner;
			trigger.Get("m_ownerFactionKey", owner);
			if (owner.IsEmpty())
				m_Report.Add("5.3", trigger);
		}

		if (!GDC_MMCheckContext.Inherits(className, PRESENCE_TRIGGER))
			return;

		array<string> names = {};
		trigger.Get("m_entityNames", names);
		if (names.IsEmpty())
		{
			m_Report.Add("5.5", trigger, "m_entityNames", GDC_MMCheckText.T("no entity name", "aucun nom d'entité"));
			return;
		}

		array<string> unknown = {};
		foreach (string name : names)
		{
			if (!m_World.IsNameKnown(name))
				unknown.Insert(name);
		}

		if (!unknown.IsEmpty())
			m_Report.Add("5.5", trigger, "m_entityNames", GDC_MMCheckText.T("names nobody holds: ", "noms que rien ne porte : ") + string.Join(", ", unknown, true));
	}

	// ---- flags ---------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Collects the flags a container sets, then walks its objects, components, children and the prefabs it makes
	//! appear. Inherited values count: a placed prefab's components are read as the prefab sets them.
	//! \param owner mission entity the container belongs to: its flag readers outside the mission events are recorded
	//! (interactions, spawners, GDC AI artillery); null inside a spawned prefab
	protected void CollectFlags(BaseContainer container, IEntitySource owner)
	{
		string className = container.GetClassName();
		if (IsFlagSetter(className))
		{
			string flag;
			if (container.Get("m_flagName", flag) && !flag.IsEmpty())
				m_SetFlags.Insert(flag);

			array<string> flags = {};
			if (container.Get("m_flagNames", flags))
			{
				foreach (string sampled : flags)
				{
					m_SetFlags.Insert(sampled);
				}
			}
		}

		if (owner)
		{
			if (GDC_MMCheckContext.Inherits(className, TILW_INTERACTION) || GDC_MMCheckContext.Inherits(className, SPAWNER))
				AddReader(container, owner, "m_conditionFlag");
			else if (GDC_MMCheckContext.Inherits(className, GDC_ARTILLERY))
				AddReader(container, owner, "m_sConditionFlag");
		}

		for (int i = 0, count = container.GetNumVars(); i < count; i++)
		{
			string name = container.GetVarName(i);
			DataVarType type = container.GetDataVarType(i);
			if (type == DataVarType.OBJECT)
			{
				BaseContainer child = container.GetObject(name);
				if (child)
					CollectFlags(child, owner);
			}
			else if (type == DataVarType.OBJECT_ARRAY && name != "components")
			{
				BaseContainerList children = container.GetObjectArray(name);
				for (int c = 0, childCount = CountOf(children); c < childCount; c++)
				{
					BaseContainer element = children.Get(c);
					if (element)
						CollectFlags(element, owner);
				}
			}
			else if (type == DataVarType.RESOURCE_NAME && SPAWNED_PREFAB_VARS.Contains(name))
			{
				ResourceName prefab;
				container.Get(name, prefab);
				CollectPrefabFlags(prefab);
			}
			else if (type == DataVarType.RESOURCE_NAME_ARRAY && SPAWNED_PREFAB_VARS.Contains(name))
			{
				array<ResourceName> prefabs = {};
				container.Get(name, prefabs);
				foreach (ResourceName prefab : prefabs)
				{
					CollectPrefabFlags(prefab);
				}
			}
		}

		IEntitySource entity = container.ToEntitySource();
		if (!entity)
			return;

		for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
		{
			CollectFlags(entity.GetComponent(i), owner);
		}

		// children defined by the prefab are only reached through their parent; those the mission maker placed are
		// in the entity list too, and setting a flag twice changes nothing
		for (int c = 0, children = entity.GetNumChildren(); c < children; c++)
		{
			IEntitySource child = entity.GetChild(c);
			if (child)
				CollectFlags(child, null);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! CollectFlags of a prefab the mission makes appear, each prefab once
	protected void CollectPrefabFlags(ResourceName prefab)
	{
		if (prefab.IsEmpty() || m_VisitedPrefabs.Contains(prefab))
			return;

		m_VisitedPrefabs.Insert(prefab);
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		IEntitySource source = resource.GetResource().ToEntitySource();
		if (source)
			CollectFlags(source, null);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsFlagSetter(string className)
	{
		foreach (string setter : FLAG_SETTERS)
		{
			if (GDC_MMCheckContext.Inherits(className, setter))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Records a condition flag read outside the mission events; an empty one means "no condition"
	protected void AddReader(BaseContainer container, IEntitySource owner, string property)
	{
		string flag;
		container.Get(property, flag);
		if (flag.IsEmpty())
			return;

		GDC_MMCheckFlagReader reader = new GDC_MMCheckFlagReader();
		reader.m_Entity = owner;
		reader.m_sProperty = property;
		reader.m_sWho = GDC_MMCheckText.T("this entity", "cette entité");
		reader.m_aFlags.Insert(flag);
		m_aReaders.Insert(reader);
	}

	// ---- names ---------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! 5.11 for the waypoints of the TILW prefab spawners and vehicle crews placed in the mission
	protected void CheckEntityNames(IEntitySource entity)
	{
		if (GDC_MMCheckContext.Inherits(entity.GetClassName(), SPAWNER))
			ReportUnknownNames(entity, entity, string.Empty, GDC_MMCheckText.T("this spawner", "ce spawner"));

		for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = entity.GetComponent(i);
			if (GDC_MMCheckContext.Inherits(component.GetClassName(), VEHICLE_CREW))
				ReportUnknownNames(component, entity, string.Empty, GDC_MMCheckText.T("its vehicle crew", "son équipage de véhicule"));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 5.11: one finding listing the names the container (and its objects) looks for that no entity holds
	protected void ReportUnknownNames(BaseContainer container, IEntitySource where, string property, string who)
	{
		array<string> unknown = {};
		CollectUnknownNames(container, unknown);
		if (!unknown.IsEmpty())
			m_Report.Add("5.11", where, property, who, string.Join(", ", unknown, true));
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectUnknownNames(BaseContainer container, notnull array<string> unknown)
	{
		for (int i = 0, count = container.GetNumVars(); i < count; i++)
		{
			string name = container.GetVarName(i);
			DataVarType type = container.GetDataVarType(i);
			if (type == DataVarType.STRING && NAME_VARS.Contains(name))
			{
				string value;
				container.Get(name, value);
				AddUnknownName(value, unknown);
			}
			else if (type == DataVarType.STRING_ARRAY && NAME_ARRAY_VARS.Contains(name))
			{
				array<string> values = {};
				container.Get(name, values);
				foreach (string value : values)
				{
					AddUnknownName(value, unknown);
				}
			}
			else if (type == DataVarType.OBJECT)
			{
				BaseContainer child = container.GetObject(name);
				if (child)
					CollectUnknownNames(child, unknown);
			}
			else if (type == DataVarType.OBJECT_ARRAY && name != "components")
			{
				BaseContainerList children = container.GetObjectArray(name);
				for (int c = 0, childCount = CountOf(children); c < childCount; c++)
				{
					BaseContainer element = children.Get(c);
					if (element)
						CollectUnknownNames(element, unknown);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddUnknownName(string name, notnull array<string> unknown)
	{
		if (!name.IsEmpty() && !unknown.Contains(name) && !m_World.IsNameKnown(name))
			unknown.Insert(name);
	}

	// ---- mission events ------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! 5.6 to 5.14
	protected void CheckMissionEvents(notnull array<IEntitySource> frameworks)
	{
		// 5.12: TILW keeps one framework entity (the last created); the events of the others are evaluated once, at start
		for (int f = 1, count = frameworks.Count(); f < count; f++)
		{
			m_Report.Add("5.12", frameworks[f], string.Empty, count.ToString());
		}

		map<string, int> eventNames = new map<string, int>();
		set<string> runByName = new set<string>();
		set<string> randomFlags = new set<string>();
		array<ref GDC_MMCheckFlagReader> conditions = {};
		array<ref GDC_MMCheckFlagReader> pregame = {};
		array<ref GDC_MMCheckEventReference> references = {};
		foreach (IEntitySource framework : frameworks)
		{
			bool endGame = ReadEvents(framework, eventNames, runByName, conditions, pregame, references);
			ReadMetaFlags(framework, conditions);
			ReadRandomFlags(framework, randomFlags);

			bool suppressed;
			framework.Get("m_suppressGameEnd", suppressed);
			if (suppressed && endGame)
				m_Report.Add("5.14", framework, "m_suppressGameEnd");
		}

		// 5.9, 5.10: each name aimed at once
		set<string> reported = new set<string>();
		foreach (GDC_MMCheckEventReference eventReference : references)
		{
			if (reported.Contains(eventReference.m_sName))
				continue;

			reported.Insert(eventReference.m_sName);
			int holders = eventNames.Get(eventReference.m_sName);
			if (holders == 0)
				m_Report.Add("5.9", eventReference.m_Framework, "m_missionEvents", eventReference.m_sWho, eventReference.m_sName);
			else if (holders > 1)
				m_Report.Add("5.10", eventReference.m_Framework, "m_missionEvents", eventReference.m_sName, holders.ToString());
		}

		// 5.8: a condition reading a flag nothing sets; an event run by name needs no reachable condition
		foreach (GDC_MMCheckFlagReader reader : m_aReaders)
		{
			conditions.Insert(reader);
		}

		foreach (GDC_MMCheckFlagReader condition : conditions)
		{
			if (!condition.m_sEvent.IsEmpty() && runByName.Contains(condition.m_sEvent))
				continue;

			set<string> seen = new set<string>();
			foreach (string flag : condition.m_aFlags)
			{
				if (seen.Contains(flag) || m_SetFlags.Contains(flag))
					continue;

				seen.Insert(flag);
				string shown = flag;
				if (flag.IsEmpty())
					shown = GDC_MMCheckText.T("(empty)", "(vide)");

				m_Report.Add("5.8", condition.m_Entity, condition.m_sProperty, condition.m_sWho, shown);
			}
		}

		// 5.13: before the briefing has ended, only the random flags are set yet
		foreach (GDC_MMCheckFlagReader pregameEvent : pregame)
		{
			array<string> notRandom = {};
			foreach (string flag : pregameEvent.m_aFlags)
			{
				if (!flag.IsEmpty() && !randomFlags.Contains(flag) && !notRandom.Contains(flag))
					notRandom.Insert(flag);
			}

			if (!notRandom.IsEmpty())
				m_Report.Add("5.13", pregameEvent.m_Entity, pregameEvent.m_sProperty, pregameEvent.m_sWho, string.Join(", ", notRandom, true));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Reads the mission events of a framework entity: 5.6, 5.7 and 5.11 at once, the rest gathered for the other rules
	//! \return whether one of its events ends the game
	protected bool ReadEvents(IEntitySource framework, notnull map<string, int> eventNames, notnull set<string> runByName,
		notnull array<ref GDC_MMCheckFlagReader> conditions, notnull array<ref GDC_MMCheckFlagReader> pregame,
		notnull array<ref GDC_MMCheckEventReference> references)
	{
		bool endGame;
		BaseContainerList events = framework.GetObjectArray("m_missionEvents");
		for (int e = 0, count = CountOf(events); e < count; e++)
		{
			BaseContainer missionEvent = events.Get(e);
			if (!missionEvent)
				continue;

			string name = DEFAULT_EVENT_NAME;
			missionEvent.Get("m_name", name);
			eventNames.Set(name, eventNames.Get(name) + 1);
			string who = string.Format(GDC_MMCheckText.T("the event \"%1\"", "l'événement « %1 »"), name);

			GDC_MMCheckFlagReader condition = new GDC_MMCheckFlagReader();
			condition.m_Entity = framework;
			condition.m_sProperty = "m_missionEvents";
			condition.m_sWho = who;
			condition.m_sEvent = name;
			BaseContainer term = missionEvent.GetObject("m_condition");
			if (!term)
				m_Report.Add("5.6", framework, "m_missionEvents", who);
			else
				ReadTerm(term, condition);

			conditions.Insert(condition);
			bool isPregame;
			missionEvent.Get("m_pregameEvent", isPregame);
			if (isPregame)
				pregame.Insert(condition);

			BaseContainerList instructions = missionEvent.GetObjectArray("m_instructions");
			for (int i = 0, instructionCount = CountOf(instructions); i < instructionCount; i++)
			{
				BaseContainer instruction = instructions.Get(i);
				if (!instruction)
					continue;

				string className = instruction.GetClassName();
				if (GDC_MMCheckContext.Inherits(className, END_GAME))
					endGame = true;

				bool runs = GDC_MMCheckContext.Inherits(className, RUN_EVENTS);
				if (runs || GDC_MMCheckContext.Inherits(className, REACTIVATE_EVENTS) || GDC_MMCheckContext.Inherits(className, DEACTIVATE_EVENTS))
					ReadEventReferences(instruction, framework, who, runs, runByName, references);

				if (GDC_MMCheckContext.Inherits(className, INSTRUCTION))
					ReportUnknownNames(instruction, framework, "m_missionEvents", who + " (" + className + ")");
			}
		}

		return endGame;
	}

	//------------------------------------------------------------------------------------------------
	protected void ReadEventReferences(BaseContainer instruction, IEntitySource framework, string who, bool runs,
		notnull set<string> runByName, notnull array<ref GDC_MMCheckEventReference> references)
	{
		array<string> names = {};
		instruction.Get("m_eventNames", names);
		foreach (string name : names)
		{
			if (runs)
				runByName.Insert(name);

			GDC_MMCheckEventReference eventReference = new GDC_MMCheckEventReference();
			eventReference.m_Framework = framework;
			eventReference.m_sWho = who;
			eventReference.m_sName = name;
			references.Insert(eventReference);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 5.6 for a meta flag without a definition (TILW then clears it and logs a script error at each evaluation)
	protected void ReadMetaFlags(IEntitySource framework, notnull array<ref GDC_MMCheckFlagReader> conditions)
	{
		BaseContainerList metaFlags = framework.GetObjectArray("m_metaFlags");
		for (int m = 0, count = CountOf(metaFlags); m < count; m++)
		{
			BaseContainer metaFlag = metaFlags.Get(m);
			if (!metaFlag)
				continue;

			string name;
			metaFlag.Get("m_flagName", name);
			GDC_MMCheckFlagReader definition = new GDC_MMCheckFlagReader();
			definition.m_Entity = framework;
			definition.m_sProperty = "m_metaFlags";
			definition.m_sWho = string.Format(GDC_MMCheckText.T("the meta flag \"%1\"", "le meta flag « %1 »"), name);
			BaseContainer term = metaFlag.GetObject("m_definition");
			if (!term)
			{
				m_Report.Add("5.6", framework, "m_metaFlags", definition.m_sWho);
				continue;
			}

			ReadTerm(term, definition);
			conditions.Insert(definition);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReadRandomFlags(IEntitySource framework, notnull set<string> randomFlags)
	{
		BaseContainerList experiments = framework.GetObjectArray("m_randomFlags");
		for (int r = 0, count = CountOf(experiments); r < count; r++)
		{
			BaseContainer experiment = experiments.Get(r);
			if (!experiment)
				continue;

			string flag;
			if (experiment.Get("m_flagName", flag) && !flag.IsEmpty())
				randomFlags.Insert(flag);

			array<string> flags = {};
			if (experiment.Get("m_flagNames", flags))
			{
				foreach (string sampled : flags)
				{
					randomFlags.Insert(sampled);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Gathers the flags a condition reads, and reports an operator without operand (5.7)
	protected void ReadTerm(BaseContainer term, notnull GDC_MMCheckFlagReader condition)
	{
		string className = term.GetClassName();
		if (GDC_MMCheckContext.Inherits(className, LITERAL))
		{
			string flag;
			term.Get("m_flagName", flag);
			condition.m_aFlags.Insert(flag);
			return;
		}

		if (!GDC_MMCheckContext.Inherits(className, OPERATION))
			return;

		BaseContainerList operands = term.GetObjectArray("m_operands");
		if (!operands || operands.Count() == 0)
		{
			m_Report.Add("5.7", condition.m_Entity, condition.m_sProperty, condition.m_sWho, className);
			return;
		}

		for (int i = 0, count = operands.Count(); i < count; i++)
		{
			BaseContainer operand = operands.Get(i);
			if (operand)
				ReadTerm(operand, condition);
		}
	}
	//------------------------------------------------------------------------------------------------
	protected static int CountOf(BaseContainerList list)
	{
		if (!list)
			return 0;

		return list.Count();
	}
}

//------------------------------------------------------------------------------------------------
//! Something that waits for flags: a mission event condition, a meta flag definition, or the condition flag of an
//! interaction, a spawner, a GDC AI artillery component
class GDC_MMCheckFlagReader
{
	IEntitySource m_Entity; //!< where the finding points: the framework entity, or the entity holding the reader
	string m_sProperty;
	string m_sWho; //!< "the event "X"", "this entity"...
	string m_sEvent; //!< name of the mission event, empty for the other readers
	ref array<string> m_aFlags = {};
}

//------------------------------------------------------------------------------------------------
//! A mission event aimed at by name (Run, Reactivate, Deactivate Events)
class GDC_MMCheckEventReference
{
	IEntitySource m_Framework;
	string m_sWho; //!< the event holding the instruction
	string m_sName;
}
#endif
