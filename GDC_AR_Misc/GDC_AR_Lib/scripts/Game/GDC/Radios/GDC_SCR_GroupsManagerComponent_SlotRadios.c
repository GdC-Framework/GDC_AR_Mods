//! Applies the per-slot radio presets after Coalition VON has tuned the player's radios.
modded class SCR_GroupsManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void TuneFreqDelayWithPresets(int playerId, IEntity player)
	{
		super.TuneFreqDelayWithPresets(playerId, player);
		GDC_TuneSlotRadios(playerId, player);
	}

	//------------------------------------------------------------------------------------------------
	protected void GDC_TuneSlotRadios(int playerId, IEntity player)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(player);
		if (!character || character.m_bGDC_SlotRadiosTuned)
			return;

		if (character.m_sGDC_SlotShortRangeFrequency.IsEmpty() && character.m_sGDC_SlotLongRangeFrequency.IsEmpty())
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;

		bool shortRangeDone;
		bool longRangeDone;
		foreach (IEntity radio : playerController.m_aRadios)
		{
			if (!radio)
				continue;

			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
			if (!radioComp)
				continue;

			// only the first radio of each type follows the slot preset
			if (radioComp.m_eRadioType == CVON_ERadioType.SHORT && !shortRangeDone)
			{
				GDC_TuneRadio(radioComp, character.m_sGDC_SlotShortRangeFrequency);
				shortRangeDone = true;
			}
			else if (radioComp.m_eRadioType == CVON_ERadioType.LONG && !longRangeDone)
			{
				GDC_TuneRadio(radioComp, character.m_sGDC_SlotLongRangeFrequency);
				longRangeDone = true;
			}
		}

		// radios not initialized yet: Coalition VON calls this again when the player joins a group
		if (shortRangeDone || longRangeDone)
			character.m_bGDC_SlotRadiosTuned = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void GDC_TuneRadio(notnull CVON_RadioComponent radioComp, string frequency)
	{
		if (frequency.IsEmpty())
			return;

		// a frequency missing from the Coalition VON config becomes an extra channel of this radio
		int channelIndex = radioComp.m_aChannels.Find(frequency);
		if (channelIndex == -1)
			channelIndex = radioComp.m_aChannels.Insert(frequency);

		radioComp.UpdateChannelServer(channelIndex + 1);
		radioComp.UpdateFrequncyServer(frequency);
	}
}
