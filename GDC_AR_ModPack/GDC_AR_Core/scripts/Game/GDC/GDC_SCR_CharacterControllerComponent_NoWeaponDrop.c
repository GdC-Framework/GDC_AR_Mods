//! Players who fall unconscious put their weapon away instead of dropping it, as vanilla already does for AI.
//! A live grenade in hand is still dropped. Replaces the "No Weapon Drop Unconscious" mod, which copied the whole
//! vanilla method without calling super: here vanilla keeps handling AI, only the player case is added.
modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnConsciousnessChanged(bool conscious)
	{
		super.OnConsciousnessChanged(conscious);

		if (GetLifeState() != ECharacterLifeState.INCAPACITATED)
			return;

		// Vanilla has already put the weapon of an active AI away
		AIControlComponent aiControl = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		if (aiControl && aiControl.IsAIActivated())
			return;

		BaseWeaponManagerComponent weaponManager = GetWeaponManagerComponent();
		if (!weaponManager || !weaponManager.GetCurrentWeapon())
			return;

		IEntity currentWeapon = weaponManager.GetCurrentWeapon().GetOwner();
		if (!currentWeapon)
			return;

		// Same rule as vanilla: a grenade already triggered or being thrown falls, anything else is put away
		SCR_CharacterCommandHandlerComponent handler = SCR_CharacterCommandHandlerComponent.Cast(GetAnimationComponent().GetCommandHandler());
		if (handler && currentWeapon.FindComponent(GrenadeMoveComponent))
		{
			BaseTriggerComponent trigger = BaseTriggerComponent.Cast(currentWeapon.FindComponent(BaseTriggerComponent));
			if ((trigger && trigger.WasTriggered()) || handler.IsThrowingAction())
			{
				handler.DropLiveGrenadeFromHand(false);
				return;
			}
		}

		TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual, true);
	}
}
