// GDC_TeleportTestEntity.c
// Path in mod: Scripts/Game/GDC/Teleport/GDC_TeleportTestEntity.c

#ifdef DEBUG
//! Automated test of the teleport modules, for the world _Tests/GDC_Teleport_AutoTest. Diag builds only.
//! The first player runs the scenario through the same network path as real requests, the second one checks
//! that the group teleport takes them along and the individual one does not, the dedicated server checks that it
//! draws nothing. Every machine logs PASS or FAIL lines.
[EntityEditorProps(category: "GDC/Logic", description: "DIAG. Automated test of the teleport modules, logs PASS or FAIL lines.")]
class GDC_TeleportTestEntityClass : GenericEntityClass
{
}

class GDC_TeleportTestEntity : GenericEntity
{
	//! A traveller lands within a few metres of the destination: arrival slots are 2 m apart.
	protected const float ARRIVAL_TOLERANCE = 10;

	//! Fade out, black and fade in of the modules of the test world last 2 s in all.
	protected const float TRAVEL_WAIT = 4;

	protected const int WATCH_TIMEOUT = 120;

	protected int m_iStep;
	protected int m_iPassed;
	protected int m_iFailed;
	protected int m_iWatchSeconds;
	protected bool m_bWatcherTravelled;
	protected SCR_PlayerController m_Controller;

	//------------------------------------------------------------------------------------------------
	protected GDC_TeleportModuleComponent FindModule(string entityName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			return null;

		return GDC_TeleportModuleComponent.Cast(entity.FindComponent(GDC_TeleportModuleComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetEntityOrigin(string entityName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			return vector.Zero;

		return entity.GetOrigin();
	}

	//------------------------------------------------------------------------------------------------
	//! \return Distance on the ground between the local character and a named entity, -1 if one of them is missing
	protected float DistanceTo(string entityName)
	{
		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!character || !entity)
			return -1;

		return vector.DistanceXZ(character.GetOrigin(), entity.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(string testName, bool passed, string detail = string.Empty)
	{
		if (passed)
		{
			m_iPassed++;
			Print("[GDC_TeleportTest] PASS " + testName);
			return;
		}

		m_iFailed++;
		Print("[GDC_TeleportTest] FAIL " + testName + " " + detail, LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckNear(string testName, string entityName)
	{
		float distance = DistanceTo(entityName);
		Check(testName, distance >= 0 && distance <= ARRIVAL_TOLERANCE, string.Format("%1 m from %2", distance, entityName));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckFar(string testName, string entityName)
	{
		float distance = DistanceTo(entityName);
		Check(testName, distance > ARRIVAL_TOLERANCE, string.Format("%1 m from %2", distance, entityName));
	}

	//------------------------------------------------------------------------------------------------
	//! Checks valid on every client: the line is drawn where it is wanted, and only there.
	protected void TestZones()
	{
		Check("Line drawn for ALL_PLAYERS", FindModule("T_All").IsZoneDrawn());
		Check("No line for INDIVIDUAL", !FindModule("T_Individual").IsZoneDrawn());
		Check("No line when not asked", !FindModule("T_RankHigh").IsZoneDrawn());
	}

	//------------------------------------------------------------------------------------------------
	protected void NextStep(float delay)
	{
		GetGame().GetCallqueue().CallLater(RunStep, delay * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Scenario of the first player.
	protected void RunStep()
	{
		m_iStep++;
		switch (m_iStep)
		{
			case 1:
				TestZones();
				Check("ALL_PLAYERS without rank is allowed", FindModule("T_All").IsAllowed(m_Controller.GetPlayerId()));
				Check("Rank too low is not allowed", !FindModule("T_RankHigh").IsAllowed(m_Controller.GetPlayerId()));
				m_Controller.GDC_RequestTeleport(FindModule("T_RankHigh"));
				NextStep(TRAVEL_WAIT);
				return;

			case 2:
				CheckNear("Server refuses a rank too low", "T_Spawn");
				m_Controller.GDC_RequestTeleport(FindModule("T_Far"));
				NextStep(TRAVEL_WAIT);
				return;

			case 3:
				CheckNear("Server refuses a module too far away", "T_Spawn");
				m_Controller.GDC_RequestTeleport(FindModule("T_All"));
				NextStep(TRAVEL_WAIT);
				return;

			case 4:
				CheckNear("ALL_PLAYERS takes the requester", "Dest_All");
				TestClock();
				m_Controller.GDC_RequestTeleport(FindModule("T_Individual"));
				NextStep(TRAVEL_WAIT);
				return;

			case 5:
				CheckNear("INDIVIDUAL takes the requester", "Dest_Ind");
				break;
		}

		Print(string.Format("[GDC_TeleportTest] DONE passed %1, failed %2", m_iPassed, m_iFailed));
	}

	//------------------------------------------------------------------------------------------------
	//! T_All sets the clock to 22:25 at its first use, on the server, replicated to the clients.
	protected void TestClock()
	{
		ChimeraWorld world = GetGame().GetWorld();
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		float hours = manager.GetTimeOfTheDay();
		Check("Clock changed by the first use", hours >= 22.4 && hours < 23, string.Format("%1 h", hours));
	}

	//------------------------------------------------------------------------------------------------
	//! Second player: must travel with T_All, then stay behind when the first player uses T_Individual.
	protected void Watch()
	{
		m_iWatchSeconds++;

		if (!m_bWatcherTravelled)
		{
			float distance = DistanceTo("Dest_All");
			if (distance >= 0 && distance <= ARRIVAL_TOLERANCE)
			{
				m_bWatcherTravelled = true;
				Check("ALL_PLAYERS takes a player gathered around the module", true);
				m_iWatchSeconds = 0;
			}
			else if (m_iWatchSeconds > WATCH_TIMEOUT)
			{
				Check("ALL_PLAYERS takes a player gathered around the module", false, string.Format("%1 m from Dest_All after %2 s", distance, WATCH_TIMEOUT));
				EndWatch();
				return;
			}

			GetGame().GetCallqueue().CallLater(Watch, 1000, false);
			return;
		}

		// The first player leaves with T_Individual TRAVEL_WAIT seconds after the group teleport.
		if (m_iWatchSeconds < TRAVEL_WAIT * 5)
		{
			if (DistanceTo("Dest_Ind") <= ARRIVAL_TOLERANCE)
			{
				Check("INDIVIDUAL leaves the others behind", false, "the second player reached Dest_Ind");
				EndWatch();
				return;
			}

			GetGame().GetCallqueue().CallLater(Watch, 1000, false);
			return;
		}

		Check("INDIVIDUAL leaves the others behind", true);
		CheckNear("Second player still at the group destination", "Dest_All");
		TestClock();
		EndWatch();
	}

	//------------------------------------------------------------------------------------------------
	protected void EndWatch()
	{
		Print(string.Format("[GDC_TeleportTest] WATCHER DONE passed %1, failed %2", m_iPassed, m_iFailed));
	}

	//------------------------------------------------------------------------------------------------
	//! The scenario needs both players spawned next to the modules.
	protected bool BothPlayersReady()
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		if (players.Count() < 2)
			return false;

		foreach (int playerId : players)
		{
			if (!GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Waits for the local player and for their character. The first player runs the scenario, the second one watches.
	protected void WaitForPlayer()
	{
		m_Controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_Controller || !m_Controller.GetControlledEntity() || m_Controller.GetPlayerId() == 0)
		{
			GetGame().GetCallqueue().CallLater(WaitForPlayer, 1000, false);
			return;
		}

		if (m_Controller.GetPlayerId() != 1)
		{
			Print(string.Format("[GDC_TeleportTest] START watcher, player %1", m_Controller.GetPlayerId()));
			TestZones();
			Watch();
			return;
		}

		if (!BothPlayersReady())
		{
			GetGame().GetCallqueue().CallLater(WaitForPlayer, 1000, false);
			return;
		}

		Print(string.Format("[GDC_TeleportTest] START player %1", m_Controller.GetPlayerId()));
		// Leaves time to the second player to settle next to the modules.
		NextStep(5);
	}

	//------------------------------------------------------------------------------------------------
	//! Dedicated server: the line is a visual aid for the players only.
	protected void TestServer()
	{
		Check("Server draws no line", !FindModule("T_All").IsZoneDrawn());
		Print(string.Format("[GDC_TeleportTest] SERVER DONE passed %1, failed %2", m_iPassed, m_iFailed));
	}

	//------------------------------------------------------------------------------------------------
	void GDC_TeleportTestEntity(IEntitySource src, IEntity parent)
	{
		if (!GetGame().InPlayMode())
			return;

		if (System.IsConsoleApp())
		{
			GetGame().GetCallqueue().CallLater(TestServer, 10000, false);
			return;
		}

		GetGame().GetCallqueue().CallLater(WaitForPlayer, 5000, false);
	}

	//------------------------------------------------------------------------------------------------
	void ~GDC_TeleportTestEntity()
	{
		if (!GetGame() || !GetGame().GetCallqueue())
			return;

		GetGame().GetCallqueue().Remove(WaitForPlayer);
		GetGame().GetCallqueue().Remove(RunStep);
		GetGame().GetCallqueue().Remove(Watch);
		GetGame().GetCallqueue().Remove(TestServer);
	}
}
#endif
