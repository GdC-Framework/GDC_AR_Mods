// GDC_FireSupportModuleComponent.c
// Path in mod: Scripts/Game/GDC/Support/GDC_FireSupportModuleComponent.c

//! One type of ammunition available for a fire support.
[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GDC_FireSupportAmmo
{
	[Attribute("", UIWidgets.EditBox, "Name shown to players.")]
	string m_sName;

	[Attribute("", UIWidgets.ResourceNamePicker, "Shell prefab. Use the vanilla *_EffectModule shells.", "et")]
	ResourceName m_sPrefab;
}

class GDC_FireSupportModuleComponentClass : GDC_SupportModuleComponentClass {}

//! Virtual fire support: shells fall on the target without any tube in the world.
class GDC_FireSupportModuleComponent : GDC_SupportModuleComponent
{
	[Attribute(desc: "Ammunition available to players. If there is only one, players don't have to choose.")]
	protected ref array<ref GDC_FireSupportAmmo> m_aAmmo;

	// The limits below also apply to values typed in a layer, as the attribute range only constrains the editor widget.
	protected const int ROUNDS_LIMIT = 50;
	protected const int DISPERSION_MIN_LIMIT = 5;
	protected const int DISPERSION_MAX_LIMIT = 500;

	[Attribute("10", UIWidgets.EditBox, "Maximum number of rounds a player can ask for in a fire mission. 50 at most.", "1 50 1")]
	protected int m_iMaxRoundsPerFireMission;

	[Attribute("10", UIWidgets.EditBox, "Smallest radius of the impact disc a player can ask for, in metres. 5 at least.", "5 500 1")]
	protected int m_iMinDispersion;

	[Attribute("100", UIWidgets.EditBox, "Largest radius of the impact disc a player can ask for, in metres. 500 at most.", "5 500 1")]
	protected int m_iMaxDispersion;

	[Attribute("3", UIWidgets.EditBox, "Delay between two rounds in seconds.", "0 inf 0.1")]
	protected float m_fIntervalBetweenRounds;

	[Attribute("10", UIWidgets.EditBox, "Delay between the request and the first shell, in seconds.", "0 inf 1")]
	protected float m_fFirstRoundDelay;

	[Attribute("200", UIWidgets.EditBox, "Height above the ground at which shells are spawned, in metres.", "50 1000 1")]
	protected float m_fSpawnHeight;

	protected ref GDC_FireMission m_Mission;

	//------------------------------------------------------------------------------------------------
	//! \return The ammunition available to players
	array<ref GDC_FireSupportAmmo> GetAmmo()
	{
		return m_aAmmo;
	}

	//------------------------------------------------------------------------------------------------
	override void GetOptionNames(notnull array<string> names)
	{
		super.GetOptionNames(names);

		if (!m_aAmmo)
			return;

		foreach (GDC_FireSupportAmmo ammo : m_aAmmo)
		{
			names.Insert(ammo.m_sName);
		}
	}

	//------------------------------------------------------------------------------------------------
	override int GetMaxQuantity()
	{
		return Math.ClampInt(m_iMaxRoundsPerFireMission, 1, ROUNDS_LIMIT);
	}

	//------------------------------------------------------------------------------------------------
	override int GetMinSpread()
	{
		return Math.ClampInt(Math.Min(m_iMinDispersion, m_iMaxDispersion), DISPERSION_MIN_LIMIT, DISPERSION_MAX_LIMIT);
	}

	//------------------------------------------------------------------------------------------------
	override int GetMaxSpread()
	{
		return Math.ClampInt(Math.Max(m_iMinDispersion, m_iMaxDispersion), DISPERSION_MIN_LIMIT, DISPERSION_MAX_LIMIT);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool IsOptionValid(int option)
	{
		return m_aAmmo && m_aAmmo.IsIndexValid(option);
	}

	//------------------------------------------------------------------------------------------------
	override protected bool StartMission(vector target, int option, int quantity, int spread)
	{
		m_Mission = new GDC_FireMission(m_aAmmo[option].m_sPrefab, target, quantity, spread, m_fIntervalBetweenRounds, m_fSpawnHeight);
		m_Mission.GetOnFirstRound().Insert(OnFirstRound);
		m_Mission.GetOnFinished().Insert(OnMissionFinished);
		m_Mission.Start(m_fFirstRoundDelay);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFirstRound()
	{
		SendFeedback(m_iRequesterId, GDC_ESupportFeedback.FIRST_ROUND, GDC_ESupportRefusal.NONE, 0);
	}

	//------------------------------------------------------------------------------------------------
	override protected float GetStartDelay()
	{
		return m_fFirstRoundDelay;
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		if (m_Mission)
			m_Mission.Cancel();
	}
}
