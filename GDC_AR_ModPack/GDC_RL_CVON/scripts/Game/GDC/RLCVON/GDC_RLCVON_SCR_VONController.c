// Reforger Lobby spectators and VoN rooms in Coalition VON, through the hooks CVON provides for lobby addons
// (IsSpectatorTalker, HearOverChannel, ComputeSpectatorLR). CVON's own frame update runs untouched.
modded class SCR_VONController
{
	protected PS_GameModeCoop m_GDC_RLCVON_GameMode;

	//------------------------------------------------------------------------------------------------
	// Spectators talk over their Reforger Lobby VoN room: their gains come from ComputeSpectatorLR, not their position
	override bool IsSpectatorTalker(int playerId)
	{
		if (super.IsSpectatorTalker(playerId))
			return true;

		return GDC_RLCVON_IsSpectating(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// A spectator hears the other spectators of their room at any distance
	override bool HearOverChannel(int playerId)
	{
		if (super.HearOverChannel(playerId))
			return true;

		if (!GDC_RLCVON_IsSpectating(SCR_PlayerController.GetLocalPlayerId()))
			return false;

		return GDC_RLCVON_IsSpectating(playerId) && GDC_RLCVON_IsInOurRoom(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Full volume in both ears for the same room, silence otherwise (players muted in the game are silent too)
	override void ComputeSpectatorLR(int playerId, out float outLeft, out float outRight, out int silencedDecibels = 0)
	{
		super.ComputeSpectatorLR(playerId, outLeft, outRight, silencedDecibels);

		float gain = 0;
		if (GDC_RLCVON_IsInOurRoom(playerId) && !GDC_RLCVON_IsMuted(playerId))
			gain = 1;

		outLeft = gain;
		outRight = gain;
		silencedDecibels = 0;
	}

	//------------------------------------------------------------------------------------------------
	// Outside the GAME state (lobby, briefing, debriefing) everyone is a spectator; in game, a player without faction is
	protected bool GDC_RLCVON_IsSpectating(int playerId)
	{
		if (playerId < 1)
			return false;

		if (!m_GDC_RLCVON_GameMode)
			m_GDC_RLCVON_GameMode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!m_GDC_RLCVON_GameMode)
			return false;

		if (m_GDC_RLCVON_GameMode.GetState() != SCR_EGameModeState.GAME)
			return true;

		return !SCR_FactionManager.SGetPlayerFaction(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected bool GDC_RLCVON_IsInOurRoom(int playerId)
	{
		PS_VoNRoomsManager rooms = PS_VoNRoomsManager.GetInstance();
		if (!rooms)
			return false;

		return rooms.GetPlayerRoom(playerId) == rooms.GetPlayerRoom(SCR_PlayerController.GetLocalPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected bool GDC_RLCVON_IsMuted(int playerId)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return false;

		SocialComponent social = SocialComponent.Cast(pc.FindComponent(SocialComponent));
		if (!social)
			return false;

		return social.IsMuted(playerId);
	}
}
