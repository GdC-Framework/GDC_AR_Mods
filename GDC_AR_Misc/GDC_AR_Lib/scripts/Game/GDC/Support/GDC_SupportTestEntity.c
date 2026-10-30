// GDC_SupportTestEntity.c
// Path in mod: Scripts/Game/GDC/Support/GDC_SupportTestEntity.c

#ifdef DEBUG
//! Automated test of the support modules, for the world _Tests/GDC_Support_AutoTest. Diag builds only.
//! Runs on the machine of the first player, through the same network path as real requests, and logs PASS or FAIL lines.
[EntityEditorProps(category: "GDC/Logic", description: "DIAG. Automated test of the support modules, logs PASS or FAIL lines.")]
class GDC_SupportTestEntityClass : GenericEntityClass
{
}

class GDC_SupportTestEntity : GenericEntity
{
	protected const vector TARGET_IN_ZONE = "1086 39 2453";
	protected const vector TARGET_OUT_OF_ZONE = "1500 39 2300";

	// Distances from T_Sequence, at 1100 39 2400.
	protected const vector TARGET_IN_RANGE = "1100 39 2700";
	protected const vector TARGET_NEAR_MAX_RANGE = "1100 39 5300";
	protected const vector TARGET_TOO_FAR = "1100 39 5900";
	protected const vector TARGET_TOO_CLOSE = "1105 39 2400";

	protected int m_iStep;
	protected int m_iPassed;
	protected int m_iFailed;
	protected SCR_PlayerController m_Controller;
	protected IEntity m_Character;
	protected GDC_SupportModuleComponent m_Sequence;

	//------------------------------------------------------------------------------------------------
	protected GDC_SupportModuleComponent FindModule(string entityName)
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			return null;

		return GDC_SupportModuleComponent.Cast(entity.FindComponent(GDC_SupportModuleComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(string testName, bool passed, string detail = string.Empty)
	{
		if (passed)
		{
			m_iPassed++;
			Print("[GDC_SupportTest] PASS " + testName);
			return;
		}

		m_iFailed++;
		Print("[GDC_SupportTest] FAIL " + testName + " " + detail, LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckInt(string testName, int actual, int expected)
	{
		Check(testName, actual == expected, string.Format("expected %1, got %2", expected, actual));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckRefusal(string testName, GDC_ESupportRefusal actual, GDC_ESupportRefusal expected)
	{
		Check(testName, actual == expected, string.Format("expected %1, got %2", typename.EnumToString(GDC_ESupportRefusal, expected), typename.EnumToString(GDC_ESupportRefusal, actual)));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckGrid(string text, vector expected)
	{
		vector position;
		bool isValid = GDC_SupportMapPanel.ParseGrid(text, position);
		Check("Grid parse '" + text + "'", isValid && vector.Distance(position, expected) < 0.5, string.Format("valid %1, position %2", isValid, position));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckGridInvalid(string text)
	{
		vector position;
		Check("Grid rejects '" + text + "'", !GDC_SupportMapPanel.ParseGrid(text, position));
	}

	//------------------------------------------------------------------------------------------------
	protected void TestGrid()
	{
		CheckGrid("123 456", "12350 0 45650");
		CheckGrid("123456", "12350 0 45650");
		CheckGrid("0108 0245", "1085 0 2455");
		CheckGrid("01086 02453", "1086 0 2453");
		CheckGridInvalid("");
		CheckGridInvalid("12345");
		CheckGridInvalid("12 34");
		CheckGridInvalid("12a 456");
		CheckGridInvalid("1234567");
		Check("Grid format pads with zeros", GDC_SupportMapPanel.FormatGrid("1086 0 2453") == "0108 0245", GDC_SupportMapPanel.FormatGrid("1086 0 2453"));
	}

	//------------------------------------------------------------------------------------------------
	protected void TestLimits()
	{
		GDC_SupportModuleComponent limits = FindModule("T_Limits");
		CheckInt("Max rounds capped at 50", limits.GetMaxQuantity(), 50);
		CheckInt("Min dispersion raised to 5", limits.GetMinSpread(), 5);
		CheckInt("Max dispersion capped at 500", limits.GetMaxSpread(), 500);

		GDC_SupportModuleComponent inverted = FindModule("T_Inverted");
		CheckInt("Inverted dispersion: min", inverted.GetMinSpread(), 40);
		CheckInt("Inverted dispersion: max", inverted.GetMaxSpread(), 300);

		array<string> options = {};
		m_Sequence.GetOptionNames(options);
		CheckInt("Ammunition list from the prefab", options.Count(), 2);
		Check("Default mode is arcade", m_Sequence.GetTargetingMode() == GDC_ESupportTargetingMode.ARCADE);
		Check("Milsim mode is read", FindModule("T_Milsim").GetTargetingMode() == GDC_ESupportTargetingMode.MILSIM);
	}

	//------------------------------------------------------------------------------------------------
	protected void TestAccess()
	{
		CheckRefusal("Rank too low", FindModule("T_RankHigh").CheckAccess(m_Character), GDC_ESupportRefusal.RANK_TOO_LOW);
		CheckRefusal("Rank high enough", FindModule("T_RankLow").CheckAccess(m_Character), GDC_ESupportRefusal.NONE);
		CheckRefusal("Item missing", FindModule("T_ItemMissing").CheckAccess(m_Character), GDC_ESupportRefusal.MISSING_ITEM);
		CheckRefusal("Rank OK and item missing", FindModule("T_RankAndItem").CheckAccess(m_Character), GDC_ESupportRefusal.MISSING_ITEM);
		CheckRefusal("No character", m_Sequence.CheckAccess(null), GDC_ESupportRefusal.INVALID_REQUEST);
	}

	//------------------------------------------------------------------------------------------------
	protected void TestZones()
	{
		GDC_SupportModuleComponent whitelist = FindModule("T_Whitelist");
		Check("Whitelist allows inside", whitelist.IsTargetAllowed(TARGET_IN_ZONE));
		Check("Whitelist refuses outside", !whitelist.IsTargetAllowed(TARGET_OUT_OF_ZONE));

		GDC_SupportModuleComponent blacklist = FindModule("T_Blacklist");
		Check("Blacklist refuses inside", !blacklist.IsTargetAllowed(TARGET_IN_ZONE));
		Check("Blacklist allows outside", blacklist.IsTargetAllowed(TARGET_OUT_OF_ZONE));

		Check("No zone allows anything", m_Sequence.IsTargetAllowed(TARGET_OUT_OF_ZONE));
	}

	//------------------------------------------------------------------------------------------------
	//! Expected values come from the vanilla ballistic tables of the M821 and M819 shells.
	protected void TestRange()
	{
		GDC_FireSupportModuleComponent fire = GDC_FireSupportModuleComponent.Cast(m_Sequence);
		array<ref GDC_FireSupportAmmo> ammo = fire.GetAmmo();

		float maxRange = ammo[0].GetMaxRange();
		Check("HE max range", maxRange > 2940 && maxRange < 2980, string.Format("%1 m", maxRange));
		maxRange = ammo[1].GetMaxRange();
		Check("Smoke max range", maxRange > 2440 && maxRange < 2480, string.Format("%1 m", maxRange));

		float minRange = ammo[0].GetMinRange();
		Check("HE min range", minRange > 60 && minRange < 90, string.Format("%1 m", minRange));
		minRange = ammo[1].GetMinRange();
		Check("Smoke min range", minRange > 115 && minRange < 150, string.Format("%1 m", minRange));

		Check("In range", fire.IsTargetInRange(TARGET_IN_RANGE, 0));
		Check("Too far", !fire.IsTargetInRange(TARGET_TOO_FAR, 0));
		Check("Too close", !fire.IsTargetInRange(TARGET_TOO_CLOSE, 0));
		Check("Smoke range differs from HE", fire.IsTargetInRange(TARGET_NEAR_MAX_RANGE, 0) && !fire.IsTargetInRange(TARGET_NEAR_MAX_RANGE, 1));

		float flightTime = fire.GetFlightTime(TARGET_IN_RANGE, 0);
		Check("Flight time at 300 m", flightTime > 8 && flightTime < 15, string.Format("%1 s", flightTime));
		flightTime = fire.GetFlightTime(TARGET_NEAR_MAX_RANGE, 0);
		Check("Flight time near max range", flightTime > 24 && flightTime < 32, string.Format("%1 s", flightTime));
	}

	//------------------------------------------------------------------------------------------------
	//! \return The marker of a support among those this player can see, null if there is none
	protected SCR_MapMarkerBase FindMarker(string supportName)
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		array<SCR_MapMarkerBase> markers = markerManager.GetStaticMarkers();
		markers.InsertAll(markerManager.GetDisabledMarkers());
		foreach (SCR_MapMarkerBase marker : markers)
		{
			if (marker.GetCustomText() == supportName)
				return marker;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! T_Limits has a marker for all players. T_Inverted has one for the US, and this player is from the USSR.
	protected void TestMarkers()
	{
		SCR_MapMarkerBase marker = FindMarker("T_Limits");
		Check("Marker received by the client", marker != null);
		if (marker)
		{
			Check("Marker is a military one", marker.GetType() == SCR_EMapMarkerType.PLACED_MILITARY);
			CheckInt("Marker is a mortar", marker.GetFlags(), EMilitarySymbolIcon.MORTAR);

			int position[2];
			marker.GetWorldPos(position);
			Check("Marker at the position of the module", position[0] == 1105 && position[1] == 2400, string.Format("%1 %2", position[0], position[1]));
		}

		Check("No marker for the other faction", FindMarker("T_Inverted") == null);
		Check("No marker by default", FindMarker("T_Sequence") == null);
	}

	//------------------------------------------------------------------------------------------------
	//! Sends a request as a real client would, without any local check: the server has to refuse it.
	protected void SendRequest(int option, int quantity, int spread, vector target = TARGET_IN_RANGE, GDC_SupportModuleComponent module = null)
	{
		if (!module)
			module = m_Sequence;

		m_Controller.GDC_RequestSupport(module, target, option, quantity, spread);
	}

	//------------------------------------------------------------------------------------------------
	protected void NextStep(float delay)
	{
		GetGame().GetCallqueue().CallLater(RunStep, delay * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Scenario of GDC_SupportAutoTest: T_Sequence has 2 missions, 6 s of cooldown, 4 rounds at most, 0.5 s between rounds, 1 s before the first one.
	protected void RunStep()
	{
		m_iStep++;
		switch (m_iStep)
		{
			case 1:
				CheckInt("Initial missions, replicated", m_Sequence.GetMissionsLeft(), 2);
				CheckRefusal("Initially available", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.NONE);
				SendRequest(0, 999, 15);
				NextStep(1.5);
				return;

			case 2:
				CheckInt("Server refuses too many rounds", m_Sequence.GetMissionsLeft(), 2);
				SendRequest(0, 0, 15);
				NextStep(1.5);
				return;

			case 3:
				CheckInt("Server refuses 0 round", m_Sequence.GetMissionsLeft(), 2);
				SendRequest(0, 2, 9999);
				NextStep(1.5);
				return;

			case 4:
				CheckInt("Server refuses too much dispersion", m_Sequence.GetMissionsLeft(), 2);
				SendRequest(99, 2, 15);
				NextStep(1.5);
				return;

			case 5:
				CheckInt("Server refuses unknown ammunition", m_Sequence.GetMissionsLeft(), 2);
				SendRequest(0, 2, 15, TARGET_OUT_OF_ZONE, FindModule("T_Whitelist"));
				NextStep(1.5);
				return;

			case 6:
				CheckInt("Server refuses a target out of the whitelist", FindModule("T_Whitelist").GetMissionsLeft(), 5);
				SendRequest(0, 2, 15, TARGET_IN_ZONE, FindModule("T_RankHigh"));
				NextStep(1.5);
				return;

			case 7:
				CheckInt("Server refuses a rank too low", FindModule("T_RankHigh").GetMissionsLeft(), 5);
				SendRequest(0, 2, 15, TARGET_TOO_FAR);
				NextStep(1.5);
				return;

			case 8:
				CheckInt("Server refuses a target out of range", m_Sequence.GetMissionsLeft(), 2);
				SendRequest(0, 4, 15);
				NextStep(0.5);
				return;

			case 9:
				CheckInt("Accepted request consumes a mission", m_Sequence.GetMissionsLeft(), 1);
				CheckRefusal("Mission active", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.MISSION_ACTIVE);
				SendRequest(0, 4, 15);
				NextStep(1.5);
				return;

			case 10:
				CheckInt("Refused while active, not consumed", m_Sequence.GetMissionsLeft(), 1);
				// From now on, the mission ends about 2.7 s after the flight time, then the cooldown lasts 6 s.
				NextStep(GDC_FireSupportModuleComponent.Cast(m_Sequence).GetFlightTime(TARGET_IN_RANGE, 0) + 5);
				return;

			case 11:
				CheckRefusal("Cooldown after the mission", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.COOLDOWN);
				Check("Cooldown remaining replicated", m_Sequence.GetCooldownRemaining() > 0 && m_Sequence.GetCooldownRemaining() <= 6, string.Format("%1 s", m_Sequence.GetCooldownRemaining()));
				SendRequest(0, 4, 15);
				NextStep(1);
				return;

			case 12:
				CheckInt("Refused during cooldown, not consumed", m_Sequence.GetMissionsLeft(), 1);
				NextStep(6.5);
				return;

			case 13:
				CheckRefusal("Available after the cooldown", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.NONE);
				SendRequest(1, 1, 10);
				NextStep(0.5);
				return;

			case 14:
				CheckInt("Last mission consumed", m_Sequence.GetMissionsLeft(), 0);
				NextStep(8);
				return;

			case 15:
				CheckRefusal("No mission left", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.NO_MISSION_LEFT);
				SendRequest(0, 1, 10);
				NextStep(1.5);
				return;
		}

		CheckInt("Still no mission left", m_Sequence.GetMissionsLeft(), 0);
		Print(string.Format("[GDC_SupportTest] DONE passed %1, failed %2", m_iPassed, m_iFailed));
	}

	//------------------------------------------------------------------------------------------------
	//! Logs what a player who joined during the test sees of the replicated state.
	protected void WatchState()
	{
		Print(string.Format("[GDC_SupportTest] WATCH missions left %1, availability %2, cooldown %3 s", m_Sequence.GetMissionsLeft(), typename.EnumToString(GDC_ESupportRefusal, m_Sequence.CheckAvailability()), m_Sequence.GetCooldownRemaining()));
	}

	//------------------------------------------------------------------------------------------------
	//! Waits for the local player and for their character. Only the first player runs the test.
	protected void WaitForPlayer()
	{
		m_Controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (m_Controller)
			m_Character = m_Controller.GetControlledEntity();

		if (!m_Controller || !m_Character || m_Controller.GetPlayerId() == 0)
		{
			GetGame().GetCallqueue().CallLater(WaitForPlayer, 1000, false);
			return;
		}

		if (m_Controller.GetPlayerId() != 1)
		{
			Print(string.Format("[GDC_SupportTest] Player %1 only watches.", m_Controller.GetPlayerId()));
			m_Sequence = FindModule("T_Sequence");
			GetGame().GetCallqueue().CallLater(WatchState, 2000, true);
			return;
		}

		m_Sequence = FindModule("T_Sequence");
		Print(string.Format("[GDC_SupportTest] START player %1, authority over the module: %2", m_Controller.GetPlayerId(), m_Sequence.IsAuthority()));
		TestGrid();
		TestLimits();
		TestAccess();
		TestZones();
		TestRange();
		TestMarkers();
		NextStep(1);
	}

	//------------------------------------------------------------------------------------------------
	void GDC_SupportTestEntity(IEntitySource src, IEntity parent)
	{
		if (!GetGame().InPlayMode() || System.IsConsoleApp())
			return;

		GetGame().GetCallqueue().CallLater(WaitForPlayer, 5000, false);
	}

	//------------------------------------------------------------------------------------------------
	void ~GDC_SupportTestEntity()
	{
		if (!GetGame() || !GetGame().GetCallqueue())
			return;

		GetGame().GetCallqueue().Remove(WaitForPlayer);
		GetGame().GetCallqueue().Remove(RunStep);
		GetGame().GetCallqueue().Remove(WatchState);
	}
}
#endif
