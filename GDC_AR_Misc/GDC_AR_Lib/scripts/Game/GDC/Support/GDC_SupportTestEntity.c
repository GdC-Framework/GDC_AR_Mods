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
	//! Sends a request as a real client would, without any local check: the server has to refuse it.
	protected void SendRequest(int option, int quantity, int spread, vector target = TARGET_IN_ZONE, GDC_SupportModuleComponent module = null)
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
				SendRequest(0, 4, 15);
				NextStep(0.5);
				return;

			case 8:
				CheckInt("Accepted request consumes a mission", m_Sequence.GetMissionsLeft(), 1);
				CheckRefusal("Mission active", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.MISSION_ACTIVE);
				SendRequest(0, 4, 15);
				NextStep(1.5);
				return;

			case 9:
				CheckInt("Refused while active, not consumed", m_Sequence.GetMissionsLeft(), 1);
				NextStep(7.5);
				return;

			case 10:
				CheckRefusal("Cooldown after the mission", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.COOLDOWN);
				Check("Cooldown remaining replicated", m_Sequence.GetCooldownRemaining() > 0 && m_Sequence.GetCooldownRemaining() <= 6, string.Format("%1 s", m_Sequence.GetCooldownRemaining()));
				SendRequest(0, 4, 15);
				NextStep(1);
				return;

			case 11:
				CheckInt("Refused during cooldown, not consumed", m_Sequence.GetMissionsLeft(), 1);
				NextStep(6.5);
				return;

			case 12:
				CheckRefusal("Available after the cooldown", m_Sequence.CheckAvailability(), GDC_ESupportRefusal.NONE);
				SendRequest(1, 1, 10);
				NextStep(0.5);
				return;

			case 13:
				CheckInt("Last mission consumed", m_Sequence.GetMissionsLeft(), 0);
				NextStep(8);
				return;

			case 14:
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
