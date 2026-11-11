// GDC_TeleportModuleComponent.c
// Path in mod: Scripts/Game/GDC/Teleport/GDC_TeleportModuleComponent.c

//! Who can use a teleport module, and who travels.
enum GDC_ETeleportMode
{
	INDIVIDUAL,	//!< Any player can use it and travels alone
	GROUP,		//!< Only a group leader can use it, and takes along the members of the group gathered around the module
	ALL_PLAYERS	//!< Only a player of sufficient rank can use it, and takes along all the players gathered around the module, whatever their group
}

class GDC_TeleportModuleComponentClass : ScriptComponentClass {}

//! Teleports players to a named entity, optionally behind a fade to black simulating a time skip.
//! Requests are decided on the server, then each traveller plays the sequence on its own machine.
//! The owner entity needs an RplComponent and a GDC_TeleportAction.
class GDC_TeleportModuleComponent : ScriptComponent
{
	[Attribute("Téléportation", UIWidgets.EditBox, "Name of the action shown to players.")]
	protected string m_sActionName;

	[Attribute("", UIWidgets.EditBox, "Name of the entity players are teleported to.")]
	protected string m_sDestinationName;

	[Attribute(typename.EnumToString(GDC_ETeleportMode, GDC_ETeleportMode.INDIVIDUAL), UIWidgets.ComboBox, "INDIVIDUAL: any player can use it and travels alone. GROUP: only a group leader can use it, and takes along the group members gathered around the module. ALL_PLAYERS: only a player of the minimum rank can use it, and takes along all the players gathered around the module, whatever their group.", enumType: GDC_ETeleportMode)]
	protected GDC_ETeleportMode m_eMode;

	[Attribute("30", UIWidgets.EditBox, "GROUP and ALL_PLAYERS modes: players further than this from the module, in metres, stay behind.", "1 1000 1")]
	protected float m_fRadius;

	[Attribute(typename.EnumToString(SCR_ECharacterRank, SCR_ECharacterRank.INVALID), UIWidgets.ComboBox, "ALL_PLAYERS mode: minimum rank required to use the module. INVALID = no rank requirement.", enumType: SCR_ECharacterRank)]
	protected SCR_ECharacterRank m_eMinRankRequired;

	[Attribute("0", UIWidgets.CheckBox, "GROUP and ALL_PLAYERS modes: draws a line on the ground along the radius, so that players see where to gather. It follows the terrain only: roads, floors and tall grass hide it.", category: "Zone marking")]
	protected bool m_bShowZoneOnGround;

	[Attribute("0.15", UIWidgets.EditBox, "Width of the line on the ground, in metres.", "0.05 2 0.05", category: "Zone marking")]
	protected float m_fZoneLineWidth;

	[Attribute("{C77A89CC3D3BC257}Assets/Decals/GDC_ZoneLine_White.emat", UIWidgets.ResourcePickerThumbnail, "Decal material of the line on the ground. The default one is a white line.", "emat", category: "Zone marking")]
	protected ResourceName m_sZoneLineMaterial;

	[Attribute("2", UIWidgets.EditBox, "Duration of the fade to black, in seconds. If the three durations are 0 the teleport is immediate.", "0 30 0.1", category: "Time skip")]
	protected float m_fFadeOutDuration;

	[Attribute("4", UIWidgets.EditBox, "Time spent on the black screen, in seconds. The player is moved at its start, so it must leave enough time to load the destination.", "0 120 0.1", category: "Time skip")]
	protected float m_fBlackDuration;

	[Attribute("2", UIWidgets.EditBox, "Duration of the fade back from black, in seconds.", "0 30 0.1", category: "Time skip")]
	protected float m_fFadeInDuration;

	[Attribute("", UIWidgets.EditBox, "Text shown on the black screen, for example \"Deux heures plus tard...\". Empty = no text.", category: "Time skip")]
	protected string m_sText;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Image shown on the black screen, centred and undistorted. Empty = black screen only.", "edds", category: "Time skip")]
	protected ResourceName m_sBackgroundImage;

	[Attribute("60", UIWidgets.EditBox, "Maximum size of the image, in percent of the screen. The image is reduced to fit and never enlarged beyond its native resolution, so it stays sharp but may look smaller on a large screen.", "10 100 5", category: "Time skip")]
	protected float m_fBackgroundMaxScreenPercent;

	[Attribute("1", UIWidgets.CheckBox, "Fades the game sounds along with the picture. Voice and radio are not affected.", category: "Time skip")]
	protected bool m_bMuteSound;

	[Attribute("-1", UIWidgets.EditBox, "Hour of the game clock after the time skip, for all players in the world, changed at the first use of the module only. -1 = clock unchanged.", "-1 23 1", category: "Time skip")]
	protected int m_iNewHour;

	[Attribute("0", UIWidgets.EditBox, "Minutes of the game clock after the time skip. Ignored if the hour is -1.", "0 59 1", category: "Time skip")]
	protected int m_iNewMinute;

	//! Distance around the module from which a request is accepted, in metres. Wider than the action radius to allow for latency.
	protected const float MAX_USE_DISTANCE = 10;

	//! Arrival points are laid out in rings around the destination.
	protected const float ARRIVAL_SPACING = 2;
	protected const float ARRIVAL_SEARCH_RADIUS = 1.5;
	protected const int ARRIVAL_SLOT_COUNT = 37;

	//! Server only. Successive travellers don't land on each other, even when they come from separate requests.
	protected int m_iNextArrivalSlot;

	//! Server only. A later use of the module must not send the clock back, for instance when several groups leave one after the other.
	protected bool m_bTimeChanged;

	//! Distance between two points of the line on the ground, in metres.
	protected const float ZONE_POINT_SPACING = 1;
	protected const int ZONE_MIN_POINTS = 16;

	//! A finalized track decal is removed after its lifetime: this one must last the whole game.
	protected const float ZONE_LINE_LIFETIME = 100000000;

	//! Clients only. Keeps the line on the ground alive.
	protected ref TrackDecalFactory m_ZoneLine;

	//! Clients only. True once the whole line has been drawn.
	protected bool m_bZoneDrawn;

	//------------------------------------------------------------------------------------------------
	//! \return True if the line on the ground has been drawn on this machine
	bool IsZoneDrawn()
	{
		return m_bZoneDrawn;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// The line is only a visual aid: nothing to draw on a dedicated server.
		if (System.IsConsoleApp() || !HasZone())
			return;

		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Not in the World Editor outside play mode, where the zone is drawn as a debug shape instead.
		if (GetGame().InPlayMode())
			DrawZoneOnGround();
	}

	//------------------------------------------------------------------------------------------------
	//! \return True if the line on the ground is wanted and meaningful: the radius only matters in GROUP and ALL_PLAYERS modes.
	protected bool HasZone()
	{
		return m_bShowZoneOnGround && m_eMode != GDC_ETeleportMode.INDIVIDUAL && m_fRadius > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Fills outPoints with the circle of the radius around the module, on the terrain: count segments, the last point closing the circle.
	protected void GetZonePoints(int count, notnull array<vector> outPoints)
	{
		vector center = GetOwner().GetOrigin();
		BaseWorld world = GetOwner().GetWorld();

		for (int i = 0; i <= count; i++)
		{
			float angle = i * Math.PI2 / count;
			vector point = center + Vector(Math.Cos(angle) * m_fRadius, 0, Math.Sin(angle) * m_fRadius);
			point[1] = world.GetSurfaceY(point[0], point[2]);
			outPoints.Insert(point);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Draws the circle of the radius on the terrain, as one track decal (the engine's tyre track system).
	protected void DrawZoneOnGround()
	{
		GenericWorldEntity worldEntity = GetGame().GetWorldEntity();
		if (!worldEntity || m_sZoneLineMaterial.IsEmpty())
			return;

		array<vector> points = {};
		GetZonePoints(Math.Max(ZONE_MIN_POINTS, Math.Ceil(Math.PI2 * m_fRadius / ZONE_POINT_SPACING)), points);

		m_ZoneLine = new TrackDecalFactory(m_sZoneLineMaterial, m_fZoneLineWidth, ZONE_LINE_LIFETIME);
		foreach (vector point : points)
		{
			GenericTerrainEntity terrain = worldEntity.GetTerrain(point[0], point[2]);
			if (!terrain)
				continue;

			vector normal = SCR_TerrainHelper.GetTerrainNormal(point, GetOwner().GetWorld());
			if (!m_ZoneLine.AddPoint(terrain, point, normal, 1))
			{
				Print("[GDC_Teleport] The line on the ground could not be drawn.", LogLevel.WARNING);
				return;
			}
		}

		m_ZoneLine.FinalizeDecal(0);
		m_bZoneDrawn = true;
	}

	//------------------------------------------------------------------------------------------------
	//! This component has no replicated member, so it isn't a replicated item itself: its RplComponent identifies it.
	RplId GetRplId()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rpl)
			return rpl.Id();

		return RplId.Invalid();
	}

	//------------------------------------------------------------------------------------------------
	//! \return The module designated by GetRplId(), null if none
	static GDC_TeleportModuleComponent FindByRplId(RplId id)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(Replication.FindItem(id));
		if (!rpl)
			return null;

		return GDC_TeleportModuleComponent.Cast(rpl.GetEntity().FindComponent(GDC_TeleportModuleComponent));
	}

	//------------------------------------------------------------------------------------------------
	string GetActionName()
	{
		return m_sActionName;
	}

	//------------------------------------------------------------------------------------------------
	float GetFadeOutDuration()
	{
		return m_fFadeOutDuration;
	}

	//------------------------------------------------------------------------------------------------
	float GetBlackDuration()
	{
		return m_fBlackDuration;
	}

	//------------------------------------------------------------------------------------------------
	float GetFadeInDuration()
	{
		return m_fFadeInDuration;
	}

	//------------------------------------------------------------------------------------------------
	string GetText()
	{
		return m_sText;
	}

	//------------------------------------------------------------------------------------------------
	ResourceName GetBackgroundImage()
	{
		return m_sBackgroundImage;
	}

	//------------------------------------------------------------------------------------------------
	float GetBackgroundMaxScreenPercent()
	{
		return m_fBackgroundMaxScreenPercent;
	}

	//------------------------------------------------------------------------------------------------
	bool GetMuteSound()
	{
		return m_bMuteSound;
	}

	//------------------------------------------------------------------------------------------------
	//! \return True if this player can use the module: only group leaders in GROUP mode, the required rank in ALL_PLAYERS mode. Valid on server and clients.
	bool IsAllowed(int playerId)
	{
		if (m_eMode == GDC_ETeleportMode.GROUP)
			return GetLedGroup(playerId) != null;

		if (m_eMode == GDC_ETeleportMode.ALL_PLAYERS)
			return HasRequiredRank(playerId);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Handles a request from a player. Server only.
	void Request(int playerId)
	{
		IEntity destination = GetGame().GetWorld().FindEntityByName(m_sDestinationName);
		if (!destination)
		{
			Print(string.Format("[GDC_Teleport] Entity '%1' could not be found.", m_sDestinationName), LogLevel.ERROR);
			return;
		}

		// The client only hides the action: the server has the last word.
		if (!IsAllowed(playerId))
			return;

		IEntity requester = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!CanTravel(requester) || vector.DistanceSq(requester.GetOrigin(), GetOwner().GetOrigin()) > MAX_USE_DISTANCE * MAX_USE_DISTANCE)
			return;

		array<int> travellers = {};
		GetTravellers(playerId, travellers);

		foreach (int traveller : travellers)
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(traveller));
			if (controller)
				controller.GDC_SendTeleport(this, GetArrivalPosition(destination.GetOrigin()));
		}

		if (m_iNewHour >= 0 && !m_bTimeChanged)
		{
			m_bTimeChanged = true;

			// Once the travellers' screens are black, so that they don't see the light jump.
			GetGame().GetCallqueue().CallLater(ChangeTime, m_fFadeOutDuration * 1000);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. The time and weather manager replicates the clock to the clients.
	protected void ChangeTime()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (manager)
			manager.SetHoursMinutesSeconds(m_iNewHour, m_iNewMinute, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void GetTravellers(int requesterId, notnull array<int> outTravellers)
	{
		outTravellers.Insert(requesterId);

		if (m_eMode == GDC_ETeleportMode.INDIVIDUAL)
			return;

		array<int> candidates = {};
		if (m_eMode == GDC_ETeleportMode.GROUP)
			candidates = GetLedGroup(requesterId).GetPlayerIDs();
		else
			GetGame().GetPlayerManager().GetPlayers(candidates);

		float maxDistanceSq = m_fRadius * m_fRadius;
		foreach (int candidateId : candidates)
		{
			if (candidateId == requesterId)
				continue;

			IEntity candidate = GetGame().GetPlayerManager().GetPlayerControlledEntity(candidateId);
			if (CanTravel(candidate) && vector.DistanceSq(candidate.GetOrigin(), GetOwner().GetOrigin()) <= maxDistanceSq)
				outTravellers.Insert(candidateId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return True for a living character on foot. Teleporting someone in a vehicle would move the vehicle.
	protected bool CanTravel(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		return character && !character.IsInVehicle() && !character.GetCharacterController().IsDead();
	}

	//------------------------------------------------------------------------------------------------
	//! \return The group this player is the leader of, null if none
	protected SCR_AIGroup GetLedGroup(int playerId)
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return null;

		SCR_AIGroup group = groupsManager.GetPlayerGroup(playerId);
		if (group && group.IsPlayerLeader(playerId))
			return group;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasRequiredRank(int playerId)
	{
		if (m_eMinRankRequired == SCR_ECharacterRank.INVALID)
			return true;

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return false;

		SCR_ECharacterRank rank = SCR_CharacterRankComponent.GetCharacterRank(character);
		return rank != SCR_ECharacterRank.INVALID && rank >= m_eMinRankRequired;
	}

	//------------------------------------------------------------------------------------------------
	protected vector GetArrivalPosition(vector destination)
	{
		vector center = destination + GetArrivalOffset(m_iNextArrivalSlot);
		m_iNextArrivalSlot = (m_iNextArrivalSlot + 1) % ARRIVAL_SLOT_COUNT;

		// The search looks for a surface downwards from there, which also works on the floor of a building.
		center[1] = destination[1] + 1;

		vector position;
		SCR_WorldTools.FindEmptyTerrainPosition(position, center, ARRIVAL_SEARCH_RADIUS);
		return position;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Offset of an arrival slot: slot 0 is the destination itself, then rings of 6, 12, 18... slots
	protected vector GetArrivalOffset(int slot)
	{
		if (slot == 0)
			return vector.Zero;

		int ring = 1;
		int firstSlot = 1;
		while (slot >= firstSlot + 6 * ring)
		{
			firstSlot += 6 * ring;
			ring++;
		}

		float angle = Math.PI2 * (slot - firstSlot) / (6 * ring);
		float radius = ring * ARRIVAL_SPACING;
		return Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
	}

#ifdef WORKBENCH

	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_SELECTED;
	}

	//------------------------------------------------------------------------------------------------
	//! While the module is selected, shows the zone of the line on the ground in the World Editor.
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!HasZone())
			return;

		const int SEGMENTS = 128;
		array<vector> points = {};
		GetZonePoints(SEGMENTS, points);

		// Slightly above the ground so that the terrain doesn't hide the shape.
		vector shapePoints[129];
		foreach (int i, vector point : points)
		{
			shapePoints[i] = point + Vector(0, 0.3, 0);
		}

		Shape.CreateLines(ARGB(255, 255, 255, 255), ShapeFlags.ONCE | ShapeFlags.NOZBUFFER, shapePoints, SEGMENTS + 1);
	}

#endif
}
