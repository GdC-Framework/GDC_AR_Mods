//! Radio frequencies of the group slot this character was spawned from (server only), one per radio of each type.
//! Characters spawned outside a group slot (respawn, Game Master) have none.
modded class SCR_ChimeraCharacter
{
	ref array<string> m_aGDC_SlotShortRangeFrequencies = {};
	ref array<string> m_aGDC_SlotLongRangeFrequencies = {};

	//! Set once the slot frequencies have been applied, so a reconnecting player keeps their own tuning
	bool m_bGDC_SlotRadiosTuned;
}
