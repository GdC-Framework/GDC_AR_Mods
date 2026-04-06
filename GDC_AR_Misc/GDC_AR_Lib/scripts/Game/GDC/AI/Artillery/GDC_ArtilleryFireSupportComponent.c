// GDC_ArtilleryFireSupportComponent.c
// Path in mod: Scripts/Game/AI/Artillery/GDC_ArtilleryFireSupportComponent.c

//! Data container for a single fire order within a GDC_ArtilleryFireSupportComponent mission.
[BaseContainerProps()]
class GDC_ArtilleryFireOrder
{
	[Attribute("1", UIWidgets.EditBox, "Number of rounds to fire in this order. Set to -1 for infinite firing.")]
	int m_iShotCount;

	[Attribute("80", UIWidgets.EditBox, "Inner radius of the impact ring (m). Rounds will not fall closer than this distance from the center. Set to 0 for a full disc.")]
	float m_fMinRadius;

	[Attribute("90", UIWidgets.EditBox, "Outer radius of the impact ring (m). Rounds will not fall farther than this distance from the center.")]
	float m_fMaxRadius;

	[Attribute("5", UIWidgets.EditBox, "Delay in seconds before the next fire order begins. Ignored for the last order.")]
	float m_fDelayAfter;
}

//! Indirect fire support mission component.
//! Set m_sConditionFlag to trigger the mission when a TilW mission flag is set.
//! Leave m_sConditionFlag empty to activate immediately on mission start.
class GDC_ArtilleryFireSupportComponentClass : ScriptComponentClass {}

class GDC_ArtilleryFireSupportComponent : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, "Name of the mortar vehicle entity (or its SCR_AIGroup) to command.\nCase 1 — placed in editor: set this to the Name field of the mortar vehicle entity as shown in Object Properties.\nCase 2 — dynamically spawned composition: set this name on the mortar entity inside the composition prefab. Only one instance of the composition may be present in the world at a time; duplicate the composition and rename each mortar for multiple simultaneous spawns.\nThe entity may be the mortar vehicle itself (crew resolved automatically) or a pre-placed SCR_AIGroup.")]
	protected string m_sAIGroupName;

	[Attribute("", UIWidgets.EditBox, "TilW mission flag that triggers the fire mission. Leave empty to activate immediately on mission start.")]
	protected string m_sConditionFlag;

	[Attribute("5", UIWidgets.EditBox, "Delay in seconds between the condition being met and the first round being fired.")]
	protected float m_fInitialDelay;

	[Attribute(typename.EnumToString(SCR_EAIArtilleryAmmoType, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE),
		UIWidgets.ComboBox, "Ammunition type used for all fire orders in this mission.", enumType: SCR_EAIArtilleryAmmoType)]
	protected SCR_EAIArtilleryAmmoType m_eAmmoType;

	[Attribute(desc: "Ordered sequence of fire orders (e.g. ranging shot → adjustment → fire for effect). Executed from top to bottom.")]
	protected ref array<ref GDC_ArtilleryFireOrder> m_aFireOrders;

	protected int m_iCurrentOrderIndex;
	protected bool m_bActive;

#ifdef WORKBENCH
	[Attribute("1", UIWidgets.CheckBox, "When enabled, fire order rings are drawn in the World Editor when the entity is selected.")]
	protected bool m_bShowDebugRings;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
#endif

	//------------------------------------------------------------------------------------------------
	//! Starts the fire mission. Ignored if already active or called on a client.
	void ActivateMission()
	{
		if (!Replication.IsServer())
			return;

		if (m_bActive)
			return;

		if (!m_aFireOrders || m_aFireOrders.IsEmpty())
		{
			Print("[GDC_ArtilleryFireSupport] No fire orders configured.", LogLevel.WARNING);
			return;
		}

		m_bActive = true;
		m_iCurrentOrderIndex = 0;

		GetGame().GetCallqueue().CallLater(ExecuteCurrentOrder, m_fInitialDelay * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Submits the current fire order as a single activity. The AI manages firing cadence.
	protected void ExecuteCurrentOrder()
	{
		if (m_iCurrentOrderIndex >= m_aFireOrders.Count())
		{
			m_bActive = false;
			return;
		}

		GDC_ArtilleryFireOrder order = m_aFireOrders[m_iCurrentOrderIndex];

		SCR_AIGroupUtilityComponent utility = ResolveGroupUtility();
		if (!utility)
		{
			m_bActive = false;
			return;
		}

		SCR_AIStaticArtilleryActivity activity = new SCR_AIStaticArtilleryActivity(
			utility,
			null,
			GetRandomPositionInRing(order.m_fMinRadius, order.m_fMaxRadius),
			m_eAmmoType,
			order.m_iShotCount,
			SCR_AIActionBase.PRIORITY_ACTIVITY_ARTILLERY_SUPPORT
		);

		activity.m_OnActionCompleted.Insert(OnCurrentOrderFinished);
		activity.m_OnActionFailed.Insert(OnCurrentOrderFinished);

		utility.AddAction(activity);
	}

	//------------------------------------------------------------------------------------------------
	//! Called when the current fire order completes or fails. Schedules the next order.
	protected void OnCurrentOrderFinished()
	{
		float delayAfter = m_aFireOrders[m_iCurrentOrderIndex].m_fDelayAfter;
		m_iCurrentOrderIndex++;

		if (m_iCurrentOrderIndex < m_aFireOrders.Count())
			GetGame().GetCallqueue().CallLater(ExecuteCurrentOrder, delayAfter * 1000, false);
		else
			m_bActive = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns a uniformly distributed random position within a ring (annulus).
	//! Uses the square-root formula to ensure uniform area distribution.
	//! \param minRadius Inner radius of the ring in metres
	//! \param maxRadius Outer radius of the ring in metres
	//! \return A random world position within the ring centred on this entity
	protected vector GetRandomPositionInRing(float minRadius, float maxRadius)
	{
		vector center = GetOwner().GetOrigin();
		float angle   = Math.RandomFloat(0, Math.PI2);
		float r       = Math.Sqrt(Math.RandomFloat(minRadius * minRadius, maxRadius * maxRadius));
		return center + Vector(Math.Cos(angle) * r, 0, Math.Sin(angle) * r);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves the SCR_AIGroupUtilityComponent from the configured group name.
	//! Falls back to this entity's own name when m_sAIGroupName is empty,
	//! which allows dynamic spawning via TILW_PrefabSpawnerEntity.m_setEntityNames
	//! to configure the target group implicitly.
	//! Primary path: the named entity is itself an SCR_AIGroup.
	//! Fallback path: the named entity is a vehicle — the group is resolved from its crew.
	//! \return The utility component, or null if the group was not found
	protected SCR_AIGroupUtilityComponent ResolveGroupUtility()
	{
		string groupName = m_sAIGroupName;
		if (groupName == "")
			groupName = GetOwner().GetName();

		if (groupName == "")
		{
			Print("[GDC_ArtilleryFireSupport] No AIGroup name configured and entity has no name.", LogLevel.WARNING);
			return null;
		}

		IEntity targetEntity = GetGame().GetWorld().FindEntityByName(groupName);
		if (!targetEntity)
		{
			Print(string.Format("[GDC_ArtilleryFireSupport] Entity '%1' not found in the world.", groupName), LogLevel.WARNING);
			return null;
		}

		// Primary path: the entity is directly an SCR_AIGroup
		SCR_AIGroup group = SCR_AIGroup.Cast(targetEntity);
		if (group)
		{
			SCR_AIGroupUtilityComponent utility = group.GetGroupUtilityComponent();
			if (!utility)
				Print(string.Format("[GDC_ArtilleryFireSupport] Group '%1' has no SCR_AIGroupUtilityComponent.", groupName), LogLevel.WARNING);
			return utility;
		}

		// Fallback path: the entity is a vehicle — resolve the group from its crew
		return ResolveGroupUtilityFromVehicle(targetEntity, groupName);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves the SCR_AIGroupUtilityComponent by inspecting the occupants of a vehicle.
	//! Iterates compartment occupants, finds the first AI agent with a valid parent group.
	//! \param vehicle  The vehicle entity to inspect
	//! \param vehicleName  Name used only for log messages
	//! \return The utility component, or null on any failure
	protected SCR_AIGroupUtilityComponent ResolveGroupUtilityFromVehicle(IEntity vehicle, string vehicleName)
	{
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(
			vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!cm)
		{
			Print(string.Format("[GDC_ArtilleryFireSupport] Entity '%1' is neither an SCR_AIGroup nor a vehicle with compartments.", vehicleName), LogLevel.WARNING);
			return null;
		}

		array<IEntity> occupants = {};
		cm.GetOccupants(occupants);
		if (occupants.IsEmpty())
		{
			Print(string.Format("[GDC_ArtilleryFireSupport] Vehicle '%1' has no occupants — crew may not yet be spawned.", vehicleName), LogLevel.WARNING);
			return null;
		}

		foreach (IEntity occupant : occupants)
		{
			AIControlComponent aiCtrl = AIControlComponent.Cast(occupant.FindComponent(AIControlComponent));
			if (!aiCtrl)
				continue;	// player-controlled or no AI component

			AIAgent agent = aiCtrl.GetAIAgent();
			if (!agent)
				continue;	// AI not activated

			AIGroup parentGroup = agent.GetParentGroup();
			if (!parentGroup)
				continue;	// agent has no group

			SCR_AIGroup scrGroup = SCR_AIGroup.Cast(parentGroup);
			if (!scrGroup)
				continue;	// base AIGroup, not SCR_AIGroup

			SCR_AIGroupUtilityComponent utility = scrGroup.GetGroupUtilityComponent();
			if (utility)
				return utility;
		}

		Print(string.Format("[GDC_ArtilleryFireSupport] No valid AI group found among occupants of vehicle '%1'.", vehicleName), LogLevel.WARNING);
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Called when a TilW mission flag changes. Activates the mission if the condition flag is set.
	//! \param flag Name of the flag that changed
	//! \param value True if the flag was set, false if cleared
	protected void OnFlagChanged(string flag, bool value)
	{
		if (flag != m_sConditionFlag || !value)
			return;

		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (fw)
			fw.GetOnFlagChanged().Remove(OnFlagChanged);

		ActivateMission();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;

		if (m_sConditionFlag == "")
		{
			// Defer by one tick so that SetName() has been called by the spawner
			// before ActivateMission() resolves the group name (UC3: dynamic spawn).
			GetGame().GetCallqueue().CallLater(ActivateMission, 0, false);
			return;
		}

		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw)
		{
			Print("[GDC_ArtilleryFireSupport] TILW_MissionFrameworkEntity not found — cannot subscribe to flag.", LogLevel.WARNING);
			return;
		}

		if (fw.IsMissionFlag(m_sConditionFlag))
		{
			ActivateMission();
			return;
		}

		fw.GetOnFlagChanged().Insert(OnFlagChanged);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

#ifdef WORKBENCH

	//------------------------------------------------------------------------------------------------
	//! Redraws debug rings for all fire orders. Colors fade from yellow (first order) to red (last).
	protected void DrawDebugRings(IEntity owner)
	{
		m_aDebugShapes.Clear();

		if (!m_aFireOrders || m_aFireOrders.IsEmpty())
			return;

		vector center = owner.GetOrigin();
		int count     = m_aFireOrders.Count();

		for (int i = 0; i < count; i++)
		{
			GDC_ArtilleryFireOrder order = m_aFireOrders[i];

			float t;
			if (count > 1)
				t = i / (count - 1.0);
			else
				t = 0.0;

			int g     = (int)(255.0 * (1.0 - t));
			int color = ARGB(200, 255, g, 0);

			if (order.m_fMinRadius > 0)
				m_aDebugShapes.Insert(CreateCircle(center, order.m_fMinRadius, color));

			m_aDebugShapes.Insert(CreateCircle(center, order.m_fMaxRadius, color));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Creates a horizontal circle Shape with 64 segments.
	//! \param center World-space center of the circle
	//! \param radius Radius in metres
	//! \param color ARGB color value
	//! \return The created Shape handle
	protected Shape CreateCircle(vector center, float radius, int color)
	{
		vector points[65];
		float step = Math.PI2 / 64;

		for (int i = 0; i <= 64; i++)
		{
			float angle = i * step;
			points[i]   = center + Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
		}

		return Shape.CreateLines(color, ShapeFlags.TRANSP | ShapeFlags.NOZBUFFER, points, 65);
	}

	//------------------------------------------------------------------------------------------------
	//! Requests continuous Workbench updates while this entity is selected.
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	//! Redraws fire order rings every frame in Workbench when the entity is selected.
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (m_bShowDebugRings)
			DrawDebugRings(owner);
		else
			m_aDebugShapes.Clear();
	}

#endif
}
