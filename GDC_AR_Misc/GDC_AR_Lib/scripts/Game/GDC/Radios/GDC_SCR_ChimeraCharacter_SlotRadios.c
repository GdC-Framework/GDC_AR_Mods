//! Radio frequencies of the group slot this character was spawned from (server only).
//! Characters spawned outside a group slot (respawn, Game Master) have none.
modded class SCR_ChimeraCharacter
{
	string m_sGDC_SlotShortRangeFrequency;
	string m_sGDC_SlotLongRangeFrequency;

	//! Set once the slot frequencies have been applied, so a reconnecting player keeps their own tuning
	bool m_bGDC_SlotRadiosTuned;
}
