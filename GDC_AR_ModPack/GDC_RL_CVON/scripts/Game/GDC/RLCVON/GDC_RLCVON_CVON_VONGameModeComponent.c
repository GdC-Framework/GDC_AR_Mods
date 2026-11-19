// PS_GameModeCoop.OnPlayerConnected doesn't call super, so the game mode components never get OnPlayerConnected
// and CVON wouldn't register the player (voice range, TeamSpeak client id). The game mode's event still fires:
// register from there, once per player, whether or not a later Reforger Lobby calls super again.
modded class CVON_VONGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnPlayerConnected().Insert(GDC_RLCVON_OnPlayerConnected);
	}

	//------------------------------------------------------------------------------------------------
	protected void GDC_RLCVON_OnPlayerConnected(int playerId)
	{
		if (m_aPlayerIds.Contains(playerId))
			return;

		OnPlayerConnected(playerId);
	}
}
