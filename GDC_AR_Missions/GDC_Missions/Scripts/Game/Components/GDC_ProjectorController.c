[ComponentEditorProps(category: "GameScripted/Projectors", description: "Controls a projector root and the associated LightEntity instances")]
class GDC_ProjectorControllerClass : ScriptComponentClass
{
}

class GDC_ProjectorController : ScriptComponent
{
	// Lights owned by this projector instance.
	protected ref array<LightEntity> m_projectorLights = new array<LightEntity>();
	// Current runtime state of the projector.
	protected bool m_bEnabled = true;
	// Base state used when no mission override is active.
	protected bool m_bInitialState = true;
	// True while the mission is explicitly forcing the projector state.
	protected bool m_bMissionOverrideActive = false;

	void RegisterLight(LightEntity light)
	{
		if (!light || m_projectorLights.Contains(light))
			return;
		m_projectorLights.Insert(light);
	}

	void ClearLights()
	{
		m_projectorLights.Clear();
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		m_projectorLights.Clear();
		if (!owner)
			return;

		LightEntity ownLight = LightEntity.Cast(owner);
		if (ownLight)
			RegisterLight(ownLight);
	}

	bool IsEnabled()
	{
		return m_bEnabled;
	}

	// Sets the projector's natural, startup state.
	void SetInitialState(bool enabled)
	{
		m_bInitialState = enabled;
		if (!m_bMissionOverrideActive)
			SetRuntimeState(enabled);
	}

	// Called by the mission framework when it imposes an active state.
	void SetMissionState(bool enabled)
	{
		m_bMissionOverrideActive = true;
		SetRuntimeState(enabled);
	}

	// Clears the TILW override and restores the projector to its initial state.
	void ClearMissionState()
	{
		m_bMissionOverrideActive = false;
		SetRuntimeState(m_bInitialState);
	}

	void SetEnabled(bool enabled)
	{
		SetRuntimeState(enabled);
	}

	protected void SetRuntimeState(bool enabled)
	{
		m_bEnabled = enabled;
		ApplyState(enabled);
	}

	protected void ApplyState(bool enabled)
	{
		foreach (LightEntity light : m_projectorLights)
		{
			if (!light)
				continue;

			GDC_LightStateComponent lightState = GDC_LightStateComponent.Cast(light.FindComponent(GDC_LightStateComponent));
			if (lightState)
			{
				lightState.SetLightEnabled(enabled);
				continue;
			}

			light.SetEnabled(enabled);
		}
	}
}
