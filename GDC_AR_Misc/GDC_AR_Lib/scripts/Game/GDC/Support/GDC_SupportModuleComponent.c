// GDC_SupportModuleComponent.c
// Path in mod: Scripts/Game/GDC/Support/GDC_SupportModuleComponent.c

//! Why a support request was refused. NONE means the request is allowed.
enum GDC_ESupportRefusal
{
	NONE,
	INVALID_REQUEST,
	NO_MISSION_LEFT,
	MISSION_ACTIVE,
	COOLDOWN,
	RANK_TOO_LOW,
	MISSING_ITEM,
	TARGET_FORBIDDEN,
	OUT_OF_RANGE
}

//! Messages sent to the player who requested a support.
enum GDC_ESupportFeedback
{
	REFUSED,
	ACCEPTED,
	FIRST_ROUND,
	FINISHED
}

enum GDC_ESupportZoneMode
{
	NONE,		//!< Any target is allowed
	WHITELIST,	//!< Targets are only allowed inside the zones
	BLACKLIST	//!< Targets are allowed everywhere except inside the zones
}

//! How players designate a target on the map.
enum GDC_ESupportTargetingMode
{
	ARCADE,	//!< Click on the map, with a marker and the area of effect shown on it
	MILSIM	//!< Coordinates typed by hand, nothing shown on the map
}

class GDC_SupportModuleComponentClass : ScriptComponentClass {}

//! Base class of support modules: access, usage count, cooldown, target areas and request life cycle.
//! The owner entity needs an RplComponent. Child classes implement StartMission().
class GDC_SupportModuleComponent : ScriptComponent
{
	[Attribute("Support", UIWidgets.EditBox, "Name shown to players.")]
	protected string m_sDisplayName;

	[Attribute(typename.EnumToString(GDC_ESupportTargetingMode, GDC_ESupportTargetingMode.ARCADE), UIWidgets.ComboBox, "ARCADE: the player clicks on the map, a marker and the area of effect are shown. MILSIM: the player types the coordinates, nothing is shown on the map.", enumType: GDC_ESupportTargetingMode)]
	protected GDC_ESupportTargetingMode m_eTargetingMode;

	[Attribute("-1", UIWidgets.EditBox, "Number of fire missions available to all players. -1 = unlimited.", "-1 inf 1")]
	protected int m_iFireMissionCount;

	[Attribute("120", UIWidgets.EditBox, "Delay in seconds between the end of a mission and the next possible request.", "0 inf 1")]
	protected float m_fCooldown;

	[Attribute(typename.EnumToString(SCR_ECharacterRank, SCR_ECharacterRank.INVALID), UIWidgets.ComboBox, "Minimum rank required. INVALID = no rank requirement. If a required item is also set, both conditions apply.", enumType: SCR_ECharacterRank)]
	protected SCR_ECharacterRank m_eMinRankRequired;

	[Attribute("", UIWidgets.ResourceNamePicker, "The player must carry one of these items (exact prefab, inherited prefabs don't count). Empty = no item requirement. If a minimum rank is also set, both conditions apply.", "et")]
	protected ref array<ResourceName> m_aRequiredItems;

	[Attribute(typename.EnumToString(GDC_ESupportZoneMode, GDC_ESupportZoneMode.NONE), UIWidgets.ComboBox, "How the areas restrict targets. NONE: any target. WHITELIST: targets only inside the areas. BLACKLIST: no-fire areas, targets anywhere except inside them.", enumType: GDC_ESupportZoneMode)]
	protected GDC_ESupportZoneMode m_eFireAreaMode;

	[Attribute("", UIWidgets.EditBox, "Names of the closed polyline entities used as areas.")]
	protected ref array<string> m_aAreaNames;

	[RplProp()]
	protected int m_iMissionsLeft;

	[RplProp()]
	protected bool m_bMissionActive;

	[RplProp()]
	protected WorldTimestamp m_fCooldownEnd;

	//! Area polygons in world coordinates, built on first use.
	protected ref array<ref array<vector>> m_aZonePolygons;

	protected RplComponent m_RplComponent;

	//! Player who requested the current mission. Authority only.
	protected int m_iRequesterId;

	protected static ref array<GDC_SupportModuleComponent> s_aModules = {};

	//------------------------------------------------------------------------------------------------
	//! \return All the support modules of the current world
	static array<GDC_SupportModuleComponent> GetModules()
	{
		return s_aModules;
	}

	//------------------------------------------------------------------------------------------------
	//! Fills the modules a character has access to (rank and items).
	//! Without access, a support doesn't exist for the player: it is neither listed nor mentioned.
	static void GetAccessibleModules(IEntity character, notnull array<GDC_SupportModuleComponent> modules)
	{
		modules.Clear();
		foreach (GDC_SupportModuleComponent module : s_aModules)
		{
			if (module.CheckAccess(character) == GDC_ESupportRefusal.NONE)
				modules.Insert(module);
		}
	}

	//------------------------------------------------------------------------------------------------
	string GetDisplayName()
	{
		return m_sDisplayName;
	}

	//------------------------------------------------------------------------------------------------
	GDC_ESupportTargetingMode GetTargetingMode()
	{
		return m_eTargetingMode;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Number of missions left, -1 if unlimited
	int GetMissionsLeft()
	{
		return m_iMissionsLeft;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Remaining cooldown in seconds, 0 if none
	float GetCooldownRemaining()
	{
		ChimeraWorld world = GetGame().GetWorld();
		WorldTimestamp now = world.GetServerTimestamp();
		if (!m_fCooldownEnd.Greater(now))
			return 0;

		return m_fCooldownEnd.DiffSeconds(now);
	}

	//------------------------------------------------------------------------------------------------
	//! \return Whether this machine has authority over this module
	bool IsAuthority()
	{
		return m_RplComponent && m_RplComponent.Role() == RplRole.Authority;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks the state of the module, regardless of who is asking. Valid on server and clients.
	GDC_ESupportRefusal CheckAvailability()
	{
		if (m_iMissionsLeft == 0)
			return GDC_ESupportRefusal.NO_MISSION_LEFT;

		if (m_bMissionActive)
			return GDC_ESupportRefusal.MISSION_ACTIVE;

		if (GetCooldownRemaining() > 0)
			return GDC_ESupportRefusal.COOLDOWN;

		return GDC_ESupportRefusal.NONE;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks the rank and item requirements for a character. Valid on server and clients.
	GDC_ESupportRefusal CheckAccess(IEntity character)
	{
		if (!character)
			return GDC_ESupportRefusal.INVALID_REQUEST;

		if (m_eMinRankRequired != SCR_ECharacterRank.INVALID)
		{
			SCR_ECharacterRank rank = SCR_CharacterRankComponent.GetCharacterRank(character);
			if (rank == SCR_ECharacterRank.INVALID || rank < m_eMinRankRequired)
				return GDC_ESupportRefusal.RANK_TOO_LOW;
		}

		if (m_aRequiredItems && !m_aRequiredItems.IsEmpty() && !CarriesRequiredItem(character))
			return GDC_ESupportRefusal.MISSING_ITEM;

		return GDC_ESupportRefusal.NONE;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CarriesRequiredItem(IEntity character)
	{
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		SCR_PrefabNamePredicate predicate = new SCR_PrefabNamePredicate();
		foreach (ResourceName item : m_aRequiredItems)
		{
			predicate.prefabName = item;
			if (inventory.FindItem(predicate, EStoragePurpose.PURPOSE_ANY))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks the areas for a target position. Valid on server and clients.
	bool IsTargetAllowed(vector target)
	{
		if (m_eFireAreaMode == GDC_ESupportZoneMode.NONE)
			return true;

		if (!m_aZonePolygons)
			BuildZonePolygons();

		bool inZone = false;
		foreach (array<vector> polygon : m_aZonePolygons)
		{
			if (Math2D.IsPointInPolygonXZ(polygon, target))
			{
				inZone = true;
				break;
			}
		}

		if (m_eFireAreaMode == GDC_ESupportZoneMode.WHITELIST)
			return inZone;

		return !inZone;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildZonePolygons()
	{
		m_aZonePolygons = {};

		if (!m_aAreaNames)
			return;

		BaseWorld world = GetGame().GetWorld();
		foreach (string zoneName : m_aAreaNames)
		{
			PolylineShapeEntity shape = PolylineShapeEntity.Cast(world.FindEntityByName(zoneName));
			if (!shape)
			{
				Print(string.Format("[GDC_Support] '%1': area '%2' is not a polyline entity of this world.", m_sDisplayName, zoneName), LogLevel.WARNING);
				continue;
			}

			array<vector> polygon = {};
			shape.GetPointsPositions(polygon);
			for (int i = 0, count = polygon.Count(); i < count; i++)
			{
				polygon[i] = shape.CoordToParent(polygon[i]);
			}

			m_aZonePolygons.Insert(polygon);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Full check of a request. Valid on server and clients.
	//! \param character Character of the requesting player
	//! \param target Target position
	//! \param option Index of the option chosen by the player (ammo type...)
	//! \param quantity Quantity chosen by the player (number of rounds...)
	//! \param spread Spread chosen by the player, in metres
	GDC_ESupportRefusal CheckRequest(IEntity character, vector target, int option, int quantity, int spread)
	{
		GDC_ESupportRefusal refusal = CheckAvailability();
		if (refusal != GDC_ESupportRefusal.NONE)
			return refusal;

		refusal = CheckAccess(character);
		if (refusal != GDC_ESupportRefusal.NONE)
			return refusal;

		if (!IsTargetAllowed(target))
			return GDC_ESupportRefusal.TARGET_FORBIDDEN;

		if (!IsOptionValid(option) || quantity < 1 || quantity > GetMaxQuantity() || spread < GetMinSpread() || spread > GetMaxSpread())
			return GDC_ESupportRefusal.INVALID_REQUEST;

		if (!IsTargetInRange(target, option))
			return GDC_ESupportRefusal.OUT_OF_RANGE;

		return GDC_ESupportRefusal.NONE;
	}

	//------------------------------------------------------------------------------------------------
	//! Valid on server and clients.
	//! \param option Index of the option chosen by the player (ammo type...)
	//! \return Whether this support can reach the target
	bool IsTargetInRange(vector target, int option)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Validates a request from a player and starts the mission. Authority only.
	//! A mission is only consumed once all the checks have passed and the mission has started.
	//! The player is told about the outcome, and later about the progress of the mission.
	//! \return NONE if the mission has started, otherwise the reason of the refusal
	GDC_ESupportRefusal Request(int playerId, vector target, int option, int quantity, int spread)
	{
		if (!IsAuthority())
			return GDC_ESupportRefusal.INVALID_REQUEST;

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		GDC_ESupportRefusal refusal = CheckRequest(character, target, option, quantity, spread);
		if (refusal == GDC_ESupportRefusal.NONE && !StartMission(target, option, quantity, spread))
			refusal = GDC_ESupportRefusal.INVALID_REQUEST;

		Print(string.Format("[GDC_Support] '%1': request from player %2 on %3: %4", m_sDisplayName, playerId, target, typename.EnumToString(GDC_ESupportRefusal, refusal)));

		if (refusal != GDC_ESupportRefusal.NONE)
		{
			SendFeedback(playerId, GDC_ESupportFeedback.REFUSED, refusal, GetCooldownRemaining());
			return refusal;
		}

		#ifdef DEBUG
		Print(string.Format("[GDC_Support] '%1': mission started, %2 rounds, spread %3 m, option %4", m_sDisplayName, quantity, spread, option));
		#endif

		m_iRequesterId = playerId;
		m_bMissionActive = true;
		if (m_iMissionsLeft > 0)
			m_iMissionsLeft--;

		Replication.BumpMe();
		SendFeedback(playerId, GDC_ESupportFeedback.ACCEPTED, GDC_ESupportRefusal.NONE, GetStartDelay());
		return GDC_ESupportRefusal.NONE;
	}

	//------------------------------------------------------------------------------------------------
	//! Tells a player about a request. The player may have left in the meantime. Authority only.
	protected void SendFeedback(int playerId, GDC_ESupportFeedback feedback, GDC_ESupportRefusal refusal, float seconds)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (controller)
			controller.GDC_SendSupportFeedback(m_sDisplayName, feedback, refusal, Math.Ceil(seconds));
	}

	//------------------------------------------------------------------------------------------------
	//! \return Delay in seconds between an accepted request and the first effect of the mission
	protected float GetStartDelay()
	{
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Fills the names of the options players can choose from. Empty if there is no choice to make.
	void GetOptionNames(notnull array<string> names)
	{
		names.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! \return The largest quantity a player can ask for. 1 means there is no choice to make.
	int GetMaxQuantity()
	{
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	//! \return The smallest spread a player can ask for, in metres
	int GetMinSpread()
	{
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! \return The largest spread a player can ask for, in metres. Equal to the minimum, there is no choice to make.
	int GetMaxSpread()
	{
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Whether this option index exists for this support
	protected bool IsOptionValid(int option)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Starts the support itself. Must lead to a call to OnMissionFinished(). Authority only.
	//! \return false if the mission could not be started
	protected bool StartMission(vector target, int option, int quantity, int spread)
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Ends the current mission and starts the cooldown. Authority only.
	protected void OnMissionFinished()
	{
		ChimeraWorld world = GetGame().GetWorld();
		#ifdef DEBUG
		Print(string.Format("[GDC_Support] '%1': mission finished, cooldown %2 s, missions left %3", m_sDisplayName, m_fCooldown, m_iMissionsLeft));
		#endif

		m_bMissionActive = false;
		m_fCooldownEnd = world.GetServerTimestamp().PlusSeconds(m_fCooldown);
		Replication.BumpMe();
		SendFeedback(m_iRequesterId, GDC_ESupportFeedback.FINISHED, GDC_ESupportRefusal.NONE, 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;

		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!m_RplComponent)
			Print(string.Format("[GDC_Support] '%1': the owner entity has no RplComponent.", m_sDisplayName), LogLevel.WARNING);

		m_iMissionsLeft = m_iFireMissionCount;
		s_aModules.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		s_aModules.RemoveItem(this);
	}
}
