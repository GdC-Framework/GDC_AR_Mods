// GDC_MissionTest_LobbyVoice.c
// Path in mod: Scripts/Game/GDC/MissionTests/Missions/GDC_MissionTest_LobbyVoice.c

//! Test of the Reforger Lobby - Coalition VON bridge (GDC RL-CVON) in the slot selection, on the Arland test world.
//! Known defect of the former AFI bridge: once a player takes a slot, he no longer hears the others. Taking a slot
//! teleports his lobby entity to the slot, far from the others, which are then no longer streamed in, and the AFI
//! bridge dropped the voice of any player whose entity it could not see.
//! The server opens the slot selection, then puts the first player in a slot. The clients check, before and after,
//! that each one still hears the other over the lobby room, whether the other's entity is streamed in or not.
//! TeamSpeak isn't needed: the checks read what CVON would send to the plugin.
class GDC_MissionTest_LobbyVoice : GDC_MissionTest
{
	protected int m_iSlottedId;

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "Lobby voice (RL-CVON bridge)";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForPlayers(); return;
			case 2: AdvanceLobby(); NextStep(3); return;
			case 3: WaitForSlotSelection(); return;
			case 4: TakeSlot(); return;
			case 5: Finish(); return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForPlayers()
	{
		if (GetPlayerCount() >= 2 && GetGameState() != SCR_EGameModeState.NULL)
		{
			m_iSlottedId = GetPlayerId(0);
			Info(string.Format("Players connected: %1 will take a slot, %2 stays without slot", m_iSlottedId, GetPlayerId(1)));
			NextStep(5);
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
	//! Leaves the clients time to check the voices before anybody takes a slot
	protected void WaitForSlotSelection()
	{
		if (GetGameState() == SCR_EGameModeState.SLOTSELECTION)
		{
			Info("Slot selection open, nobody in a slot");
			NextStep(30);
			return;
		}

		if (StepElapsed() > 60)
		{
			Check("Lobby reaches the slot selection", false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	//! Only the first player takes a slot: the case of the defect
	protected void TakeSlot()
	{
		IEntity soldier;
		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		map<RplId, ref PS_PlayableContainer> playables = manager.GetPlayables();
		foreach (RplId id, PS_PlayableContainer container : playables)
		{
			if (container && container.GetPlayableComponent())
			{
				soldier = container.GetPlayableComponent().GetOwner();
				break;
			}
		}

		if (!Check("A playable slot exists", soldier != null))
		{
			Finish();
			return;
		}

		if (!Check("First player takes a slot", AssignSlot(m_iSlottedId, soldier), Pos(soldier.GetOrigin())))
		{
			Finish();
			return;
		}

		// The clients check the voices again meanwhile
		NextStep(45);
	}
}

//! Client side: checks how this client hears the other player, before and after the first player takes a slot.
class GDC_MissionTestClient_LobbyVoice : GDC_MissionTest
{
	protected int m_iSlottedId;
	protected int m_iOtherId;
	protected string m_sRole;

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "Lobby voice (client)";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForSlotSelection(); return;
			case 2: CheckVoice("without slot"); NextStep(1); return;
			case 3: WaitForSlot(); return;
			case 4: CheckAfterSlot(); return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForSlotSelection()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (controller && controller.GetPlayerId() > 0 && controller.GetControlledEntity() && GetPlayerCount() >= 2 && GetGameState() == SCR_EGameModeState.SLOTSELECTION)
		{
			m_iSlottedId = GetPlayerId(0);
			if (controller.GetPlayerId() == m_iSlottedId)
			{
				m_iOtherId = GetPlayerId(1);
				m_sRole = "slotted player";
			}
			else
			{
				m_iOtherId = m_iSlottedId;
				m_sRole = "player without slot";
			}

			Info(string.Format("Client of player %1 (%2), listens to player %3", controller.GetPlayerId(), m_sRole, m_iOtherId));
			// CVON updates who it hears every few seconds
			NextStep(15);
			return;
		}

		if (StepElapsed() > 600)
		{
			Check(m_sRole + ": slot selection reached", false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForSlot()
	{
		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		if (manager && manager.GetPlayableByPlayer(m_iSlottedId) != RplId.Invalid())
		{
			Info(m_sRole + ": first player is in a slot");
			// Teleport of his lobby entity, streaming, then a CVON update
			NextStep(15);
			return;
		}

		if (StepElapsed() > 120)
		{
			Check(m_sRole + ": first player seen in a slot", false, "after 120 s");
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAfterSlot()
	{
		bool streamed = CheckVoice("after the slot");
		if (streamed)
			Warn(m_sRole + ": the other player's entity is still streamed in after the slot, the cause of the defect isn't confirmed");
		else
			Info(m_sRole + ": the other player's entity is no longer streamed in (the AFI bridge dropped such a voice)");

		Finish();
	}

	//------------------------------------------------------------------------------------------------
	//! Logs where both lobby entities are, then checks what CVON would send to TeamSpeak for the other player
	//! \return Whether the other player's entity is streamed in on this client
	protected bool CheckVoice(string phase)
	{
		string prefix = m_sRole + ", " + phase + ": ";
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity own = controller.GetControlledEntity();
		IEntity other = GetPlayerEntity(m_iOtherId);

		string otherInfo = "not streamed in";
		if (other)
			otherInfo = string.Format("streamed in, at %1 m", Math.Round(vector.Distance(own.GetOrigin(), other.GetOrigin())));

		Info(prefix + "own entity at " + Pos(own.GetOrigin()) + ", other player's entity " + otherInfo);

		CVON_VONContainer entry = controller.m_aLocalEntries.Get(m_iOtherId);
		if (Check(prefix + "other player in the CVON voices", entry != null))
			Check(prefix + "other player heard over the lobby room", entry.m_bIsSpectator, typename.EnumToString(CVON_EVONType, entry.m_eVonType));

		SCR_VONController von = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		float left;
		float right;
		int silenced;
		von.ComputeSpectatorLR(m_iOtherId, left, right, silenced);
		Check(prefix + "full volume in both ears", left == 1 && right == 1, string.Format("left %1, right %2", left, right));

		return other != null;
	}
}
