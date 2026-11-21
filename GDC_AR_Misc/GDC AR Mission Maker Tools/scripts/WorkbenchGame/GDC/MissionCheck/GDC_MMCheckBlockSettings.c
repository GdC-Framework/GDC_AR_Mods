#ifdef WORKBENCH
//! Block 4, system settings. Classes are looked up by name: no compile dependency on TILW.
//! - 4.1 [TILW] (warning) a PK_EntityDeleter without a prefab filter deletes every root entity in its sphere.
class GDC_MMCheckBlockSettings : GDC_MMCheckBlock
{
	protected static const string ENTITY_DELETER = "PK_EntityDeleter";

	//------------------------------------------------------------------------------------------------
	override bool IsImplemented()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		if (context.Uses(GDC_MMCheckFramework.TILW))
			CheckEntityDeleters(context, report);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckEntityDeleters(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		foreach (IEntitySource entity : context.m_aEntities)
		{
			if (!GDC_MMCheckContext.Inherits(entity.GetClassName(), ENTITY_DELETER))
				continue;

			array<ResourceName> filter = {};
			entity.Get("m_prefabFilterList", filter);
			if (filter.IsEmpty())
				report.Add("4.1", entity);
		}
	}
}
#endif
