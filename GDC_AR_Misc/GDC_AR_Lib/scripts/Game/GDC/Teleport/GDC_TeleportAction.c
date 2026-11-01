// GDC_TeleportAction.c
// Path in mod: Scripts/Game/GDC/Teleport/GDC_TeleportAction.c

//! Player action of a teleport module. Only sends the request: the module decides on the server.
class GDC_TeleportAction : ScriptedUserAction
{
	protected GDC_TeleportModuleComponent m_Module;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Module = GDC_TeleportModuleComponent.Cast(pOwnerEntity.FindComponent(GDC_TeleportModuleComponent));
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (m_Module && controller)
			controller.GDC_RequestTeleport(m_Module);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!m_Module)
			return false;

		outName = m_Module.GetActionName();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Module)
			return false;

		return m_Module.IsAllowed(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user));
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
}
