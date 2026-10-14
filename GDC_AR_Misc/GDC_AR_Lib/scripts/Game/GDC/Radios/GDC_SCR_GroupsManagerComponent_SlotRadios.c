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

		if (character.m_aGDC_SlotShortRangeFrequencies.IsEmpty() && character.m_aGDC_SlotLongRangeFrequencies.IsEmpty())
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;

		// the n-th radio of each type follows the n-th slot frequency of that type
		int shortRangeIndex;
		int longRangeIndex;
		foreach (IEntity radio : playerController.m_aRadios)
		{
			if (!radio)
				continue;

			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
			if (!radioComp)
				continue;

			if (radioComp.m_eRadioType == CVON_ERadioType.SHORT)
			{
				if (character.m_aGDC_SlotShortRangeFrequencies.IsIndexValid(shortRangeIndex))
					GDC_TuneRadio(radioComp, character.m_aGDC_SlotShortRangeFrequencies[shortRangeIndex]);

				shortRangeIndex++;
			}
			else if (radioComp.m_eRadioType == CVON_ERadioType.LONG)
			{
				if (character.m_aGDC_SlotLongRangeFrequencies.IsIndexValid(longRangeIndex))
					GDC_TuneRadio(radioComp, character.m_aGDC_SlotLongRangeFrequencies[longRangeIndex]);

				longRangeIndex++;
			}
		}

		// radios not initialized yet: Coalition VON calls this again when the player joins a group
		if (shortRangeIndex > 0 || longRangeIndex > 0)
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
