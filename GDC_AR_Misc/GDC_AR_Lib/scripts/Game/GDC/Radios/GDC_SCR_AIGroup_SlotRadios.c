//! Per-slot radio presets for playable groups (Coalition VON).
//! The slot index is only known in SpawnGroupMember, so it is kept while the member is spawned
//! and the frequencies are stored on the character when it is added to the group.
modded class SCR_AIGroup
{
	[Attribute(desc: "Short-range radio (e.g. PRC-152) frequencies for each slot, in the same order as the unit prefab slots. Several radios: comma-separated list, first radio first (e.g. 32000, 31000). Leave an entry empty to keep the Coalition VON default.", category: "GDC Radios")]
	ref array<string> m_aGDC_SlotShortRangeFrequencies;

	[Attribute(desc: "Long-range radio (e.g. PRC-117) frequencies for each slot, in the same order as the unit prefab slots. Several radios: comma-separated list, first radio first. Leave an entry empty to use the group long-range frequency below.", category: "GDC Radios")]
	ref array<string> m_aGDC_SlotLongRangeFrequencies;

	[Attribute(desc: "Long-range radio (e.g. PRC-117) frequency for the first long-range radio of every member of this group who has no slot frequency above. Vehicle radios key-filled by such a member are tuned to it too. Leave empty to keep the Coalition VON default.", category: "GDC Radios")]
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
			GDC_ParseFrequencyList(m_aGDC_SlotShortRangeFrequencies[slotIndex], character.m_aGDC_SlotShortRangeFrequencies);

		if (m_aGDC_SlotLongRangeFrequencies && m_aGDC_SlotLongRangeFrequencies.IsIndexValid(slotIndex))
			GDC_ParseFrequencyList(m_aGDC_SlotLongRangeFrequencies[slotIndex], character.m_aGDC_SlotLongRangeFrequencies);

		if (character.m_aGDC_SlotLongRangeFrequencies.IsEmpty() && !m_sGDC_LongRangeFrequency.IsEmpty())
			character.m_aGDC_SlotLongRangeFrequencies.Insert(m_sGDC_LongRangeFrequency.Trim());
	}

	//------------------------------------------------------------------------------------------------
	//! Splits a comma-separated frequency list ("32000, 31000") into outFrequencies
	protected void GDC_ParseFrequencyList(string list, notnull array<string> outFrequencies)
	{
		outFrequencies.Clear();
		array<string> tokens = {};
		list.Split(",", tokens, true);
		foreach (string token : tokens)
		{
			token.TrimInPlace();
			if (!token.IsEmpty())
				outFrequencies.Insert(token);
		}
	}
}
