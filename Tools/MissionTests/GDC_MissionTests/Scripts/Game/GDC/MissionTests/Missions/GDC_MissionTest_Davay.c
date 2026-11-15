// GDC_MissionTest_Davay.c
// Path in mod: Scripts/Game/GDC/MissionTests/Missions/GDC_MissionTest_Davay.c

//! A group spawned by the reinforcement events, followed from its spawn to its waypoint.
class GDC_DavayTrackedGroup
{
	SCR_AIGroup m_Group;
	string m_sWaypoint;
	bool m_bWave;
	float m_fSpawnTime;
	vector m_vTarget;
	float m_fReachRadius;
	float m_fStartDistance = -1;
	float m_fBestDistance = 1000000;
	float m_fReachTime = -1;
	vector m_vLastMovePos;
	float m_fLastMoveTime;
	bool m_bStalled;
	vector m_vStallPos;
	int m_iMaxMembers;
	bool m_bDestroyed;
}

//! A placed group whose movement is watched: patrols must move, defenders must stay.
class GDC_DavayWatchedGroup
{
	string m_sName;
	bool m_bPatrol;
	bool m_bStarted;
	vector m_vStart;
	float m_fMaxDisplacement;
}

//! Automated test of the mission "Davay, duy na Gruwice" (worlds/tanin69/_En cours/Davai_duy_na_Gruwice).
//! Needs two clients: the first one takes the PL slot (Granit), the second one the SL slot of Berkut.
//! Both are made invulnerable. The insertion axis is Bleu.
class GDC_MissionTest_Davay : GDC_MissionTest
{
	protected const string ENEMY_FACTION = "RHS_ION";
	protected const int EXPECTED_AI_AT_START = 78;
	protected const int EXPECTED_AXIS_AI = 16;
	protected const int EXPECTED_AI_LIMIT = 256;
	protected const float SAMPLE_PERIOD = 5;

	//! Trigger TRG_RUS_HERE: 300 s of presence in the hotel. Margin for the teleport fade and the trigger tick.
	protected const float TRIGGER_DELAY_MIN = 295;
	protected const float TRIGGER_DELAY_MAX = 340;

	//! Event "Stop Reinf" clears GO_REINF after 1100 s.
	protected const float STOP_DELAY = 1100;

	//! End of the test, after GO_REINF: leaves the last wave (915 s) ten minutes to walk to Lenin Place.
	protected const float END_DELAY = 1500;

	protected const float NEAR = 15;

	protected int m_iPlId;
	protected int m_iSlId;
	protected IEntity m_PlCharacter;
	protected IEntity m_SlCharacter;
	protected IEntity m_RiflemanCharacter;

	protected int m_iAIAtStart;
	protected int m_iMaxAI;
	protected float m_fGameStart;
	protected float m_fHotelTime;
	protected float m_fReinforcementTime = -1;
	protected bool m_bSampling;
	protected bool m_bDefendersChecked;
	protected bool m_bSlotsApplied;

	protected ref array<ref GDC_DavayTrackedGroup> m_aTracked = {};
	protected ref array<ref GDC_DavayWatchedGroup> m_aWatched = {};

	//! Window shooters: members and their altitude when the game starts.
	protected ref array<IEntity> m_aShooters = {};
	protected ref array<float> m_aShooterHeights = {};

	protected IEntity m_Intel;

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "Davay, duy na Gruwice";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForPlayers(); return;
			case 2: CheckLobby(); return;
			case 3: TakeSlots(); return;
			case 4: AdvanceLobby(); NextStep(8); return;
			case 5: AdvanceLobby(); NextStep(3); return;
			case 6: WaitForControl(); return;
			case 7: CheckGameStart(); return;
			case 8: CheckGathered(); return;
			case 9: CheckRankRefused(); return;
			case 10: CheckInsertion(); return;
			case 11: CheckAxisOpposition(); return;
			case 12: CheckFireSupport(); return;
			case 13: CheckFireSupportConsumed(); return;
			case 14: WaitForReinforcements(); return;
			case 15: CheckStopReinforcements(); return;
			case 16: FinalChecks(); return;
		}
	}

	// ----- Lobby ----------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void WaitForPlayers()
	{
		if (GetPlayerCount() >= 2 && GetGameState() != SCR_EGameModeState.NULL)
		{
			m_iPlId = GetPlayerId(0);
			m_iSlId = GetPlayerId(1);
			Info(string.Format("Players connected: PL = %1, SL = %2", m_iPlId, m_iSlId));
			// Leaves time to the pregame spawners (vehicle crews) and to the clients' lobby menus
			NextStep(15);
			return;
		}

		if (StepElapsed() > 600)
		{
			Check("Two players connected", false, string.Format("%1 after 600 s", GetPlayerCount()));
			Finish();
			return;
		}

		RepeatStep(2);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckLobby()
	{
		Info("Game state " + typename.EnumToString(SCR_EGameModeState, GetGameState()));
		CheckNamedEntities();
		CheckTeam();
		CheckEnemiesAtStart();
		CheckModules();

		if (GetGameState() == SCR_EGameModeState.PREGAME)
			AdvanceLobby();

		NextStep(3);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckNamedEntities()
	{
		array<string> names = {
			"TFM", "TRG_RUS_HERE", "Spawn_1", "Spawn_2", "Spawn_3", "WP_SAD1", "Wp_Def1", "Wp_Def2", "Opp_Est_Def_WP",
			"Join_bleu", "Join_rouge", "Join_vert", "Insert_1", "Insert_2", "Insert_3",
			"Trg_Arrivee_Rouge", "Trg_Arrivee_Bleu", "Trg_Arrivee_Vert", "NoFireArea",
			"aGranit", "bBerkut", "cTaiga", "PR3_Emplacement", "PR3_grp",
			"ZU_Ouest", "ZU_Nord", "Pat_Nord", "Pat_Ouest"
		};
		CheckEntitiesExist(names);

		array<string> flags = {"GO_REINF", "Arrivee_Rouge", "Arrivee_Bleu", "Arrivee_Vert"};
		array<string> setFlags = {};
		foreach (string flag : flags)
		{
			if (IsFlagSet(flag))
				setFlags.Insert(flag);
		}

		Check("No mission flag set before the game", setFlags.IsEmpty(), JoinStrings(setFlags));
	}

	//------------------------------------------------------------------------------------------------
	//! Slots, ranks, loadouts and radio presets of the 15 players' characters.
	protected void CheckTeam()
	{
		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		CheckInt("Playable slots", manager.GetPlayables().Count(), 15);

		CheckGroup("aGranit", "PL,Medic,RTO", "31000");
		CheckGroup("bBerkut", "SL,MG,AMG,GL,LAT,Rifleman", "32000");
		CheckGroup("cTaiga", "SL,MG,AMG,GL,AT,AAT", "33000");
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckGroup(string groupName, string expectedRoles, string shortRangeFrequency)
	{
		array<IEntity> members = {};
		GetGroupMembers(SCR_AIGroup.Cast(Find(groupName)), members);

		array<string> roles = {};
		foreach (IEntity member : members)
		{
			string role = GetRole(member);
			roles.Insert(role);
			CheckMember(groupName, role, member, shortRangeFrequency);
		}

		array<string> expected = {};
		expectedRoles.Split(",", expected, true);
		array<string> sortedRoles = {};
		sortedRoles.Copy(roles);
		sortedRoles.Sort();
		expected.Sort();
		Check(groupName + " roles", JoinStrings(sortedRoles) == JoinStrings(expected), JoinStrings(roles));
	}

	//------------------------------------------------------------------------------------------------
	//! \return Role read from the GDC prefab name, e.g. "PL" for Character_RHS_RF_MSV_VKPO_DS_PL_GDC.et
	protected string GetRole(IEntity character)
	{
		string prefab = GetPrefabName(character);
		int start = prefab.IndexOf("VKPO_DS_");
		int end = prefab.IndexOf("_GDC.et");
		if (start < 0 || end < 0)
			return "?" + prefab;

		start += 8;
		return prefab.Substring(start, end - start);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckMember(string groupName, string role, IEntity member, string shortRangeFrequency)
	{
		string label = groupName + " " + role;
		array<IEntity> items = {};
		GetInventoryItems(member, items);

		CheckInt(label + " R-148", CountItems(items, "Radio_R148"), 1);
		bool hasLongRange = role == "PL" || role == "RTO" || role == "SL";
		if (hasLongRange)
			CheckInt(label + " R-168", CountItems(items, "Radio_R168"), 1);
		else
			CheckInt(label + " no R-168", CountItems(items, "Radio_R168"), 0);

		if (role == "PL")
		{
			if (!m_PlCharacter)
				m_PlCharacter = member;

			CheckInt(label + " Orion", CountItems(items, "Orion"), 1);
			int magazines = CountItems(items, "Magazine_545x39");
			Check(label + " at least 5 AK magazines", magazines >= 5, string.Format("%1", magazines));
			Check(label + " rank CAPTAIN", SCR_CharacterRankComponent.GetCharacterRank(member) == SCR_ECharacterRank.CAPTAIN, SCR_CharacterRankComponent.GetCharacterRankName(member));
		}
		else if (role == "SL")
		{
			if (groupName == "bBerkut")
				m_SlCharacter = member;

			Check(label + " rank LIEUTENANT", SCR_CharacterRankComponent.GetCharacterRank(member) == SCR_ECharacterRank.LIEUTENANT, SCR_CharacterRankComponent.GetCharacterRankName(member));
		}
		else if (role == "AMG")
		{
			int boxes = CountItems(items, "Box_762x54_PK");
			Check(label + " at least 3 PK boxes", boxes >= 3, string.Format("%1", boxes));
		}
		else if (role == "Medic")
		{
			CheckInt(label + " 6Sh118 backpack", CountItems(items, "Backpack_Ratnik_6sh118"), 1);
			IEntity kit = FindItem(items, "MedicalKit_01_USSR");
			int contents = CountItemContents(kit);
			Check(label + " medical kit filled", kit && contents > 0, string.Format("kit %1, %2 items inside", kit != null, contents));
		}
		else if (role == "Rifleman" && groupName == "bBerkut")
		{
			m_RiflemanCharacter = member;
		}

		// Per-slot radio presets of GDC AR Lib, stored on the character when the group spawns it
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(member);
		string shortRange;
		string longRange;
		if (character && !character.m_aGDC_SlotShortRangeFrequencies.IsEmpty())
			shortRange = character.m_aGDC_SlotShortRangeFrequencies[0];

		if (character && !character.m_aGDC_SlotLongRangeFrequencies.IsEmpty())
			longRange = character.m_aGDC_SlotLongRangeFrequencies[0];

		Check(label + " slot frequencies", shortRange == shortRangeFrequency && longRange == "48000", string.Format("CP %1, LP %2", shortRange, longRange));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckEnemiesAtStart()
	{
		array<IEntity> enemies = {};
		m_iAIAtStart = GetAICharacters(ENEMY_FACTION, enemies);
		m_iMaxAI = m_iAIAtStart;
		CheckInt("Enemy AI at start", m_iAIAtStart, EXPECTED_AI_AT_START);

		AIWorld aiWorld = GetGame().GetAIWorld();
		CheckInt("Active AI limit", aiWorld.GetLimitOfActiveAIs(), EXPECTED_AI_LIMIT);

		array<string> vehicles = {"ZU_Ouest", "ZU_Nord", "Pat_Nord", "Pat_Ouest"};
		foreach (string vehicleName : vehicles)
		{
			int seats;
			int occupied = GetTurretOccupancy(Find(vehicleName), seats);
			Check(vehicleName + " turrets manned", seats > 0 && occupied == seats, string.Format("%1 of %2", occupied, seats));
		}

		// Placed AI must not be on the axes before the players arrive
		array<string> axisGroups = {"Opp_Ouest_Def_grp", "Opp_Ouest_Pat_grp", "Opp_Nord_Def_grp", "Opp_Est_Def_grp"};
		array<string> present = {};
		foreach (string groupName : axisGroups)
		{
			if (Find(groupName))
				present.Insert(groupName);
		}

		Check("No axis opposition before the arrival", present.IsEmpty(), JoinStrings(present));

		m_Intel = FindNear(Vector(10174.8, 21, 5425.2), 3, "IntelligenceFolder");
		Check("Intel on the hotel ground floor", m_Intel != null);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckModules()
	{
		array<GDC_SupportModuleComponent> modules = GDC_SupportModuleComponent.GetModules();
		CheckInt("Support modules", modules.Count(), 1);
		if (!modules.IsEmpty())
			CheckInt("Fire missions at start", modules[0].GetMissionsLeft(), 5);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity m_FoundEntity;
	protected string m_sSearchedPrefab;

	protected IEntity FindNear(vector position, float radius, string prefabPart)
	{
		m_FoundEntity = null;
		m_sSearchedPrefab = prefabPart;
		GetGame().GetWorld().QueryEntitiesBySphere(position, radius, OnQueryEntity, null, EQueryEntitiesFlags.ALL);
		return m_FoundEntity;
	}

	protected bool OnQueryEntity(IEntity entity)
	{
		if (!GetPrefabName(entity).Contains(m_sSearchedPrefab))
			return true;

		m_FoundEntity = entity;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void TakeSlots()
	{
		Check("PL slot taken by the first player", AssignSlot(m_iPlId, m_PlCharacter));
		Check("Berkut SL slot taken by the second player", AssignSlot(m_iSlId, m_SlCharacter));
		Info("Game state " + typename.EnumToString(SCR_EGameModeState, GetGameState()));
		NextStep(5);
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForControl()
	{
		bool plReady = GetPlayerEntity(m_iPlId) == m_PlCharacter;
		bool slReady = GetPlayerEntity(m_iSlId) == m_SlCharacter;
		if (plReady && slReady)
		{
			Check("Game state GAME", GetGameState() == SCR_EGameModeState.GAME, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Check("Players control their slot", true, string.Format("%1 s", Math.Round(StepElapsed())));
			SetInvulnerable(m_PlCharacter);
			SetInvulnerable(m_SlCharacter);
			m_fGameStart = Now();
			StartSampling();
			NextStep(20);
			return;
		}

		// The client asks for its slot when it opens the game menu; ask for it on its behalf if it doesn't
		if (StepElapsed() > 20 && !m_bSlotsApplied)
		{
			m_bSlotsApplied = true;
			PS_PlayableManager.GetInstance().ApplyPlayable(m_iPlId);
			PS_PlayableManager.GetInstance().ApplyPlayable(m_iSlId);
			Warn("Slots applied by the server, the clients did not take them");
		}

		if (StepElapsed() > 60)
		{
			Check("Players control their slot", false, string.Format("PL %1, SL %2, state %3", plReady, slReady, typename.EnumToString(SCR_EGameModeState, GetGameState())));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	// ----- Game start and insertion ---------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void CheckGameStart()
	{
		CheckRadio("PL", m_iPlId, CVON_ERadioType.SHORT, "31000");
		CheckRadio("PL", m_iPlId, CVON_ERadioType.LONG, "48000");
		CheckRadio("Berkut SL", m_iSlId, CVON_ERadioType.SHORT, "32000");
		CheckRadio("Berkut SL", m_iSlId, CVON_ERadioType.LONG, "48000");

		RecordShooters();

		// Both players gather next to the Bleu flag, within the 10 m of the module
		GDC_TeleportModuleComponent module = FindTeleportModule("Join_bleu");
		vector flag = module.GetOwner().GetOrigin();
		MovePlayer(m_iPlId, module, GroundPosition(flag[0] + 2, flag[2]));
		MovePlayer(m_iSlId, module, GroundPosition(flag[0] - 2, flag[2]));
		NextStep(12);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckRadio(string label, int playerId, CVON_ERadioType radioType, string frequency)
	{
		CVON_RadioComponent radio = GetPlayerRadio(playerId, radioType);
		string tuned = "no radio";
		if (radio)
			tuned = radio.m_sFrequency;

		Check(string.Format("%1 %2 radio tuned", label, typename.EnumToString(CVON_ERadioType, radioType)), tuned == frequency, string.Format("%1, expected %2", tuned, frequency));
	}

	//------------------------------------------------------------------------------------------------
	protected void RecordShooters()
	{
		array<string> groupNames = {"Fen1_grp", "Fen2_grp", "Fen3_grp", "Fen4_grp", "Fen5_grp", "Fen6_grp", "Fen7_grp", "Fen8_grp", "Fen9_grp", "Fen1_grp2", "Fen1_grp3", "Fen1_grp4"};
		foreach (string groupName : groupNames)
		{
			array<IEntity> members = {};
			GetGroupMembers(SCR_AIGroup.Cast(Find(groupName)), members);
			foreach (IEntity member : members)
			{
				m_aShooters.Insert(member);
				m_aShooterHeights.Insert(member.GetOrigin()[1]);
			}
		}

		Info(string.Format("%1 window shooters recorded", m_aShooters.Count()));
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsNear(IEntity entity, vector position, float distance)
	{
		return entity && vector.DistanceXZ(entity.GetOrigin(), position) <= distance;
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckGathered()
	{
		vector flag = Find("Join_bleu").GetOrigin();
		Check("Players gathered at the Bleu flag", IsNear(m_PlCharacter, flag, 10) && IsNear(m_SlCharacter, flag, 10), string.Format("PL at %1, SL at %2", Pos(m_PlCharacter.GetOrigin()), Pos(m_SlCharacter.GetOrigin())));

		GDC_TeleportModuleComponent module = FindTeleportModule("Join_bleu");
		Check("Insertion allowed to the PL (CAPTAIN)", module.IsAllowed(m_iPlId));
		Check("Insertion refused to the SL (LIEUTENANT)", !module.IsAllowed(m_iSlId));
		module.Request(m_iSlId);
		NextStep(12);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckRankRefused()
	{
		vector flag = Find("Join_bleu").GetOrigin();
		Check("Server refuses the insertion to the SL", IsNear(m_SlCharacter, flag, 10), Pos(m_SlCharacter.GetOrigin()));

		FindTeleportModule("Join_bleu").Request(m_iPlId);
		NextStep(15);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckInsertion()
	{
		vector insert = Find("Insert_2").GetOrigin();
		if (StepElapsed() < 1)
		{
			Check("PL inserted on Bleu", IsNear(m_PlCharacter, insert, NEAR), Pos(m_PlCharacter.GetOrigin()));
			Check("SL inserted with the PL", IsNear(m_SlCharacter, insert, NEAR), Pos(m_SlCharacter.GetOrigin()));
		}

		if (IsFlagSet("Arrivee_Bleu"))
		{
			Check("Flag Arrivee_Bleu raised", true, string.Format("%1 s after the insertion", Math.Round(StepElapsed())));
			Check("Other arrival flags not raised", !IsFlagSet("Arrivee_Rouge") && !IsFlagSet("Arrivee_Vert"));
			NextStep(20);
			return;
		}

		if (StepElapsed() > 30)
		{
			Check("Flag Arrivee_Bleu raised", false, "after 30 s");
			NextStep(1);
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAxisOpposition()
	{
		CheckGroupSize("Opp_Ouest_Def_grp", 8);
		CheckGroupSize("Opp_Ouest_Pat_grp", 4);
		CheckGroupSize("Opp_Ouest_Pat2_grp", 4);

		array<string> others = {"Opp_Nord_Def_grp", "Opp_Nord_Pat_grp", "Opp_Nord_Pat2_grp", "Opp_Est_Def_grp", "Opp_Est_Pat_grp", "Opp_Est_Pat2_grp"};
		array<string> present = {};
		foreach (string groupName : others)
		{
			if (Find(groupName))
				present.Insert(groupName);
		}

		Check("Opposition of the other axes not spawned", present.IsEmpty(), JoinStrings(present));

		array<IEntity> enemies = {};
		CheckInt("Enemy AI after the arrival", GetAICharacters(ENEMY_FACTION, enemies), m_iAIAtStart + EXPECTED_AXIS_AI);

		// GetInNearest of PR3_grp: the M2HB of the emplacement must be manned by now
		int gunners;
		array<IEntity> all = {};
		GetAICharacters(ENEMY_FACTION, all);
		vector emplacement = Find("PR3_Emplacement").GetOrigin();
		foreach (IEntity enemy : all)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(enemy);
			if (character.IsInVehicle() && vector.Distance(character.GetOrigin(), emplacement) < 8)
				gunners++;
		}

		Check("PR3 M2HB emplacement manned", gunners > 0, string.Format("%1 seated", gunners));
		NextStep(2);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckGroupSize(string groupName, int expected)
	{
		array<IEntity> members = {};
		int count = GetGroupMembers(SCR_AIGroup.Cast(Find(groupName)), members);
		Check(groupName + " spawned", count == expected, string.Format("%1 members, expected %2", count, expected));
	}

	// ----- Fire support ---------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void CheckFireSupport()
	{
		GDC_SupportModuleComponent module = GDC_SupportModuleComponent.GetModules()[0];
		CheckRefusal("Mortars accessible to the PL (R-168)", module.CheckAccess(m_PlCharacter), GDC_ESupportRefusal.NONE);
		CheckRefusal("Mortars accessible to the SL (R-168)", module.CheckAccess(m_SlCharacter), GDC_ESupportRefusal.NONE);
		CheckRefusal("Mortars refused to a rifleman (no R-168)", module.CheckAccess(m_RiflemanCharacter), GDC_ESupportRefusal.MISSING_ITEM);

		vector leninPlace = GroundPosition(10200, 5440);
		Check("Lenin Place is a no-fire area", !module.IsTargetAllowed(leninPlace));
		CheckRefusal("Server refuses a target on Lenin Place", module.Request(m_iPlId, leninPlace, 0, 3, 30), GDC_ESupportRefusal.TARGET_FORBIDDEN);
		CheckInt("Refused request not consumed", module.GetMissionsLeft(), 5);

		// A field north of the town, far from the players and from the AI
		CheckRefusal("Fire mission accepted outside the town", module.Request(m_iPlId, GroundPosition(10700, 6600), 0, 3, 30), GDC_ESupportRefusal.NONE);
		NextStep(3);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckRefusal(string testName, GDC_ESupportRefusal refusal, GDC_ESupportRefusal expected)
	{
		Check(testName, refusal == expected, string.Format("%1, expected %2", typename.EnumToString(GDC_ESupportRefusal, refusal), typename.EnumToString(GDC_ESupportRefusal, expected)));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckFireSupportConsumed()
	{
		GDC_SupportModuleComponent module = GDC_SupportModuleComponent.GetModules()[0];
		CheckInt("Accepted request consumes a mission", module.GetMissionsLeft(), 4);
		GDC_ESupportRefusal refusal = module.Request(m_iPlId, GroundPosition(10700, 6600), 0, 3, 30);
		Check("Second request refused while the mortars are busy", refusal != GDC_ESupportRefusal.NONE, typename.EnumToString(GDC_ESupportRefusal, refusal));
		CheckInt("Busy refusal not consumed", module.GetMissionsLeft(), 4);

		CheckDefenders();

		// The PL goes to the hotel ground floor, next to the intel: trigger TRG_RUS_HERE starts counting
		MovePlayer(m_iPlId, FindTeleportModule("Join_bleu"), Vector(10176, 21.1, 5426));
		m_fHotelTime = Now();
		NextStep(12);
	}

	// ----- Reinforcements -------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void WaitForReinforcements()
	{
		if (StepElapsed() < 1)
			Check("PL in the hotel", vector.Distance(m_PlCharacter.GetOrigin(), Find("TRG_RUS_HERE").GetOrigin()) <= 25, Pos(m_PlCharacter.GetOrigin()));

		if (IsFlagSet("GO_REINF"))
		{
			m_fReinforcementTime = Now();
			float delay = m_fReinforcementTime - m_fHotelTime;
			Check("GO_REINF after 300 s in the hotel", delay >= TRIGGER_DELAY_MIN && delay <= TRIGGER_DELAY_MAX, string.Format("%1 s", Math.Round(delay)));
			m_Runner.ResetFpsStats();
			NextStep(STOP_DELAY + 10);
			return;
		}

		if (Now() - m_fHotelTime > 450)
		{
			Check("GO_REINF after 300 s in the hotel", false, "not raised after 450 s, PL at " + Pos(m_PlCharacter.GetOrigin()));
			FinalChecks();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckStopReinforcements()
	{
		Check("GO_REINF cleared by Stop Reinf", !IsFlagSet("GO_REINF"), string.Format("%1 s after GO_REINF", Math.Round(Now() - m_fReinforcementTime)));
		NextStep(END_DELAY - STOP_DELAY - 10);
	}

	// ----- Sampling -------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void StartSampling()
	{
		Watch("Pied_NE_grp", true);
		Watch("Pied_SO_grp", true);
		Watch("Pied_SE_grp", true);
		Watch("Pied_NO_grp", true);
		Watch("Pat_Center_grp", true);
		Watch("Def_Est_grp", false);
		Watch("Def_Nord_grp", false);
		Watch("Def_Sud_grp", false);
		Watch("Def_SE_grp", false);
		Watch("Def_Center_grp2", false);
		Watch("Def_Center_grp3", false);
		Watch("Def_Center_grp4", false);

		m_bSampling = true;
		Sample();
	}

	//------------------------------------------------------------------------------------------------
	protected void Watch(string groupName, bool patrol)
	{
		GDC_DavayWatchedGroup watched = new GDC_DavayWatchedGroup();
		watched.m_sName = groupName;
		watched.m_bPatrol = patrol;
		m_aWatched.Insert(watched);
	}

	//------------------------------------------------------------------------------------------------
	protected void Sample()
	{
		if (m_bDone || !m_bSampling)
			return;

		array<IEntity> enemies = {};
		int enemyCount = GetAICharacters(ENEMY_FACTION, enemies);
		if (enemyCount > m_iMaxAI)
			m_iMaxAI = enemyCount;

		foreach (GDC_DavayWatchedGroup watched : m_aWatched)
		{
			vector position;
			if (!GetGroupPosition(SCR_AIGroup.Cast(Find(watched.m_sName)), position))
				continue;

			if (!watched.m_bStarted)
			{
				watched.m_bStarted = true;
				watched.m_vStart = position;
			}

			watched.m_fMaxDisplacement = Math.Max(watched.m_fMaxDisplacement, vector.DistanceXZ(position, watched.m_vStart));
		}

		TrackReinforcements();
		GetGame().GetCallqueue().CallLater(Sample, SAMPLE_PERIOD * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void TrackReinforcements()
	{
		array<SCR_AIGroup> groups = {};
		GetAIGroups(groups);
		foreach (SCR_AIGroup group : groups)
		{
			if (!GetPrefabName(group).Contains("QuickReactionForce") || FindTracked(group))
				continue;

			AIWaypoint waypoint = group.GetCurrentWaypoint();
			GDC_DavayTrackedGroup tracked = new GDC_DavayTrackedGroup();
			tracked.m_Group = group;
			tracked.m_fSpawnTime = Now();
			tracked.m_fLastMoveTime = Now();
			if (waypoint)
			{
				tracked.m_sWaypoint = waypoint.GetName();
				tracked.m_vTarget = waypoint.GetOrigin();
			}

			tracked.m_bWave = tracked.m_sWaypoint == "WP_SAD1";
			if (tracked.m_bWave)
				tracked.m_fReachRadius = 60;
			else
				tracked.m_fReachRadius = 40;

			m_aTracked.Insert(tracked);
			Info(string.Format("QRF spawned %1 s after GO_REINF, waypoint '%2'", Math.Round(Now() - m_fReinforcementTime), tracked.m_sWaypoint));
		}

		foreach (GDC_DavayTrackedGroup tracked : m_aTracked)
		{
			UpdateTracked(tracked);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected GDC_DavayTrackedGroup FindTracked(SCR_AIGroup group)
	{
		foreach (GDC_DavayTrackedGroup tracked : m_aTracked)
		{
			if (tracked.m_Group == group)
				return tracked;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTracked(notnull GDC_DavayTrackedGroup tracked)
	{
		if (tracked.m_bDestroyed)
			return;

		array<IEntity> members = {};
		int count = GetGroupMembers(tracked.m_Group, members);
		if (count > tracked.m_iMaxMembers)
			tracked.m_iMaxMembers = count;

		vector position;
		if (!GetGroupPosition(tracked.m_Group, position))
		{
			// Members spawn one by one: a group is only destroyed once it has had members
			if (tracked.m_iMaxMembers > 0)
				tracked.m_bDestroyed = true;

			return;
		}

		float distance = vector.DistanceXZ(position, tracked.m_vTarget);
		if (tracked.m_fStartDistance < 0)
		{
			tracked.m_fStartDistance = distance;
			tracked.m_vLastMovePos = position;
		}

		tracked.m_fBestDistance = Math.Min(tracked.m_fBestDistance, distance);
		if (distance <= tracked.m_fReachRadius && tracked.m_fReachTime < 0)
			tracked.m_fReachTime = Now();

		if (vector.DistanceXZ(position, tracked.m_vLastMovePos) > 10)
		{
			tracked.m_vLastMovePos = position;
			tracked.m_fLastMoveTime = Now();
			return;
		}

		if (tracked.m_fReachTime < 0 && !tracked.m_bStalled && Now() - tracked.m_fLastMoveTime > 120)
		{
			tracked.m_bStalled = true;
			tracked.m_vStallPos = position;
			Warn(string.Format("QRF to '%1' stalled for 120 s at %2, %3 m from its waypoint", tracked.m_sWaypoint, Pos(position), Math.Round(distance)));
		}
	}

	// ----- Final checks ---------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Defenders are checked before the reinforcements, which may pull them into a fight.
	protected void CheckDefenders()
	{
		m_bDefendersChecked = true;
		foreach (GDC_DavayWatchedGroup watched : m_aWatched)
		{
			if (watched.m_bPatrol)
				continue;

			if (watched.m_fMaxDisplacement > 50)
				Warn(string.Format("Defenders %1 moved %2 m from their position", watched.m_sName, Math.Round(watched.m_fMaxDisplacement)));
			else
				Info(string.Format("Defenders %1 held their position (%2 m)", watched.m_sName, Math.Round(watched.m_fMaxDisplacement)));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void FinalChecks()
	{
		m_bSampling = false;
		TrackReinforcements();

		foreach (GDC_DavayWatchedGroup watched : m_aWatched)
		{
			if (watched.m_bPatrol)
				Check("Patrol " + watched.m_sName + " moves", watched.m_fMaxDisplacement >= 25, string.Format("%1 m at most", Math.Round(watched.m_fMaxDisplacement)));
		}

		array<string> vehicles = {"Pat_Nord", "Pat_Ouest"};
		foreach (string vehicleName : vehicles)
		{
			IEntity vehicle = Find(vehicleName);
			int seats;
			int occupied = GetTurretOccupancy(vehicle, seats);
			Info(string.Format("%1 at %2, turret %3 of %4 manned", vehicleName, Pos(vehicle.GetOrigin()), occupied, seats));
		}

		CheckShooters();

		if (m_fReinforcementTime >= 0)
			CheckReinforcements();

		Check("Intel still in the hotel", m_Intel != null);
		Check("Enemy AI stayed under the active AI limit", m_iMaxAI <= EXPECTED_AI_LIMIT, string.Format("maximum %1", m_iMaxAI));
		Info(string.Format("Reinforcement phase: %1", m_Runner.GetFpsReport()));
		Finish();
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckShooters()
	{
		int fallen;
		foreach (int i, IEntity shooter : m_aShooters)
		{
			if (!IsAlive(shooter))
				continue;

			float drop = m_aShooterHeights[i] - shooter.GetOrigin()[1];
			if (drop > 2.5)
			{
				fallen++;
				Warn(string.Format("Window shooter %1 m lower than at start, now at %2", Math.Round(drop), Pos(shooter.GetOrigin())));
			}
		}

		Info(string.Format("Window shooters: %1 recorded, %2 lower than at start", m_aShooters.Count(), fallen));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckReinforcements()
	{
		array<float> waveTimes = {};
		array<string> blocWaypoints = {};
		int late;
		foreach (GDC_DavayTrackedGroup tracked : m_aTracked)
		{
			float spawned = tracked.m_fSpawnTime - m_fReinforcementTime;
			if (spawned > STOP_DELAY)
				late++;

			if (tracked.m_bWave)
				waveTimes.Insert(spawned);
			else
				blocWaypoints.Insert(tracked.m_sWaypoint);

			ReportTracked(tracked, spawned);
		}

		CheckInt("Reinforcement groups (4 waves of 3)", waveTimes.Count(), 12);
		blocWaypoints.Sort();
		Check("Bloc groups on their 3 waypoints", JoinStrings(blocWaypoints) == "Opp_Est_Def_WP, Wp_Def1, Wp_Def2", JoinStrings(blocWaypoints));
		CheckInt("No QRF after Stop Reinf", late, 0);

		// Spawned at T + 5, 10, 15 s, then every 300 s; a sample comes up to SAMPLE_PERIOD later
		array<float> expected = {5, 10, 15, 305, 310, 315, 605, 610, 615, 905, 910, 915};
		waveTimes.Sort();
		array<string> offTime = {};
		foreach (int i, float time : waveTimes)
		{
			if (!expected.IsIndexValid(i) || time < expected[i] - 2 || time > expected[i] + SAMPLE_PERIOD + 7)
				offTime.Insert(string.Format("%1 s", Math.Round(time)));
		}

		Check("Waves on schedule (T+5/10/15 s, every 300 s)", offTime.IsEmpty(), "off schedule: " + JoinStrings(offTime));
	}

	//------------------------------------------------------------------------------------------------
	protected void ReportTracked(notnull GDC_DavayTrackedGroup tracked, float spawned)
	{
		string label = string.Format("QRF T+%1 s to '%2'", Math.Round(spawned), tracked.m_sWaypoint);
		if (tracked.m_fReachTime >= 0)
		{
			Check(label + " reached its waypoint", true, string.Format("in %1 s, from %2 m", Math.Round(tracked.m_fReachTime - tracked.m_fSpawnTime), Math.Round(tracked.m_fStartDistance)));
			return;
		}

		if (tracked.m_bDestroyed)
		{
			Info(label + " destroyed before reaching its waypoint");
			return;
		}

		string detail = string.Format("closest %1 m, start %2 m", Math.Round(tracked.m_fBestDistance), Math.Round(tracked.m_fStartDistance));
		if (tracked.m_bStalled)
			detail += ", stalled at " + Pos(tracked.m_vStallPos);

		Check(label + " reached its waypoint", false, detail);
	}
}
