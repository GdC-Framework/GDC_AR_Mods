// GDC_SCR_PlayerController_Teleport.c
// Path in mod: Scripts/Game/GDC/Teleport/GDC_SCR_PlayerController_Teleport.c

//! Network path of teleports: the request goes up to the server, the order comes back to each traveller.
modded class SCR_PlayerController
{
	protected ref GDC_TimeSkipSequence m_GDC_TimeSkipSequence;

	//------------------------------------------------------------------------------------------------
	//! Sends a teleport request to the server. To be called on the machine of the requesting player.
	void GDC_RequestTeleport(notnull GDC_TeleportModuleComponent module)
	{
		RplId moduleId = module.GetRplId();
		if (!moduleId.IsValid())
		{
			Print(string.Format("[GDC_Teleport] '%1' is not replicated.", module.GetActionName()), LogLevel.WARNING);
			return;
		}

		Rpc(GDC_RpcAsk_Teleport, moduleId);
	}

	//------------------------------------------------------------------------------------------------
	//! The player is the owner of this controller: its id cannot be forged by the client.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GDC_RpcAsk_Teleport(RplId moduleId)
	{
		GDC_TeleportModuleComponent module = GDC_TeleportModuleComponent.FindByRplId(moduleId);
		if (module)
			module.Request(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	//! Orders the player owning this controller to travel. Server only.
	void GDC_SendTeleport(notnull GDC_TeleportModuleComponent module, vector position)
	{
		Rpc(GDC_RpcDo_Teleport, module.GetRplId(), position);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GDC_RpcDo_Teleport(RplId moduleId, vector position)
	{
		// A second request made during the sequence must not restart it.
		if (m_GDC_TimeSkipSequence && m_GDC_TimeSkipSequence.IsRunning())
			return;

		GDC_TeleportModuleComponent module = GDC_TeleportModuleComponent.FindByRplId(moduleId);
		if (!module)
			return;

		m_GDC_TimeSkipSequence = new GDC_TimeSkipSequence();
		m_GDC_TimeSkipSequence.Start(position, module.GetFadeOutDuration(), module.GetBlackDuration(), module.GetFadeInDuration(), module.GetText(), module.GetBackgroundImage(), module.GetBackgroundMaxScreenPercent(), module.GetMuteSound());
	}
}
