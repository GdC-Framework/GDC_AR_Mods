// GDC_SCR_PlayerController_Support.c
// Path in mod: Scripts/Game/GDC/Support/GDC_SCR_PlayerController_Support.c

//! Network path of support requests. Support modules belong to the server, so a client can only reach them through its own player controller.
modded class SCR_PlayerController
{
	protected const float GDC_SUPPORT_HINT_DURATION = 8;

	//------------------------------------------------------------------------------------------------
	//! Sends a support request to the server. To be called on the machine of the requesting player.
	//! The answer comes back through GDC_RpcDo_SupportFeedback.
	//! \param option Index of the option chosen by the player (ammo type...)
	//! \param quantity Quantity chosen by the player (number of rounds...)
	//! \param spread Spread chosen by the player, in metres
	void GDC_RequestSupport(notnull GDC_SupportModuleComponent module, vector target, int option, int quantity, int spread)
	{
		RplId moduleId = Replication.FindItemId(module);
		if (!moduleId.IsValid())
		{
			Print(string.Format("[GDC_Support] '%1' is not replicated.", module.GetDisplayName()), LogLevel.WARNING);
			return;
		}

		Rpc(GDC_RpcAsk_RequestSupport, moduleId, target, option, quantity, spread);
	}

	//------------------------------------------------------------------------------------------------
	//! The player is the owner of this controller: its id cannot be forged by the client.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GDC_RpcAsk_RequestSupport(RplId moduleId, vector target, int option, int quantity, int spread)
	{
		GDC_SupportModuleComponent module = GDC_SupportModuleComponent.Cast(Replication.FindItem(moduleId));
		if (!module)
		{
			GDC_SendSupportFeedback(string.Empty, GDC_ESupportFeedback.REFUSED, GDC_ESupportRefusal.INVALID_REQUEST, 0);
			return;
		}

		module.Request(GetPlayerId(), target, option, quantity, spread);
	}

	//------------------------------------------------------------------------------------------------
	//! Sends a message to the player owning this controller, and to nobody else. Server only.
	void GDC_SendSupportFeedback(string supportName, GDC_ESupportFeedback feedback, GDC_ESupportRefusal refusal, int seconds)
	{
		Rpc(GDC_RpcDo_SupportFeedback, supportName, feedback, refusal, seconds);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GDC_RpcDo_SupportFeedback(string supportName, GDC_ESupportFeedback feedback, GDC_ESupportRefusal refusal, int seconds)
	{
		string text = GDC_GetSupportFeedbackText(feedback, refusal, seconds);
		Print(string.Format("[GDC_Support] '%1': %2", supportName, text));
		SCR_HintManagerComponent.ShowCustomHint(text, supportName, GDC_SUPPORT_HINT_DURATION);

		if (feedback == GDC_ESupportFeedback.REFUSED || feedback == GDC_ESupportFeedback.FINISHED)
			GDC_SupportMapPanel.RemoveMissionMarker(supportName);
	}

	//------------------------------------------------------------------------------------------------
	protected string GDC_GetSupportFeedbackText(GDC_ESupportFeedback feedback, GDC_ESupportRefusal refusal, int seconds)
	{
		switch (feedback)
		{
			case GDC_ESupportFeedback.ACCEPTED:
				return string.Format("Demande acceptée, premier coup dans %1 s.", seconds);

			case GDC_ESupportFeedback.FIRST_ROUND:
				return "Coups partis.";

			case GDC_ESupportFeedback.FINISHED:
				return "Fin de mission.";
		}

		return "Demande refusée : " + GDC_GetSupportRefusalText(refusal, seconds);
	}

	//------------------------------------------------------------------------------------------------
	string GDC_GetSupportRefusalText(GDC_ESupportRefusal refusal, int seconds)
	{
		switch (refusal)
		{
			case GDC_ESupportRefusal.NO_MISSION_LEFT:
				return "plus de mission disponible.";

			case GDC_ESupportRefusal.MISSION_ACTIVE:
				return "une mission est déjà en cours.";

			case GDC_ESupportRefusal.COOLDOWN:
				return string.Format("support indisponible pendant %1 s.", seconds);

			case GDC_ESupportRefusal.RANK_TOO_LOW:
				return "grade insuffisant.";

			case GDC_ESupportRefusal.MISSING_ITEM:
				return "équipement requis manquant.";

			case GDC_ESupportRefusal.TARGET_FORBIDDEN:
				return "cible en zone interdite au tir.";
		}

		return "demande invalide.";
	}
}
