// GDC_FollowNearestPlayerEntity.c
// Path in mod: Scripts/Game/GDC/GDC_FollowNearestPlayerEntity.c

[EntityEditorProps(category: "GDC/Logic", description: "Positions this entity on the player closest to a reference entity, refreshed at a configurable interval")]
class GDC_FollowNearestPlayerEntityClass : GenericEntityClass
{
}

class GDC_FollowNearestPlayerEntity : GenericEntity
{
	[Attribute("", UIWidgets.EditBox, "Name of the reference entity used to find the nearest player. Leave empty to use this entity's own starting position as the reference point.")]
	protected string m_sReferenceEntityName;

	[Attribute("1", UIWidgets.EditBox, "Refresh interval in seconds between position updates.")]
	protected float m_fRefreshInterval;

	[Attribute("1", UIWidgets.CheckBox, "If enabled, a map marker is placed and kept updated at the detected position.")]
	protected bool m_bShowMarker;

	protected IEntity m_ReferenceEntity;
	protected ref SCR_MapMarkerBase m_Marker;

	//! Only one instance of this entity may exist in the world at a time.
	protected static GDC_FollowNearestPlayerEntity s_Instance;

	//------------------------------------------------------------------------------------------------
	//! Moves this entity onto the position of the player nearest to the reference point.
	protected void UpdatePosition()
	{
		vector referencePoint;
		if (m_ReferenceEntity)
			referencePoint = m_ReferenceEntity.GetOrigin();
		else
			referencePoint = GetOrigin();

		IEntity nearestPlayer = FindNearestPlayerEntity(referencePoint);
		if (!nearestPlayer)
			return;

		vector detectedPosition = nearestPlayer.GetOrigin();
		SetOrigin(detectedPosition);

		if (m_bShowMarker)
			AddMarker(detectedPosition);
	}

	//------------------------------------------------------------------------------------------------
	//! Places a plain static map marker at the given world position, or moves it there if already placed.
	protected void AddMarker(vector position)
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
		{
			Print("[GDC_FollowNearestPlayer] SCR_MapMarkerManagerComponent not found — is it attached to the current game mode?", LogLevel.WARNING);
			return;
		}

		int posX = (int)Math.Round(position[0]);
		int posY = (int)Math.Round(position[2]);

		if (m_Marker)
		{
			m_Marker.SetWorldPos(posX, posY);
			return;
		}

		m_Marker = new SCR_MapMarkerBase();
		m_Marker.SetType(SCR_EMapMarkerType.SIMPLE);
		m_Marker.SetWorldPos(posX, posY);
		markerManager.InsertStaticMarker(m_Marker, false, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the controlled entity of the player closest to the given point, or null if no player is connected.
	protected IEntity FindNearestPlayerEntity(vector referencePoint)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		IEntity nearestEntity;
		float nearestDistSq = -1;

		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;

			float distSq = vector.DistanceSq(playerEntity.GetOrigin(), referencePoint);
			if (nearestDistSq < 0 || distSq < nearestDistSq)
			{
				nearestDistSq = distSq;
				nearestEntity = playerEntity;
			}
		}

		return nearestEntity;
	}

	//------------------------------------------------------------------------------------------------
	void GDC_FollowNearestPlayerEntity(IEntitySource src, IEntity parent)
	{
		if (s_Instance)
		{
			Print("[GDC_FollowNearestPlayer] Only one instance of GDC_FollowNearestPlayerEntity is allowed in the world!", LogLevel.WARNING);
			delete this;
			return;
		}

		s_Instance = this;

		if (!GetGame().InPlayMode())
			return;

		if (!Replication.IsServer())
			return;

		if (m_sReferenceEntityName != "")
		{
			m_ReferenceEntity = GetGame().GetWorld().FindEntityByName(m_sReferenceEntityName);
			if (!m_ReferenceEntity)
				Print(string.Format("[GDC_FollowNearestPlayer] Reference entity '%1' not found. Using own position instead.", m_sReferenceEntityName), LogLevel.WARNING);
		}

		float refreshMs = m_fRefreshInterval * 1000;
		if (refreshMs < 100)
			refreshMs = 100;

		GetGame().GetCallqueue().CallLater(UpdatePosition, refreshMs, true);
	}
}
