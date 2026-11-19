// While Coalition VON runs, Reforger Lobby's own VoN must not transmit: CVON carries every voice, rooms included.
// Without CVON in the mission, Reforger Lobby keeps its VoN.
modded class PS_PlayableControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void LobbyVoNEnable()
	{
		if (CVON_VONGameModeComponent.GetInstance())
			return;

		super.LobbyVoNEnable();
	}

	//------------------------------------------------------------------------------------------------
	override void LobbyVoNFactionEnable()
	{
		if (CVON_VONGameModeComponent.GetInstance())
			return;

		super.LobbyVoNFactionEnable();
	}

	//------------------------------------------------------------------------------------------------
	override void LobbyVoNAdminEnable()
	{
		if (CVON_VONGameModeComponent.GetInstance())
			return;

		super.LobbyVoNAdminEnable();
	}

	//------------------------------------------------------------------------------------------------
	override void LobbyVoNDisable()
	{
		if (CVON_VONGameModeComponent.GetInstance())
			return;

		super.LobbyVoNDisable();
	}

	//------------------------------------------------------------------------------------------------
	override void LobbyVoNDisableDelayed()
	{
		if (CVON_VONGameModeComponent.GetInstance())
			return;

		super.LobbyVoNDisableDelayed();
	}
}
