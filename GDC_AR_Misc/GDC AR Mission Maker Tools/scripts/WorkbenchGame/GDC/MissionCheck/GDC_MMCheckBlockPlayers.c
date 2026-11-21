#ifdef WORKBENCH
//! Block 3, players and slots. A playable group is an SCR_AIGroup with m_bSetPlayable; its slots are its
//! m_aUnitPrefabSlots. Classes are looked up by name: no compile dependency on GDC AR Lib or Reforger Lobby.
//! - 3.1 [RL] at least one playable group when Reforger Lobby is used;
//! - 3.2 each playable group has a faction;
//! - 3.3 [GDC] a slot frequency list that is filled has one entry per slot;
//! - 3.4 [GDC] a teleport module in ALL_PLAYERS mode asks for a rank some playable slot has;
//! - 3.5 [GDC] with Coalition VON loaded, the radios a playable slot starts with are Coalition VON radios. The item
//!   prefabs are read as loaded: CVON replaces some base game radios (same GUID), which then count as CVON radios. The
//!   worn equipment is left out: a base game radio backpack serves as CVON manpack;
//! - 3.6 to 3.10, the loadout of each playable slot, read by GDC_LoadoutAnalysis (the same analysis as the Loadout
//!   Editor's): it fits in its gear (3.6), its primary weapon has a spare magazine (3.7), every prefab it uses loads
//!   (3.8), it is not heavier than the character can carry (3.9), no item aims at a storage missing from the gear
//!   (3.10, not reported when 3.8 is: missing gear explains it).
class GDC_MMCheckBlockPlayers : GDC_MMCheckBlock
{
	protected static const string GROUP = "SCR_AIGroup";
	protected static const string RANK_COMPONENT = "SCR_CharacterRankComponent";
	protected static const string TELEPORT_COMPONENT = "GDC_TeleportModuleComponent";
	protected static const ref array<string> FREQUENCY_LISTS = { "m_aGDC_SlotShortRangeFrequencies", "m_aGDC_SlotLongRangeFrequencies" };
	protected static const int TELEPORT_ALL_PLAYERS = 2; //!< GDC_ETeleportMode.ALL_PLAYERS (INDIVIDUAL, GROUP, ALL_PLAYERS)
	//! below every rank: SCR_ECharacterRank.INVALID is the LAST value of the enum (RENEGADE, PRIVATE... CUSTOM10,
	//! INVALID), not a lowest rank
	protected static const int NO_RANK = -1;
	protected static const string CVON_GUID = "6624E618B9058265";
	protected static const string RADIO_COMPONENT = "BaseRadioComponent";
	protected static const string CVON_RADIO_COMPONENT = "CVON_RadioComponent";
	protected static const string WORN_COMPONENT = "BaseLoadoutManagerComponent";
	protected static const int ITEM_OTHER = 0;
	protected static const int ITEM_CVON_RADIO = 1;
	protected static const int ITEM_OTHER_RADIO = 2;

	protected GDC_MMCheckContext m_Context;
	protected GDC_MMCheckReport m_Report;
	protected ref map<string, int> m_mPrefabRanks; //!< slot prefab -> rank (SCR_ECharacterRank), INVALID without one
	protected ref map<string, ref array<ResourceName>> m_mSlotRadios; //!< slot prefab -> radios it starts with that are not CVON's
	protected ref map<string, int> m_mItemKinds; //!< item prefab -> ITEM_*
	protected ref map<string, ref GDC_LoadoutAnalysis> m_mLoadouts; //!< slot prefab -> its loadout, null when not readable

	//------------------------------------------------------------------------------------------------
	override bool IsImplemented()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		m_Context = context;
		m_Report = report;
		m_mPrefabRanks = new map<string, int>();
		m_mSlotRadios = new map<string, ref array<ResourceName>>();
		m_mItemKinds = new map<string, int>();
		m_mLoadouts = new map<string, ref GDC_LoadoutAnalysis>();

		array<string> loaded = {};
		GameProject.GetLoadedAddons(loaded);
		bool checkRadios = loaded.Contains(CVON_GUID);

		array<IEntitySource> playable = {};
		foreach (IEntitySource entity : context.m_aEntities)
		{
			if (!GDC_MMCheckContext.Inherits(entity.GetClassName(), GROUP))
				continue;

			CheckFrequencyLists(entity);

			bool isPlayable;
			entity.Get("m_bSetPlayable", isPlayable);
			if (isPlayable)
				playable.Insert(entity);
		}

		if (playable.IsEmpty() && context.Uses(GDC_MMCheckOrigin.RL))
			report.Add("3.1", null);

		int highestRank = NO_RANK;
		foreach (IEntitySource group : playable)
		{
			string faction;
			group.Get("m_faction", faction);
			if (faction.IsEmpty())
				report.Add("3.2", group);

			array<ResourceName> slots = {};
			group.Get("m_aUnitPrefabSlots", slots);
			set<string> radioChecked = new set<string>();
			foreach (ResourceName slot : slots)
			{
				int rank = GetPrefabRank(slot);
				if (rank != SCR_ECharacterRank.INVALID && rank > highestRank)
					highestRank = rank;

				if (radioChecked.Contains(slot))
					continue;

				radioChecked.Insert(slot);
				CheckLoadout(group, slot);
				if (!checkRadios)
					continue;

				foreach (ResourceName radio : GetNonCvonRadios(slot))
				{
					report.Add("3.5", group, string.Empty, PrefabName(slot), PrefabName(radio));
				}
			}
		}

		if (context.Uses(GDC_MMCheckOrigin.GDC))
			CheckTeleportModules(highestRank);
	}

	//------------------------------------------------------------------------------------------------
	//! 3.3: only the lists the mission maker filled; an empty list keeps the default for every slot
	protected void CheckFrequencyLists(IEntitySource group)
	{
		array<ResourceName> slots = {};
		group.Get("m_aUnitPrefabSlots", slots);
		foreach (string list : FREQUENCY_LISTS)
		{
			if (group.GetVarIndex(list) < 0)
				continue; // GDC AR Lib not loaded

			array<string> frequencies = {};
			group.Get(list, frequencies);
			if (!frequencies.IsEmpty() && frequencies.Count() != slots.Count())
				m_Report.Add("3.3", group, list, frequencies.Count().ToString(), slots.Count().ToString());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 3.4: a module in ALL_PLAYERS mode with a minimum rank above the highest rank of the playable slots
	protected void CheckTeleportModules(int highestRank)
	{
		foreach (IEntitySource entity : m_Context.m_aEntities)
		{
			for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
			{
				IEntityComponentSource component = entity.GetComponent(i);
				if (!GDC_MMCheckContext.Inherits(component.GetClassName(), TELEPORT_COMPONENT))
					continue;

				int mode;
				component.Get("m_eMode", mode);
				int minRank;
				component.Get("m_eMinRankRequired", minRank);
				if (mode != TELEPORT_ALL_PLAYERS || minRank == SCR_ECharacterRank.INVALID || minRank <= highestRank)
					continue;

				string highest = GDC_MMCheckText.T("none", "aucun");
				if (highestRank != NO_RANK)
					highest = typename.EnumToString(SCR_ECharacterRank, highestRank);

				m_Report.Add("3.4", entity, string.Empty, typename.EnumToString(SCR_ECharacterRank, minRank), highest);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! 3.6 to 3.10, once per slot prefab and group
	protected void CheckLoadout(IEntitySource group, ResourceName slot)
	{
		GDC_LoadoutAnalysis loadout;
		if (!m_mLoadouts.Find(slot, loadout))
		{
			loadout = GDC_LoadoutAnalysis.ReadPrefab(slot);
			m_mLoadouts.Insert(slot, loadout);
		}

		if (!loadout)
			return;

		if (!loadout.m_aOverLimit.IsEmpty())
			m_Report.Add("3.6", group, string.Empty, PrefabName(slot), loadout.m_aOverLimit.Count().ToString());

		if (loadout.GetPrimarySpareMagazines() == 0)
			m_Report.Add("3.7", group, string.Empty, PrefabName(slot), PrefabName(loadout.GetPrimaryWeapon()));

		if (!loadout.m_aMissing.IsEmpty())
		{
			array<string> names = {};
			foreach (ResourceName missing : loadout.m_aMissing)
			{
				names.Insert(PrefabName(missing));
			}

			m_Report.Add("3.8", group, string.Empty, PrefabName(slot), string.Join(", ", names, true));
		}
		else if (!loadout.m_aNotOnGear.IsEmpty())
		{
			m_Report.Add("3.10", group, string.Empty, PrefabName(slot), loadout.m_aNotOnGear.Count().ToString());
		}

		float weight = loadout.GetTotalWeight();
		if (loadout.m_fMaxWeight > 0 && weight > loadout.m_fMaxWeight)
			m_Report.Add("3.9", group, string.Empty, PrefabName(slot), weight.ToString(-1, 1), loadout.m_fMaxWeight.ToString(-1, 0));
	}

	//------------------------------------------------------------------------------------------------
	//! \return the radios the slot prefab starts with (inventory and equipment slots, worn equipment left out) that are
	//! not Coalition VON radios, read once per prefab
	protected array<ResourceName> GetNonCvonRadios(ResourceName slot)
	{
		array<ResourceName> radios = m_mSlotRadios.Get(slot);
		if (radios)
			return radios;

		radios = {};
		m_mSlotRadios.Insert(slot, radios);
		Resource resource = Resource.Load(slot);
		if (!resource || !resource.IsValid())
			return radios;

		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source)
			return radios;

		set<string> items = new set<string>();
		for (int i = 0, count = source.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = source.GetComponent(i);
			if (!GDC_MMCheckContext.Inherits(component.GetClassName(), WORN_COMPONENT))
				CollectItems(component, string.Empty, items);
		}

		foreach (string item : items)
		{
			if (GetItemKind(item) == ITEM_OTHER_RADIO)
				radios.Insert(item);
		}

		return radios;
	}

	//------------------------------------------------------------------------------------------------
	//! Item prefabs a character starts with: PrefabsToSpawn of its initial inventory items, Prefab of its initial
	//! equipment slots (InitialStorageSlots), in the container and the objects it holds.
	//! \param list name of the object array holding the container, empty if none
	protected static void CollectItems(BaseContainer container, string list, notnull set<string> items)
	{
		for (int i = 0, count = container.GetNumVars(); i < count; i++)
		{
			string name = container.GetVarName(i);
			DataVarType type = container.GetDataVarType(i);
			if (type == DataVarType.RESOURCE_NAME_ARRAY && name == "PrefabsToSpawn")
			{
				array<ResourceName> prefabs = {};
				container.Get(name, prefabs);
				foreach (ResourceName prefab : prefabs)
				{
					if (!prefab.IsEmpty())
						items.Insert(prefab);
				}
			}
			else if (type == DataVarType.RESOURCE_NAME && name == "Prefab" && list == "InitialStorageSlots")
			{
				ResourceName prefab;
				container.Get(name, prefab);
				if (!prefab.IsEmpty())
					items.Insert(prefab);
			}
			else if (type == DataVarType.OBJECT)
			{
				BaseContainer child = container.GetObject(name);
				if (child)
					CollectItems(child, string.Empty, items);
			}
			else if (type == DataVarType.OBJECT_ARRAY)
			{
				BaseContainerList children = container.GetObjectArray(name);
				if (!children)
					continue;

				for (int c = 0, childCount = children.Count(); c < childCount; c++)
				{
					BaseContainer element = children.Get(c);
					if (element)
						CollectItems(element, name, items);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return ITEM_CVON_RADIO, ITEM_OTHER_RADIO (a radio component but no CVON one) or ITEM_OTHER, read once per prefab
	protected int GetItemKind(ResourceName item)
	{
		int kind;
		if (m_mItemKinds.Find(item, kind))
			return kind;

		kind = ITEM_OTHER;
		Resource resource = Resource.Load(item);
		if (resource && resource.IsValid())
		{
			IEntitySource source = resource.GetResource().ToEntitySource();
			if (source)
			{
				bool isRadio;
				bool isCvon;
				for (int i = 0, count = source.GetComponentCount(); i < count; i++)
				{
					string componentClass = source.GetComponent(i).GetClassName();
					if (GDC_MMCheckContext.Inherits(componentClass, RADIO_COMPONENT))
						isRadio = true;

					if (GDC_MMCheckContext.Inherits(componentClass, CVON_RADIO_COMPONENT))
						isCvon = true;
				}

				if (isCvon)
					kind = ITEM_CVON_RADIO;
				else if (isRadio)
					kind = ITEM_OTHER_RADIO;
			}
		}

		m_mItemKinds.Insert(item, kind);
		return kind;
	}

	//------------------------------------------------------------------------------------------------
	protected static string PrefabName(ResourceName prefab)
	{
		return FilePath.StripExtension(FilePath.StripPath(prefab.GetPath()));
	}

	//------------------------------------------------------------------------------------------------
	//! \return rank set on the character prefab (SCR_CharacterRankComponent.m_iRank, inherited values included), read once
	protected int GetPrefabRank(ResourceName prefab)
	{
		int rank;
		if (m_mPrefabRanks.Find(prefab, rank))
			return rank;

		rank = SCR_ECharacterRank.INVALID;
		Resource resource = Resource.Load(prefab);
		if (resource && resource.IsValid())
		{
			IEntitySource source = resource.GetResource().ToEntitySource();
			if (source)
			{
				for (int i = 0, count = source.GetComponentCount(); i < count; i++)
				{
					IEntityComponentSource component = source.GetComponent(i);
					if (GDC_MMCheckContext.Inherits(component.GetClassName(), RANK_COMPONENT))
					{
						component.Get("m_iRank", rank);
						break;
					}
				}
			}
		}

		m_mPrefabRanks.Insert(prefab, rank);
		return rank;
	}
}
#endif
