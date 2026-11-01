// GDC_TeleportModuleComponent.c
// Path in mod: Scripts/Game/GDC/Teleport/GDC_TeleportModuleComponent.c

//! Who can use a teleport module, and who travels.
enum GDC_ETeleportMode
{
	INDIVIDUAL,	//!< Any player can use it and travels alone
	GROUP		//!< Only a group leader can use it, and takes along the members of the group gathered around the module
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

	[Attribute(typename.EnumToString(GDC_ETeleportMode, GDC_ETeleportMode.INDIVIDUAL), UIWidgets.ComboBox, "INDIVIDUAL: any player can use it and travels alone. GROUP: only a group leader can use it, and takes along the group members gathered around the module.", enumType: GDC_ETeleportMode)]
	protected GDC_ETeleportMode m_eMode;

	[Attribute("30", UIWidgets.EditBox, "GROUP mode: group members further than this from the module, in metres, stay behind.", "1 1000 1")]
	protected float m_fGroupRadius;

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

	//! Distance around the module from which a request is accepted, in metres. Wider than the action radius to allow for latency.
	protected const float MAX_USE_DISTANCE = 10;

	//! Arrival points are laid out in rings around the destination.
	protected const float ARRIVAL_SPACING = 2;
	protected const float ARRIVAL_SEARCH_RADIUS = 1.5;
	protected const int ARRIVAL_SLOT_COUNT = 37;

	//! Server only. Successive travellers don't land on each other, even when they come from separate requests.
	protected int m_iNextArrivalSlot;

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
	//! \return True if this player can use the module: only group leaders in GROUP mode. Valid on server and clients.
	bool IsAllowed(int playerId)
	{
		if (m_eMode != GDC_ETeleportMode.GROUP)
			return true;

		return GetLedGroup(playerId) != null;
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
	}

	//------------------------------------------------------------------------------------------------
	protected void GetTravellers(int requesterId, notnull array<int> outTravellers)
	{
		outTravellers.Insert(requesterId);

		if (m_eMode != GDC_ETeleportMode.GROUP)
			return;

		float maxDistanceSq = m_fGroupRadius * m_fGroupRadius;
		foreach (int memberId : GetLedGroup(requesterId).GetPlayerIDs())
		{
			if (memberId == requesterId)
				continue;

			IEntity member = GetGame().GetPlayerManager().GetPlayerControlledEntity(memberId);
			if (CanTravel(member) && vector.DistanceSq(member.GetOrigin(), GetOwner().GetOrigin()) <= maxDistanceSq)
				outTravellers.Insert(memberId);
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
}
