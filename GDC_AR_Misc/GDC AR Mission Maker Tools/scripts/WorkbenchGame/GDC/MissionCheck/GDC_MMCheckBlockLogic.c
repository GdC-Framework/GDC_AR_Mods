#ifdef WORKBENCH
//! Block 5, mission logic (triggers, events, interactions, spawners). Classes are looked up by name: no compile
//! dependency on TILW.
//! - 5.1 [TILW] (warning) a TILW interaction is not the child of another entity (its label may break on a dedicated
//!   server);
//! - 5.2 [TILW] no TILW_VehicleCrewComponent (dead code with Reforger Lobby since TILW dropped its RL dependency).
class GDC_MMCheckBlockLogic : GDC_MMCheckBlock
{
	protected static const string TILW_INTERACTION = "TILW_BaseInteraction";
	protected static const string TILW_VEHICLE_CREW = "TILW_VehicleCrewComponent";
	protected static const string ACTIONS_MANAGER = "ActionsManagerComponent";

	//------------------------------------------------------------------------------------------------
	override bool IsImplemented()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		if (context.Uses(GDC_MMCheckFramework.TILW))
			CheckTilw(context, report);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckTilw(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		foreach (IEntitySource entity : context.m_aEntities)
		{
			if (entity.GetParent() && HasTilwInteraction(entity))
				report.Add("5.1", entity);

			for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
			{
				if (GDC_MMCheckContext.Inherits(entity.GetComponent(i).GetClassName(), TILW_VEHICLE_CREW))
				{
					report.Add("5.2", entity);
					break;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether one of the entity's additional actions is a TILW interaction
	protected static bool HasTilwInteraction(IEntitySource entity)
	{
		for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = entity.GetComponent(i);
			if (!GDC_MMCheckContext.Inherits(component.GetClassName(), ACTIONS_MANAGER))
				continue;

			BaseContainerList actions = component.GetObjectArray("additionalActions");
			if (!actions)
				continue;

			for (int a = 0, actionCount = actions.Count(); a < actionCount; a++)
			{
				BaseContainer action = actions.Get(a);
				if (action && GDC_MMCheckContext.Inherits(action.GetClassName(), TILW_INTERACTION))
					return true;
			}
		}

		return false;
	}
}
#endif
