//! Per-slot radio presets for playable groups (Coalition VON).
//! The slot index is only known in SpawnGroupMember, so it is kept while the member is spawned
//! and the frequencies are stored on the character when it is added to the group.
modded class SCR_AIGroup
{
	[Attribute(desc: "Short-range radio (e.g. PRC-152) frequency for each slot, in the same order as the unit prefab slots. Leave an entry empty to keep the Coalition VON default.", category: "GDC Radios")]
	ref array<string> m_aGDC_SlotShortRangeFrequencies;

	[Attribute(desc: "Long-range radio (e.g. PRC-117) frequency for each slot, in the same order as the unit prefab slots. Leave an entry empty to use the group long-range frequency below.", category: "GDC Radios")]
	ref array<string> m_aGDC_SlotLongRangeFrequencies;

	[Attribute(desc: "Long-range radio (e.g. PRC-117) frequency for every member of this group who carries one and has no slot frequency above. Leave empty to keep the Coalition VON default.", category: "GDC Radios")]
	string m_sGDC_LongRangeFrequency;

	//! Slot index of the member currently being spawned, -1 outside SpawnGroupMember
	protected int m_iGDC_SpawningSlotIndex = -1;

	//------------------------------------------------------------------------------------------------
	override protected bool SpawnGroupMember(bool snapToTerrain, int index, ResourceName res, bool editMode, bool isLast)
	{
		if (!editMode)
			m_iGDC_SpawningSlotIndex = index;

		bool result = super.SpawnGroupMember(snapToTerrain, index, res, editMode, isLast);
		m_iGDC_SpawningSlotIndex = -1;
		return result;
	}

	//------------------------------------------------------------------------------------------------
	override bool AddAIEntityToGroup(IEntity entity)
	{
		if (m_iGDC_SpawningSlotIndex >= 0)
			GDC_StoreSlotRadioPreset(SCR_ChimeraCharacter.Cast(entity), m_iGDC_SpawningSlotIndex);

		return super.AddAIEntityToGroup(entity);
	}

	//------------------------------------------------------------------------------------------------
	protected void GDC_StoreSlotRadioPreset(SCR_ChimeraCharacter character, int slotIndex)
	{
		if (!character)
			return;

		if (m_aGDC_SlotShortRangeFrequencies && m_aGDC_SlotShortRangeFrequencies.IsIndexValid(slotIndex))
			character.m_sGDC_SlotShortRangeFrequency = m_aGDC_SlotShortRangeFrequencies[slotIndex];

		character.m_sGDC_SlotLongRangeFrequency = m_sGDC_LongRangeFrequency;
		if (m_aGDC_SlotLongRangeFrequencies && m_aGDC_SlotLongRangeFrequencies.IsIndexValid(slotIndex) && !m_aGDC_SlotLongRangeFrequencies[slotIndex].IsEmpty())
			character.m_sGDC_SlotLongRangeFrequency = m_aGDC_SlotLongRangeFrequencies[slotIndex];
	}
}
