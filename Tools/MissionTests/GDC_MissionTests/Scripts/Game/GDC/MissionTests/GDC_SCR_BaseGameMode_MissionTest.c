// GDC_SCR_BaseGameMode_MissionTest.c
// Path in mod: Scripts/Game/GDC/MissionTests/GDC_SCR_BaseGameMode_MissionTest.c

//! Starts an automated mission test on a dedicated server launched with -gdcTest <Mission>, and its client part,
//! if the scenario has one, on the clients launched with the same parameter.
//! This addon is only loaded by Tools/MissionTests/run_mission_test.ps1: it is never published.
modded class SCR_BaseGameMode
{
	protected GDC_MissionTestRunnerEntity m_GDC_TestRunner;
	protected ref GDC_MissionTest m_GDC_ClientTest;

	//------------------------------------------------------------------------------------------------
	override void OnGameStart()
	{
		super.OnGameStart();

		string missionName;
		if (!System.GetCLIParam("gdcTest", missionName) || missionName.IsEmpty())
			return;

		if (!System.IsConsoleApp())
		{
			if (!m_GDC_ClientTest)
			{
				m_GDC_ClientTest = GDC_CreateClientTest(missionName);
				if (m_GDC_ClientTest)
					m_GDC_ClientTest.Start(null);
			}

			return;
		}

		if (!Replication.IsServer() || m_GDC_TestRunner)
			return;

		GDC_MissionTest test = GDC_CreateMissionTest(missionName);
		if (!test)
		{
			Print("[GDC_MissionTest] FAIL Unknown mission test '" + missionName + "'", LogLevel.ERROR);
			Print("[GDC_MissionTest] DONE passed 0, failed 1, warnings 0");
			return;
		}

		m_GDC_TestRunner = GDC_MissionTestRunnerEntity.Cast(GetGame().SpawnEntity(GDC_MissionTestRunnerEntity, GetGame().GetWorld()));
		m_GDC_TestRunner.Run(test);
	}

	//------------------------------------------------------------------------------------------------
	//! Server part, one line per mission test: an explicit list rather than reflection.
	protected GDC_MissionTest GDC_CreateMissionTest(string missionName)
	{
		switch (missionName)
		{
			case "Davay":
				return new GDC_MissionTest_Davay();

			case "Uncon":
				return new GDC_MissionTest_Uncon();

			case "LobbyVoice":
				return new GDC_MissionTest_LobbyVoice();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Client part, for the scenarios that check what only a client can see. Null if the scenario has none.
	protected GDC_MissionTest GDC_CreateClientTest(string missionName)
	{
		switch (missionName)
		{
			case "Uncon":
				return new GDC_MissionTestClient_Uncon();

			case "LobbyVoice":
				return new GDC_MissionTestClient_LobbyVoice();
		}

		return null;
	}
}
