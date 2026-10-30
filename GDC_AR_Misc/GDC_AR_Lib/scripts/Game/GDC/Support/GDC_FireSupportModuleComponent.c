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

	//! Elevation limits of the vanilla mortars (Mortar_Base.et). The tables stop at exactly 45 degrees, hence the margin.
	protected const float MIN_ELEVATION_DEG = 45.1;
	protected const float MAX_ELEVATION_DEG = 85;

	//! Keeps the prefab loaded, as the source below belongs to it.
	protected ref Resource m_Resource;
	protected IEntitySource m_Source;

	//! Speed coefficients of the charges of the shell, lowest first.
	protected ref array<float> m_aChargeCoefs;

	//------------------------------------------------------------------------------------------------
	protected void LoadBallistics()
	{
		if (m_aChargeCoefs)
			return;

		int defaultCharge;
		m_aChargeCoefs = SCR_MortarShellGadgetComponent.GetPrefabInitSpeedCoef(m_sPrefab, defaultCharge);
		if (!m_aChargeCoefs)
		{
			m_aChargeCoefs = {};
			return;
		}

		m_aChargeCoefs.Sort();
		m_Resource = Resource.Load(m_sPrefab);
		m_Source = m_Resource.GetResource().ToEntitySource();
	}

	//------------------------------------------------------------------------------------------------
	//! Flight time with the lowest charge able to reach the distance, as a mortar crew would do.
	//! The difference of altitude is ignored: the exact engine method simulates the shell and is too slow for the map panel.
	//! \param distance Horizontal distance between the tube and the target, in metres
	//! \return Flight time in seconds, negative if the distance is out of range
	float GetFlightTime(float distance)
	{
		LoadBallistics();
		if (!m_Source || distance < 1)
			return -1;

		float maxAimTan = Math.Tan(MAX_ELEVATION_DEG * Math.DEG2RAD);
		foreach (float coef : m_aChargeCoefs)
		{
			float time;
			float aimHeight = BallisticTable.GetHeightFromProjectileSource(distance, time, m_Source, coef, false);
			if (time > 0 && aimHeight / distance <= maxAimTan)
				return time;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Shortest distance the shell can reach, in metres. 0 if the prefab has no ballistic data.
	float GetMinRange()
	{
		LoadBallistics();
		if (!m_Source || m_aChargeCoefs.IsEmpty())
			return 0;

		float time;
		return Math.Max(BallisticTable.GetDistanceOfProjectileSource(MAX_ELEVATION_DEG * Math.DEG2RAD, time, m_Source, m_aChargeCoefs[0], false), 0);
	}

	//------------------------------------------------------------------------------------------------
	//! \return Longest distance the shell can reach, in metres. 0 if the prefab has no ballistic data.
	float GetMaxRange()
	{
		LoadBallistics();
		if (!m_Source || m_aChargeCoefs.IsEmpty())
			return 0;

		float time;
		return Math.Max(BallisticTable.GetDistanceOfProjectileSource(MIN_ELEVATION_DEG * Math.DEG2RAD, time, m_Source, m_aChargeCoefs[m_aChargeCoefs.Count() - 1], false), 0);
	}
}

class GDC_FireSupportModuleComponentClass : GDC_SupportModuleComponentClass {}

//! Virtual fire support: shells fall on the target without any tube in the world.
//! The position of the owner entity stands for the tubes: it sets the range and the flight time.
class GDC_FireSupportModuleComponent : GDC_SupportModuleComponent
{
	[Attribute(desc: "Ammunition available to players. If there is only one, players don't have to choose.")]
	protected ref array<ref GDC_FireSupportAmmo> m_aAmmo;

	// The limits below also apply to values typed in a layer, as the attribute range only constrains the editor widget.
	protected const int ROUNDS_LIMIT = 50;
	protected const int DISPERSION_MIN_LIMIT = 5;
	protected const int DISPERSION_MAX_LIMIT = 500;

	protected const int MARKER_DELAY_MS = 1000;

	[Attribute("10", UIWidgets.EditBox, "Maximum number of rounds a player can ask for in a fire mission. 50 at most.", "1 50 1")]
	protected int m_iMaxRoundsPerFireMission;

	[Attribute("10", UIWidgets.EditBox, "Smallest radius of the impact disc a player can ask for, in metres. 5 at least.", "5 500 1")]
	protected int m_iMinDispersion;

	[Attribute("100", UIWidgets.EditBox, "Largest radius of the impact disc a player can ask for, in metres. 500 at most.", "5 500 1")]
	protected int m_iMaxDispersion;

	[Attribute("3", UIWidgets.EditBox, "Delay between two rounds in seconds.", "0 inf 0.1")]
	protected float m_fIntervalBetweenRounds;

	[Attribute("10", UIWidgets.EditBox, "Delay between the request and the first shot, in seconds. The flight time is added to it, computed from the position of this entity.", "0 inf 1")]
	protected float m_fFirstRoundDelay;

	[Attribute("1", UIWidgets.CheckBox, "When enabled, the minimum and maximum range of each ammunition are drawn in the World Editor while this entity is selected.")]
	protected bool m_bShowRangeRings;

	[Attribute("0", UIWidgets.CheckBox, "When enabled, a mortar symbol named after this support is shown on the map, at the position of this entity.")]
	protected bool m_bShowMapMarker;

	[Attribute(typename.EnumToString(EMilitarySymbolIdentity, EMilitarySymbolIdentity.BLUFOR), UIWidgets.ComboBox, "Faction of the mortar symbol. The vanilla markers only exist for BLUFOR, OPFOR, INDFOR and UNKNOWN.", enumType: EMilitarySymbolIdentity)]
	protected EMilitarySymbolIdentity m_eMarkerFaction;

	[Attribute("", UIWidgets.EditBox, "Key of the only faction that sees the marker (US, USSR, FIA...). Empty = all players.")]
	protected string m_sMarkerVisibleToFaction;

	protected ref GDC_FireMission m_Mission;

	//! Flight time of the first shell of the current mission, in seconds. Authority only.
	protected float m_fFlightTime;

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
	override bool IsTargetInRange(vector target, int option)
	{
		return IsOptionValid(option) && GetFlightTime(target, option) > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Flight time from this entity to the target in seconds, negative if the target is out of range
	float GetFlightTime(vector target, int option)
	{
		return m_aAmmo[option].GetFlightTime(vector.DistanceXZ(GetOwner().GetOrigin(), target));
	}

	//------------------------------------------------------------------------------------------------
	override protected bool StartMission(vector target, int option, int quantity, int spread)
	{
		m_fFlightTime = GetFlightTime(target, option);
		m_Mission = new GDC_FireMission(m_aAmmo[option].m_sPrefab, target, quantity, spread, m_fIntervalBetweenRounds);
		m_Mission.GetOnFirstRound().Insert(OnFirstRound);
		m_Mission.GetOnFinished().Insert(OnMissionFinished);
		m_Mission.Start(m_fFirstRoundDelay, m_fFlightTime);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFirstRound()
	{
		SendFeedback(m_iRequesterId, GDC_ESupportFeedback.FIRST_ROUND, GDC_ESupportRefusal.NONE, m_fFlightTime);
	}

	//------------------------------------------------------------------------------------------------
	override protected float GetStartDelay()
	{
		return m_fFirstRoundDelay;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !m_aAmmo)
			return;

		foreach (GDC_FireSupportAmmo ammo : m_aAmmo)
		{
			if (ammo.GetMaxRange() <= 0)
				Print(string.Format("[GDC_Support] '%1': ammunition '%2' has no ballistic data, every target will be out of range.", m_sDisplayName, ammo.m_sName), LogLevel.WARNING);
		}

		// The marker manager has to be replicated before it can tell clients about a marker.
		if (m_bShowMapMarker && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(InsertMapMarker, MARKER_DELAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Adds the marker of the vanilla military type, kept by the server for players joining later.
	protected void InsertMapMarker()
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
		{
			Print(string.Format("[GDC_Support] '%1': no map marker manager in this world.", m_sDisplayName), LogLevel.WARNING);
			return;
		}

		SCR_MapMarkerBase marker = markerManager.PrepareMilitaryMarker(m_eMarkerFaction, EMilitarySymbolDimension.LAND, EMilitarySymbolIcon.MORTAR);
		if (!marker)
		{
			Print(string.Format("[GDC_Support] '%1': no map marker for faction %2.", m_sDisplayName, typename.EnumToString(EMilitarySymbolIdentity, m_eMarkerFaction)), LogLevel.WARNING);
			return;
		}

		if (!m_sMarkerVisibleToFaction.IsEmpty())
		{
			FactionManager factionManager = GetGame().GetFactionManager();
			Faction faction;
			if (factionManager)
				faction = factionManager.GetFactionByKey(m_sMarkerVisibleToFaction);

			// Without the faction, the marker would be shown to everyone, enemies included.
			if (!faction)
			{
				Print(string.Format("[GDC_Support] '%1': unknown faction '%2', no map marker.", m_sDisplayName, m_sMarkerVisibleToFaction), LogLevel.WARNING);
				return;
			}

			marker.AddMarkerFactionFlags(factionManager.GetFactionIndex(faction));
		}

		vector origin = GetOwner().GetOrigin();
		marker.SetWorldPos(origin[0], origin[2]);
		marker.SetCustomText(m_sDisplayName);
		markerManager.InsertStaticMarker(marker, false, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(InsertMapMarker);

		if (m_Mission)
			m_Mission.Cancel();
	}

#ifdef WORKBENCH

	//------------------------------------------------------------------------------------------------
	//! Draws one ring per range for one frame. Rings follow the terrain.
	protected void DrawRangeRing(IEntity owner, float radius, int color, string label)
	{
		const int SEGMENTS = 128;
		vector center = owner.GetOrigin();
		BaseWorld world = owner.GetWorld();

		vector points[129];
		for (int i = 0; i <= SEGMENTS; i++)
		{
			float angle = i * Math.PI2 / SEGMENTS;
			vector point = center + Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
			point[1] = world.GetSurfaceY(point[0], point[2]) + 1;
			points[i] = point;
		}

		Shape.CreateLines(color, ShapeFlags.ONCE | ShapeFlags.NOZBUFFER, points, SEGMENTS + 1);

		vector labelPosition = points[0];
		DebugTextWorldSpace.Create(world, label, DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA, labelPosition[0], labelPosition[1], labelPosition[2], size: 16.0, color: color, bgColor: ARGB(160, 0, 0, 0));
	}

	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_SELECTED;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!m_bShowRangeRings || !m_aAmmo)
			return;

		array<int> colors = {ARGB(255, 255, 70, 50), ARGB(255, 70, 170, 255), ARGB(255, 90, 230, 90), ARGB(255, 255, 210, 50)};
		foreach (int i, GDC_FireSupportAmmo ammo : m_aAmmo)
		{
			float maxRange = ammo.GetMaxRange();
			if (maxRange <= 0)
				continue;

			float minRange = ammo.GetMinRange();
			int color = colors[i % colors.Count()];
			DrawRangeRing(owner, maxRange, color, string.Format("%1 max %2 m", ammo.m_sName, Math.Round(maxRange)));
			DrawRangeRing(owner, minRange, color, string.Format("%1 min %2 m", ammo.m_sName, Math.Round(minRange)));
		}
	}

#endif
}
