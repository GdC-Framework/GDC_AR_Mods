//------------------------------------------------------------------------------------------------
//! Simple component to control the initial state of a LightEntity
//! Attach this component directly to a LightEntity to control its startup state
[ComponentEditorProps(category: "GameScripted/Lights", description: "Controls initial state (on/off) of a LightEntity")]
class GDC_LightStateComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Flicker modes
enum EFlickerMode
{
	BALANCED,           //! Balanced on/off alternation with random durations
	MICRO_OUTAGES,      //! Stays on with brief micro-outages in bursts
	INTENSITY_VARIATION //! Continuous intensity variations (no on/off)
}

//------------------------------------------------------------------------------------------------
class GDC_LightStateComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Light enabled at startup")]
	protected bool m_bInitiallyEnabled;
	
	[Attribute("0", UIWidgets.CheckBox, "Enable automatic flickering")]
	protected bool m_bAutoFlickerEnabled;
	
	[Attribute("0", UIWidgets.ComboBox, "Flicker mode", "", ParamEnumArray.FromEnum(EFlickerMode))]
	protected EFlickerMode m_eFlickerMode;
	
	[Attribute("2.0", UIWidgets.EditBox, "BALANCED mode only: Minimum duration in seconds before state change")]
	protected float m_fMinDuration;
	
	[Attribute("8.0", UIWidgets.EditBox, "BALANCED mode only: Maximum duration in seconds before state change")]
	protected float m_fMaxDuration;
	
	[Attribute("5.0", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Min on duration (seconds)")]
	protected float m_fMicroOnMinDuration;
	
	[Attribute("15.0", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Max on duration (seconds)")]
	protected float m_fMicroOnMaxDuration;
	
	[Attribute("0.1", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Min off duration (seconds)")]
	protected float m_fMicroOffMinDuration;
	
	[Attribute("0.5", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Max off duration (seconds)")]
	protected float m_fMicroOffMaxDuration;
	
	[Attribute("1", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Min number of outages in a burst")]
	protected int m_iMinBurstCount;
	
	[Attribute("3", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Max number of outages in a burst")]
	protected int m_iMaxBurstCount;
	
	[Attribute("0.2", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Min interval between bursts (seconds)")]
	protected float m_fMinBurstInterval;
	
	[Attribute("0.8", UIWidgets.EditBox, "MICRO_OUTAGES mode only: Max interval between bursts (seconds)")]
	protected float m_fMaxBurstInterval;
	
	[Attribute("3.0", UIWidgets.EditBox, "INTENSITY_VARIATION mode only: Base LV intensity")]
	protected float m_fBaseLV;
	
	[Attribute("0.5", UIWidgets.EditBox, "INTENSITY_VARIATION mode only: Min LV multiplier (e.g., 0.5 = 50% of base)")]
	protected float m_fMinLVMultiplier;
	
	[Attribute("1.2", UIWidgets.EditBox, "INTENSITY_VARIATION mode only: Max LV multiplier (e.g., 1.2 = 120% of base)")]
	protected float m_fMaxLVMultiplier;
	
	[Attribute("0.5", UIWidgets.EditBox, "INTENSITY_VARIATION mode only: Min duration for each intensity (seconds)")]
	protected float m_fIntensityMinDuration;
	
	[Attribute("2.0", UIWidgets.EditBox, "INTENSITY_VARIATION mode only: Max duration for each intensity (seconds)")]
	protected float m_fIntensityMaxDuration;
	
	protected bool m_bFlickerActive;
	protected bool m_bCurrentState;
	protected int m_iCurrentBurstRemaining;
	protected Color m_OriginalColor;
	
	//------------------------------------------------------------------------------------------------
	//! Enable or disable the light
	void SetLightEnabled(bool enabled)
	{
		LightEntity light = LightEntity.Cast(GetOwner());
		if (light)
		{
			light.SetEnabled(enabled);
			m_bCurrentState = enabled;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Set the startup state for this light when it is spawned.
	void SetInitialState(bool enabled)
	{
		m_bInitiallyEnabled = enabled;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set light intensity (LV)
	void SetLightIntensity(float lv)
	{
		LightEntity light = LightEntity.Cast(GetOwner());
		if (light)
		{
			light.SetColor(m_OriginalColor, lv);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns the current light state
	bool IsLightEnabled()
	{
		LightEntity light = LightEntity.Cast(GetOwner());
		if (light)
			return light.IsEnabled();
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Start automatic flickering
	void StartAutoFlicker()
	{
		if (!m_bFlickerActive)
		{
			m_bFlickerActive = true;
			m_iCurrentBurstRemaining = 0;
			ScheduleNextToggle();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Stop automatic flickering
	void StopAutoFlicker()
	{
		m_bFlickerActive = false;
		GetGame().GetCallqueue().Remove(ToggleLightState);
		GetGame().GetCallqueue().Remove(VaryIntensity);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Schedule the next state change
	protected void ScheduleNextToggle()
	{
		if (!m_bFlickerActive)
			return;
		
		float duration;
		
		// Choose duration based on mode
		if (m_eFlickerMode == EFlickerMode.INTENSITY_VARIATION)
		{
			// Intensity variation mode
			duration = Math.RandomFloat(m_fIntensityMinDuration, m_fIntensityMaxDuration) * 1000;
		}
		else if (m_eFlickerMode == EFlickerMode.MICRO_OUTAGES)
		{
			// Micro-outages mode with bursts
			if (m_bCurrentState)
			{
				// Currently on
				if (m_iCurrentBurstRemaining > 0)
				{
					// In the middle of a burst -> short interval before next outage
					duration = Math.RandomFloat(m_fMinBurstInterval, m_fMaxBurstInterval) * 1000;
				}
				else
				{
					// Stable on period -> wait long before starting a new burst
					duration = Math.RandomFloat(m_fMicroOnMinDuration, m_fMicroOnMaxDuration) * 1000;
					// Initialize new burst
					m_iCurrentBurstRemaining = Math.RandomInt(m_iMinBurstCount, m_iMaxBurstCount + 1);
				}
			}
			else
			{
				// Currently off (outage) -> turn back on quickly
				duration = Math.RandomFloat(m_fMicroOffMinDuration, m_fMicroOffMaxDuration) * 1000;
				// Decrement burst counter
				if (m_iCurrentBurstRemaining > 0)
					m_iCurrentBurstRemaining--;
			}
		}
		else
		{
			// Balanced mode: same duration regardless of state
			duration = Math.RandomFloat(m_fMinDuration, m_fMaxDuration) * 1000;
		}
		
		// Schedule next toggle via CallQueue
		if (m_eFlickerMode == EFlickerMode.INTENSITY_VARIATION)
			GetGame().GetCallqueue().CallLater(VaryIntensity, duration, false);
		else
			GetGame().GetCallqueue().CallLater(ToggleLightState, duration, false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Toggle the light state
	protected void ToggleLightState()
	{
		if (!m_bFlickerActive)
			return;
			
		SetLightEnabled(!m_bCurrentState);
		ScheduleNextToggle();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Vary the light intensity (for INTENSITY_VARIATION mode)
	protected void VaryIntensity()
	{
		if (!m_bFlickerActive)
			return;
		
		// Generate random intensity within range
		float multiplier = Math.RandomFloat(m_fMinLVMultiplier, m_fMaxLVMultiplier);
		float newLV = m_fBaseLV * multiplier;
		
		SetLightIntensity(newLV);
		ScheduleNextToggle();
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;
		
		// Owner must be a LightEntity
		LightEntity light = LightEntity.Cast(owner);
		if (!light)
		{
			Print("GDC_LightStateComponent: ERROR - This component must be attached to a LightEntity!", LogLevel.ERROR);
			return;
		}
		
		// Store original color for intensity variations
		m_OriginalColor = Color.White; // Default, will use light's existing color
		
		// In micro-outages mode, force initial state to "on"
		bool initialState = m_bInitiallyEnabled;
		if (m_bAutoFlickerEnabled)
		{
			if (m_eFlickerMode == EFlickerMode.MICRO_OUTAGES)
			{
				initialState = true;
			}
			else if (m_eFlickerMode == EFlickerMode.INTENSITY_VARIATION)
			{
				// Always on for intensity variation mode
				initialState = true;
			}
		}
		
		// Force the initial state from the component, overriding the prefab/default state.
		SetLightEnabled(initialState);
		if (LightEntity.Cast(owner))
			LightEntity.Cast(owner).SetEnabled(initialState);
		
		string state = "OFF";
		if (initialState)
			state = "ON";
		
		Print(string.Format("GDC_LightStateComponent: Initial state applied: %1", state), LogLevel.NORMAL);
		
		// Start automatic flickering if enabled
		if (m_bAutoFlickerEnabled)
		{
			StartAutoFlicker();
			
			string mode = "BALANCED";
			if (m_eFlickerMode == EFlickerMode.MICRO_OUTAGES)
				mode = "MICRO_OUTAGES";
			else if (m_eFlickerMode == EFlickerMode.INTENSITY_VARIATION)
				mode = "INTENSITY_VARIATION";
				
			Print(string.Format("GDC_LightStateComponent: Automatic flickering enabled (mode: %1)", mode), LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		StopAutoFlicker();
		super.OnDelete(owner);
	}
}
