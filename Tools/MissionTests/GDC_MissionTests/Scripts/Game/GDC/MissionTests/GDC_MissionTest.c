// GDC_MissionTest.c
// Path in mod: Scripts/Game/GDC/MissionTests/GDC_MissionTest.c

//! Base of the automated mission tests. Runs on the dedicated server only, started by the command line
//! parameter -gdcTest <Mission> (see GDC_SCR_BaseGameMode_MissionTest.c). Every check prints a line
//! "[GDC_MissionTest] PASS|FAIL|WARN|INFO ..." that the launcher script collects at the end.
//! A scenario derives from this class and implements RunStep(): one step per call, each step schedules
//! the next one with NextStep() or waits for a condition with RepeatStep().
class GDC_MissionTest
{
	protected const string TAG = "[GDC_MissionTest] ";

	protected int m_iPassed;
	protected int m_iFailed;
	protected int m_iWarnings;

	protected int m_iStep;
	protected bool m_bRepeat;
	protected float m_fStepStart;
	protected bool m_bDone;

	protected GDC_MissionTestRunnerEntity m_Runner;

	//------------------------------------------------------------------------------------------------
	//! Name printed in the logs.
	string GetName()
	{
		return "Mission";
	}

	//------------------------------------------------------------------------------------------------
	void Start(GDC_MissionTestRunnerEntity runner)
	{
		m_Runner = runner;
		Info(string.Format("START %1", GetName()));
		NextStep(1);
	}

	//------------------------------------------------------------------------------------------------
	//! Scenario steps, numbered from 1.
	protected void RunStep(int step);

	// ----- Step scheduling ------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void NextStep(float delay)
	{
		GetGame().GetCallqueue().CallLater(DoStep, delay * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Runs the current step again after a delay, to wait for a condition.
	protected void RepeatStep(float delay)
	{
		m_bRepeat = true;
		NextStep(delay);
	}

	//------------------------------------------------------------------------------------------------
	protected void DoStep()
	{
		if (m_bDone)
			return;

		if (!m_bRepeat)
		{
			m_iStep++;
			m_fStepStart = Now();
		}

		m_bRepeat = false;
		RunStep(m_iStep);
	}

	//------------------------------------------------------------------------------------------------
	//! \return Seconds spent in the current step, repeats included
	protected float StepElapsed()
	{
		return Now() - m_fStepStart;
	}

	//------------------------------------------------------------------------------------------------
	//! \return World time in seconds
	static float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000;
	}

	// ----- Results --------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected bool Check(string testName, bool passed, string detail = string.Empty)
	{
		if (passed)
		{
			m_iPassed++;
			if (detail.IsEmpty())
				Print(TAG + "PASS " + testName);
			else
				Print(TAG + "PASS " + testName + " (" + detail + ")");

			return true;
		}

		m_iFailed++;
		Print(TAG + "FAIL " + testName + " " + detail, LogLevel.ERROR);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckInt(string testName, int value, int expected)
	{
		return Check(testName, value == expected, string.Format("%1, expected %2", value, expected));
	}

	//------------------------------------------------------------------------------------------------
	protected void Warn(string message)
	{
		m_iWarnings++;
		Print(TAG + "WARN " + message, LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	protected void Info(string message)
	{
		Print(TAG + "INFO " + message);
	}

	//------------------------------------------------------------------------------------------------
	protected void Finish()
	{
		m_bDone = true;
		// Client parts have no runner entity
		if (m_Runner)
			Info(m_Runner.GetFpsReport());
		Print(string.Format(TAG + "DONE passed %1, failed %2, warnings %3", m_iPassed, m_iFailed, m_iWarnings));
	}

	// ----- World helpers --------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static IEntity Find(string name)
	{
		return GetGame().GetWorld().FindEntityByName(name);
	}

	//------------------------------------------------------------------------------------------------
	//! Checks that all the named entities exist.
	protected void CheckEntitiesExist(notnull array<string> names)
	{
		array<string> missing = {};
		foreach (string name : names)
		{
			if (!Find(name))
				missing.Insert(name);
		}

		Check(string.Format("%1 named entities exist", names.Count()), missing.IsEmpty(), "missing: " + JoinStrings(missing));
	}

	//------------------------------------------------------------------------------------------------
	static string JoinStrings(notnull array<string> values)
	{
		string result;
		foreach (int i, string value : values)
		{
			if (i > 0)
				result += ", ";

			result += value;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	static string Pos(vector position)
	{
		return string.Format("%1 / %2 (alt %3)", Math.Round(position[0]), Math.Round(position[2]), Math.Round(position[1]));
	}

	//------------------------------------------------------------------------------------------------
	static vector GroundPosition(float x, float z)
	{
		return Vector(x, GetGame().GetWorld().GetSurfaceY(x, z), z);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsAlive(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		return controller && !controller.IsDead();
	}

	//------------------------------------------------------------------------------------------------
	static FactionKey GetFactionKey(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return string.Empty;

		Faction faction = character.GetFaction();
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	//! \return Prefab path of an entity, empty if it has none
	static string GetPrefabName(IEntity entity)
	{
		if (!entity)
			return string.Empty;

		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
			return string.Empty;

		return prefabData.GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	//! \return Living AI characters of a faction, players excluded
	static int GetAICharacters(FactionKey factionKey, notnull array<IEntity> outCharacters)
	{
		outCharacters.Clear();
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return 0;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (AIGroup.Cast(agent))
				continue;

			IEntity entity = agent.GetControlledEntity();
			if (!IsAlive(entity) || EntityUtils.IsPlayer(entity))
				continue;

			if (GetFactionKey(entity) == factionKey && !outCharacters.Contains(entity))
				outCharacters.Insert(entity);
		}

		return outCharacters.Count();
	}

	//------------------------------------------------------------------------------------------------
	static void GetAIGroups(notnull array<SCR_AIGroup> outGroups)
	{
		outGroups.Clear();
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(agent);
			if (group)
				outGroups.Insert(group);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return Living members of a group
	static int GetGroupMembers(AIGroup group, notnull array<IEntity> outMembers)
	{
		outMembers.Clear();
		if (!group)
			return 0;

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (IsAlive(entity))
				outMembers.Insert(entity);
		}

		return outMembers.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! \return Position of the leader of a group, or of its first living member; false if nobody is left
	static bool GetGroupPosition(AIGroup group, out vector position)
	{
		if (!group)
			return false;

		IEntity leader = group.GetLeaderEntity();
		if (IsAlive(leader))
		{
			position = leader.GetOrigin();
			return true;
		}

		array<IEntity> members = {};
		if (GetGroupMembers(group, members) == 0)
			return false;

		position = members[0].GetOrigin();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Group of a character, null if it has none
	static SCR_AIGroup GetCharacterGroup(IEntity character)
	{
		if (!character)
			return null;

		AIControlComponent control = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		if (!control || !control.GetControlAIAgent())
			return null;

		return SCR_AIGroup.Cast(control.GetControlAIAgent().GetParentGroup());
	}

	//------------------------------------------------------------------------------------------------
	//! \return Number of occupied turret seats and number of turret seats of a vehicle
	static int GetTurretOccupancy(IEntity vehicle, out int turretSeats)
	{
		turretSeats = 0;
		if (!vehicle)
			return 0;

		BaseCompartmentManagerComponent manager = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!manager)
			return 0;

		int occupied;
		array<BaseCompartmentSlot> slots = {};
		manager.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (slot.GetType() != ECompartmentType.TURRET)
				continue;

			turretSeats++;
			if (IsAlive(slot.GetOccupant()))
				occupied++;
		}

		return occupied;
	}

	// ----- Inventory helpers ----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static void GetInventoryItems(IEntity character, notnull array<IEntity> outItems)
	{
		outItems.Clear();
		if (!character)
			return;

		InventoryStorageManagerComponent storage = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (storage)
			storage.GetItems(outItems);
	}

	//------------------------------------------------------------------------------------------------
	//! \return Number of items whose prefab path contains the given text
	static int CountItems(notnull array<IEntity> items, string prefabPart)
	{
		int count;
		foreach (IEntity item : items)
		{
			if (GetPrefabName(item).Contains(prefabPart))
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	static IEntity FindItem(notnull array<IEntity> items, string prefabPart)
	{
		foreach (IEntity item : items)
		{
			if (GetPrefabName(item).Contains(prefabPart))
				return item;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Number of items stored inside an item (kit, pouch...)
	static int CountItemContents(IEntity item)
	{
		if (!item)
			return 0;

		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent));
		if (!storage)
			return 0;

		array<IEntity> contents = {};
		return storage.GetAll(contents);
	}

	// ----- Players and Reforger Lobby -------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static int GetPlayerCount()
	{
		array<int> players = {};
		return GetGame().GetPlayerManager().GetPlayers(players);
	}

	//------------------------------------------------------------------------------------------------
	static int GetPlayerId(int index)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		if (!players.IsIndexValid(index))
			return 0;

		players.Sort();
		return players[index];
	}

	//------------------------------------------------------------------------------------------------
	static IEntity GetPlayerEntity(int playerId)
	{
		return GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! \return RL slot of a character, null if it is not playable
	static PS_PlayableContainer FindPlayable(IEntity character)
	{
		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		if (!manager || !character)
			return null;

		map<RplId, ref PS_PlayableContainer> playables = manager.GetPlayables();
		foreach (RplId id, PS_PlayableContainer container : playables)
		{
			if (container && container.GetPlayableComponent() && container.GetPlayableComponent().GetOwner() == character)
				return container;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Puts a player in a RL slot, as the slot menu does. Server only.
	static bool AssignSlot(int playerId, IEntity character)
	{
		PS_PlayableContainer container = FindPlayable(character);
		if (!container)
			return false;

		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		manager.SetPlayerFactionKey(playerId, container.GetFactionKey());
		manager.SetPlayerPlayable(playerId, container.GetRplId());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Moves the RL lobby to its next state, as the admin button does.
	static void AdvanceLobby()
	{
		PS_GameModeCoop gameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.AdvanceGameState(SCR_EGameModeState.NULL);
	}

	//------------------------------------------------------------------------------------------------
	static SCR_EGameModeState GetGameState()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return SCR_EGameModeState.NULL;

		return gameMode.GetState();
	}

	//------------------------------------------------------------------------------------------------
	//! The test players must survive the whole scenario. Server side.
	static void SetInvulnerable(IEntity character)
	{
		SCR_ChimeraCharacter chimera = SCR_ChimeraCharacter.Cast(character);
		if (!chimera)
			return;

		DamageManagerComponent damageManager = chimera.GetDamageManager();
		if (damageManager)
			damageManager.EnableDamageHandling(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Moves a player with the GDC teleport sequence: the owner of a character is the only one who can move it.
	static void MovePlayer(int playerId, notnull GDC_TeleportModuleComponent module, vector position)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (controller)
			controller.GDC_SendTeleport(module, position);
	}

	//------------------------------------------------------------------------------------------------
	static GDC_TeleportModuleComponent FindTeleportModule(string entityName)
	{
		IEntity entity = Find(entityName);
		if (!entity)
			return null;

		return GDC_TeleportModuleComponent.Cast(entity.FindComponent(GDC_TeleportModuleComponent));
	}

	//------------------------------------------------------------------------------------------------
	static bool IsFlagSet(string flagName)
	{
		TILW_MissionFrameworkEntity framework = TILW_MissionFrameworkEntity.GetInstance();
		return framework && framework.IsMissionFlag(flagName);
	}

	//------------------------------------------------------------------------------------------------
	//! \return First radio of a type carried by a player, null if none
	static CVON_RadioComponent GetPlayerRadio(int playerId, CVON_ERadioType radioType)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!controller)
			return null;

		foreach (IEntity radio : controller.m_aRadios)
		{
			if (!radio)
				continue;

			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
			if (radioComp && radioComp.m_eRadioType == radioType)
				return radioComp;
		}

		return null;
	}
}
