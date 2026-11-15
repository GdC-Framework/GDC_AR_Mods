//! Name tags never show that a character is unconscious: an unconscious character keeps the tag of a conscious one.
//! Replaces the "No Icon For Uncon" mod, which only hid the vanilla icon: with COALITION Squad Interface the role icon
//! still switched to the unconscious image, and in vanilla the missing icon itself gave the state away.
//! Every path goes through ActivateEntityState: the life state change and the initial check of a joining player.
modded class SCR_NameTagData
{
	//------------------------------------------------------------------------------------------------
	override void ActivateEntityState(ENameTagEntityState state)
	{
		if (state == ENameTagEntityState.UNCONSCIOUS)
			return;

		super.ActivateEntityState(state);
	}
}
