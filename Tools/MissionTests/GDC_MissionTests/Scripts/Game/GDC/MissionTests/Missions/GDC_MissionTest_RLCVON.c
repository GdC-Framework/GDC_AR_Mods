// GDC_MissionTest_RLCVON.c
// Path in mod: Scripts/Game/GDC/MissionTests/Missions/GDC_MissionTest_RLCVON.c

//! Test of the Reforger Lobby - Coalition VON bridge (GDC RL-CVON), from the preview to a death in game, on the Arland
//! test world. TeamSpeak isn't needed: the clients check what CVON would send to the plugin for the other player.
//! The server drives the lobby; each client waits for a phase, lets CVON update, then checks:
//! - preview, slot selection without slot, then with the first player in a slot. Taking a slot teleports the lobby
//!   entity far away and the other player is no longer streamed in: the former AFI bridge dropped his voice;
//! - lobby rooms: the second player alone in his local room is not heard, both in the global room are;
//! - Reforger Lobby's own voice keys don't start the vanilla VON while CVON runs;
//! - briefing: the other player is heard if and only if both are in the same Reforger Lobby room;
//! - game: both alive and close, the other player is a direct voice, not a lobby room voice;
//! - the second player is killed: the first one no longer hears him.
class GDC_MissionTest_RLCVON : GDC_MissionTest
{
	protected int m_iFirstId;
	protected int m_iSecondId;
	protected ref array<IEntity> m_aSoldiers = {};

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "RL-CVON bridge";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForPlayers(); return;
			case 2: AdvanceLobby(); NextStep(3); return;
			case 3: WaitForState(SCR_EGameModeState.SLOTSELECTION, 30); return;
			case 4: TakeSlot(m_iFirstId, 0); NextStep(40); return;
			case 5: MoveToRoom(m_iSecondId, "#PS-VoNRoom_Local" + m_iSecondId); NextStep(25); return;
			case 6: MoveToRoom(m_iFirstId, "#PS-VoNRoom_Global"); MoveToRoom(m_iSecondId, "#PS-VoNRoom_Global"); NextStep(25); return;
			case 7: TakeSlot(m_iSecondId, 1); AdvanceLobby(); NextStep(3); return;
			case 8: WaitForState(SCR_EGameModeState.BRIEFING, 30); return;
			case 9: AdvanceLobby(); NextStep(3); return;
			case 10: WaitForControl(); return;
			case 11: KillSecond(); NextStep(30); return;
			case 12: Check("Second player dead", !IsAlive(m_aSoldiers[1])); Finish(); return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The clients check the preview meanwhile
	protected void WaitForPlayers()
	{
		if (GetPlayerCount() >= 2 && GetGameState() != SCR_EGameModeState.NULL)
		{
			m_iFirstId = GetPlayerId(0);
			m_iSecondId = GetPlayerId(1);
			Info(string.Format("Players connected: first %1, second %2, state %3", m_iFirstId, m_iSecondId, typename.EnumToString(SCR_EGameModeState, GetGameState())));
			NextStep(25);
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
	//! Waits for a lobby state (a cutscene may come first), then leaves the clients time to check
	protected void WaitForState(SCR_EGameModeState state, float checkTime)
	{
		if (GetGameState() == state)
		{
			Info("State " + typename.EnumToString(SCR_EGameModeState, state));
			if (state == SCR_EGameModeState.BRIEFING)
				LogRooms();

			NextStep(checkTime);
			return;
		}

		if (StepElapsed() > 60)
		{
			Check("Lobby reaches " + typename.EnumToString(SCR_EGameModeState, state), false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void TakeSlot(int playerId, int index)
	{
		if (m_aSoldiers.IsEmpty())
		{
			PS_PlayableManager manager = PS_PlayableManager.GetInstance();
			map<RplId, ref PS_PlayableContainer> playables = manager.GetPlayables();
			foreach (RplId id, PS_PlayableContainer container : playables)
			{
				if (container && container.GetPlayableComponent())
					m_aSoldiers.Insert(container.GetPlayableComponent().GetOwner());
			}
		}

		if (!m_aSoldiers.IsIndexValid(index))
		{
			Check("Playable slot " + index + " exists", false, string.Format("%1 slots", m_aSoldiers.Count()));
			Finish();
			return;
		}

		Check(string.Format("Player %1 takes slot %2", playerId, index), AssignSlot(playerId, m_aSoldiers[index]), Pos(m_aSoldiers[index].GetOrigin()));
	}

	//------------------------------------------------------------------------------------------------
	protected void MoveToRoom(int playerId, string roomName)
	{
		PS_VoNRoomsManager.GetInstance().MoveToRoom(playerId, string.Empty, roomName);
		Info(string.Format("Player %1 moved to room %2", playerId, roomName));
	}

	//------------------------------------------------------------------------------------------------
	protected void LogRooms()
	{
		PS_VoNRoomsManager rooms = PS_VoNRoomsManager.GetInstance();
		array<int> playerIds = {m_iFirstId, m_iSecondId};
		foreach (int playerId : playerIds)
		{
			Info(string.Format("Player %1 in room '%2'", playerId, rooms.GetRoomName(rooms.GetPlayerRoom(playerId))));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForControl()
	{
		if (GetGameState() == SCR_EGameModeState.BRIEFING)
		{
			// A cutscene or a slow briefing: ask again
			if (StepElapsed() > 10 && StepElapsed() < 12)
				AdvanceLobby();
		}

		if (GetPlayerEntity(m_iFirstId) == m_aSoldiers[0] && GetPlayerEntity(m_iSecondId) == m_aSoldiers[1])
		{
			Check("Players control their slot", true, string.Format("%1 s", Math.Round(StepElapsed())));
			SetInvulnerable(m_aSoldiers[0]);
			// The clients check the direct voice meanwhile (their fade to game takes a few seconds)
			NextStep(45);
			return;
		}

		if (StepElapsed() > 90)
		{
			Check("Players control their slot", false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void KillSecond()
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_aSoldiers[1]);
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(character.GetDamageManager());
		damageManager.Kill(Instigator.CreateInstigator(null));
		Info("Second player killed");
	}
}

//! Client side: checks how this client hears the other player at each phase driven by the server.
class GDC_MissionTestClient_RLCVON : GDC_MissionTest
{
	protected const float SETTLE = 12; // CVON updates who it hears every few seconds

	protected int m_iFirstId;
	protected int m_iSecondId;
	protected int m_iOtherId;
	protected bool m_bFirst;
	protected string m_sRole;
	protected IEntity m_SecondCharacter;

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "RL-CVON bridge (client)";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForPreview(); return;
			case 2: CheckHeard("preview", true); NextStep(1); return;
			case 3: WaitUntil(GetGameState() == SCR_EGameModeState.SLOTSELECTION, "slot selection", SETTLE); return;
			case 4: CheckHeard("slot selection, no slot", true); NextStep(1); return;
			case 5: WaitUntil(HasSlot(m_iFirstId), "first player in a slot", SETTLE); return;
			case 6: CheckAfterSlot(); NextStep(1); return;
			case 7: WaitUntil(IsInRoom(m_iSecondId, "#PS-VoNRoom_Local" + m_iSecondId), "second player in his local room", SETTLE); return;
			case 8: CheckHeard("second player alone in his local room", false); NextStep(1); return;
			case 9: WaitUntil(IsInRoom(m_iFirstId, "#PS-VoNRoom_Global") && IsInRoom(m_iSecondId, "#PS-VoNRoom_Global"), "both players in the global room", SETTLE); return;
			case 10: CheckHeard("both in the global room", true); CheckLobbyVoiceKeys(); NextStep(1); return;
			case 11: WaitUntil(GetGameState() == SCR_EGameModeState.BRIEFING, "briefing", SETTLE); return;
			case 12: CheckBriefing(); NextStep(1); return;
			case 13: WaitUntil(ControlsSlot(), "game, both players in their slot", SETTLE); return;
			case 14: CheckGame(); NextStep(1); return;
			case 15: WaitUntil(m_SecondCharacter && !IsAlive(m_SecondCharacter), "second player dead", SETTLE); return;
			case 16: CheckAfterDeath(); Finish(); return;
		}
	}

	// ----- Phases ---------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void WaitForPreview()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (controller && controller.GetPlayerId() > 0 && controller.GetControlledEntity() && GetPlayerCount() >= 2 && GetGameState() == SCR_EGameModeState.PREGAME)
		{
			m_iFirstId = GetPlayerId(0);
			m_iSecondId = GetPlayerId(1);
			m_bFirst = controller.GetPlayerId() == m_iFirstId;
			if (m_bFirst)
			{
				m_iOtherId = m_iSecondId;
				m_sRole = "first player";
			}
			else
			{
				m_iOtherId = m_iFirstId;
				m_sRole = "second player";
			}

			Info(string.Format("Client of player %1 (%2), listens to player %3", controller.GetPlayerId(), m_sRole, m_iOtherId));
			NextStep(SETTLE);
			return;
		}

		if (StepElapsed() > 600)
		{
			Check("Preview reached with two players", false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	//! Waits for a condition seen on this client, then leaves CVON time to update
	protected void WaitUntil(bool condition, string phase, float settle)
	{
		if (condition)
		{
			Info(m_sRole + ": " + phase);
			NextStep(settle);
			return;
		}

		if (StepElapsed() > 300)
		{
			Check(m_sRole + ": phase reached: " + phase, false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAfterSlot()
	{
		IEntity other = GetPlayerEntity(m_iOtherId);
		string streamed = "no longer streamed in (the AFI bridge dropped such a voice)";
		if (other)
			streamed = "still streamed in: the cause of the AFI defect isn't reproduced";

		Info(m_sRole + ": other player's lobby entity " + streamed);
		CheckHeard("first player in a slot", true);
	}

	//------------------------------------------------------------------------------------------------
	//! Reforger Lobby's voice keys (direct, faction channel, admin) must not touch the vanilla VON while CVON runs
	protected void CheckLobbyVoiceKeys()
	{
		PS_PlayableControllerComponent playable = PS_PlayableControllerComponent.Cast(GetGame().GetPlayerController().FindComponent(PS_PlayableControllerComponent));
		PS_LobbyVoNComponent von;
		if (playable)
			von = playable.GetVoN();

		if (!Check(m_sRole + ": Reforger Lobby VoN component found", von != null))
			return;

		ECommMethod method = von.GetCommMethod();
		BaseTransceiver radio = von.GetTransmitRadio();
		playable.LobbyVoNEnable();
		playable.LobbyVoNFactionEnable();
		playable.LobbyVoNAdminEnable();
		bool unchanged = von.GetCommMethod() == method && von.GetTransmitRadio() == radio;
		playable.LobbyVoNDisable();
		Check(m_sRole + ": lobby voice keys leave the vanilla VON alone", unchanged, typename.EnumToString(ECommMethod, von.GetCommMethod()));
	}

	//------------------------------------------------------------------------------------------------
	//! Reforger Lobby puts each player in his group's or the command room: same room, heard; else not
	protected void CheckBriefing()
	{
		PS_VoNRoomsManager rooms = PS_VoNRoomsManager.GetInstance();
		int ownRoom = rooms.GetPlayerRoom(GetGame().GetPlayerController().GetPlayerId());
		int otherRoom = rooms.GetPlayerRoom(m_iOtherId);
		bool sameRoom = ownRoom == otherRoom;
		Info(string.Format("%1: briefing rooms, own '%2', other '%3'", m_sRole, rooms.GetRoomName(ownRoom), rooms.GetRoomName(otherRoom)));
		CheckHeard("briefing, same room " + sameRoom, sameRoom);
	}

	//------------------------------------------------------------------------------------------------
	protected bool ControlsSlot()
	{
		if (GetGameState() != SCR_EGameModeState.GAME)
			return false;

		// Slot soldiers are on the ground, close to each other; lobby entities are at 5000 m
		IEntity own = GetGame().GetPlayerController().GetControlledEntity();
		IEntity other = GetPlayerEntity(m_iOtherId);
		if (!IsAlive(own) || !IsAlive(other) || own.GetOrigin()[1] > 1000)
			return false;

		return vector.Distance(own.GetOrigin(), other.GetOrigin()) < 200;
	}

	//------------------------------------------------------------------------------------------------
	//! Both alive and close: a direct voice, not a lobby room voice
	protected void CheckGame()
	{
		IEntity own = GetGame().GetPlayerController().GetControlledEntity();
		IEntity other = GetPlayerEntity(m_iOtherId);
		if (m_bFirst)
			m_SecondCharacter = other;
		else
			m_SecondCharacter = own;

		Info(string.Format("%1: in game, other player at %2 m", m_sRole, Math.Round(vector.Distance(own.GetOrigin(), other.GetOrigin()))));

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		CVON_VONContainer entry = controller.m_aLocalEntries.Get(m_iOtherId);
		if (Check(m_sRole + ", game: other player in the CVON voices", entry != null))
			Check(m_sRole + ", game: other player heard as a direct voice", !entry.m_bIsSpectator && entry.m_eVonType == CVON_EVONType.DIRECT, string.Format("%1, spectator %2", typename.EnumToString(CVON_EVONType, entry.m_eVonType), entry.m_bIsSpectator));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAfterDeath()
	{
		if (m_bFirst)
		{
			CheckHeard("second player dead", false);
			return;
		}

		// While the listener's own character is dead, CVON sends the plugin no sound at all: just log the entry
		string detail;
		IsHeard(m_iOtherId, detail);
		Info(string.Format("%1, dead: entry for the first player: %2", m_sRole, detail));
	}

	// ----- Helpers --------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Whether CVON would play the other player to this client: a voice entry, and for a lobby room voice a gain
	protected bool IsHeard(int playerId, out string detail)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		CVON_VONContainer entry = controller.m_aLocalEntries.Get(playerId);
		if (!entry)
		{
			detail = "no CVON voice entry";
			return false;
		}

		if (!entry.m_bIsSpectator)
		{
			detail = typename.EnumToString(CVON_EVONType, entry.m_eVonType) + " voice";
			return true;
		}

		SCR_VONController von = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		float left;
		float right;
		int silenced;
		von.ComputeSpectatorLR(playerId, left, right, silenced);
		detail = string.Format("lobby room voice, left %1, right %2", left, right);
		return left > 0 || right > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckHeard(string phase, bool expected)
	{
		string detail;
		bool heard = IsHeard(m_iOtherId, detail);
		string expectation = "other player heard";
		if (!expected)
			expectation = "other player not heard";

		Check(m_sRole + ", " + phase + ": " + expectation, heard == expected, detail);
	}

	//------------------------------------------------------------------------------------------------
	static bool HasSlot(int playerId)
	{
		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		return manager && manager.GetPlayableByPlayer(playerId) != RplId.Invalid();
	}

	//------------------------------------------------------------------------------------------------
	static bool IsInRoom(int playerId, string roomName)
	{
		PS_VoNRoomsManager rooms = PS_VoNRoomsManager.GetInstance();
		if (!rooms)
			return false;

		int roomId = rooms.GetRoomWithFaction(string.Empty, roomName);
		return roomId >= 0 && rooms.GetPlayerRoom(playerId) == roomId;
	}
}
