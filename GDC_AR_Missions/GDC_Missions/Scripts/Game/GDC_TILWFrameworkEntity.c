[EntityEditorProps(category: "GameScripted/Projectors", description: "TILW hook that calls a projector switch group")]
class GDC_TILWFrameworkEntityClass : TILW_MissionFrameworkEntityClass
{
}

class GDC_TILWFrameworkEntity : TILW_MissionFrameworkEntity
{
	[Attribute("", UIWidgets.Object, "Target projector switch group entity")]
	protected GDC_ProjectorSwitchGroupEntity m_switchGroup;

	override void FrameworkScript1()
	{
		Print("TILWMF | GDC | HOOK FrameworkScript1 invoked", LogLevel.WARNING);

		if (!m_switchGroup)
		{
			Print("TILWMF | GDC | FrameworkScript1 called but no switch group is assigned", LogLevel.ERROR);
			return;
		}

		Print("TILWMF | GDC | HOOK FrameworkScript1 forwarding SetMissionState(true)", LogLevel.WARNING);
		m_switchGroup.SetMissionState(true);
	}

	override void FrameworkScript2()
	{
		Print("TILWMF | GDC | HOOK FrameworkScript2 invoked", LogLevel.WARNING);

		if (!m_switchGroup)
		{
			Print("TILWMF | GDC | FrameworkScript2 called but no switch group is assigned", LogLevel.ERROR);
			return;
		}

		Print("TILWMF | GDC | HOOK FrameworkScript2 forwarding SetMissionState(false)", LogLevel.WARNING);
		m_switchGroup.SetMissionState(false);
	}
}
