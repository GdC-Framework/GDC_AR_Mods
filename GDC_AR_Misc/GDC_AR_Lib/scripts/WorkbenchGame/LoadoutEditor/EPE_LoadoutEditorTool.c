#ifdef WORKBENCH
[WorkbenchToolAttribute(
	name: "ECHO Loadout Editor",
	description: "Edit the loadout of a character prefab: the one open with Edit Prefab, the character selected in the world, or the one picked in Base Character.",
	awesomeFontCode: 0xF553,
	wbModules: { "WorldEditor" })]
class EPE_LoadoutEditorTool : WorldEditorTool
{
	[Attribute(desc: "Character prefab being edited. Taken from the prefab open with Edit Prefab, or from the character selected in the world. With a world open you can also pick a prefab here and press Load From Prefab.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Base")]
	protected ResourceName m_BaseCharacter;

	[Attribute(desc: "Headgear", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_Hat;

	[Attribute(desc: "Jacket or shirt", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_Jacket;

	[Attribute(desc: "Load-bearing vest", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_Vest;

	[Attribute(desc: "Body armour", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_ArmoredVest;

	[Attribute(desc: "Backpack", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_Back;

	[Attribute(desc: "Gloves", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_HandWear;

	[Attribute(desc: "Trousers", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_Pants;

	[Attribute(desc: "Footwear", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Worn")]
	protected ResourceName m_Boots;

	[Attribute(desc: "Primary weapon, right shoulder (weapon slot 0)", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Weapons")]
	protected ResourceName m_PrimaryWeapon;

	[Attribute(desc: "Launcher or second long weapon, left shoulder (weapon slot 1)", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Weapons")]
	protected ResourceName m_Launcher;

	[Attribute(desc: "Sidearm, holstered pistol (weapon slot 2)", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Weapons")]
	protected ResourceName m_Sidearm;

	[Attribute(desc: "Grenade (weapon slot 3)", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Weapons")]
	protected ResourceName m_Grenade;

	[Attribute(desc: "Throwable, e.g. smoke (weapon slot 4)", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Weapons")]
	protected ResourceName m_Throwable;

	[Attribute(desc: "Slots the character itself carries, taken from the prefab. A flashlight or radio is not one of these - place it as an item in the Items groups.", category: "Gadgets")]
	protected ref array<ref EPE_LoadoutGadgetEntry> m_aGadgetSlots;

	[Attribute(desc: "What the character carries, one group per worn item. Press 'Load From Prefab' to fill this in.", category: "Items")]
	protected ref array<ref EPE_InventoryGroup> m_aGroups;

	protected ref array<string> m_aWarnings;
	protected ref EPE_LoadoutDefinition m_LoadedDefinition;
	protected ref array<ref EPE_StorageNode> m_aNodeCache;
	protected string m_sNodeCacheKey;
	protected ref Resource m_TargetResource;
	protected ResourceName m_TargetResourceName;

	override void OnActivate()
	{
		ResourceName found = FindCharacterInEditor();
		if (found.IsEmpty())
		{
			if (!m_LoadedDefinition && !m_BaseCharacter.IsEmpty())
				LoadCharacter();

			return;
		}

		if (found != m_BaseCharacter || !m_LoadedDefinition)
			LoadFromPrefab();
	}

	protected ResourceName FindCharacterInEditor()
	{
		IEntitySource opened = SCR_WorldEditorToolHelper.GetPrefabEditModeEntitySource();
		if (opened)
			return CharacterPrefabOf(opened);

		return CharacterPrefabOf(FindSelectedCharacter());
	}

	protected IEntitySource FindSelectedCharacter()
	{
		if (!m_API)
			return null;

		for (int i = 0, count = m_API.GetSelectedEntitiesCount(); i < count; i++)
		{
			IEntitySource selected = m_API.GetSelectedEntity(i);
			if (!CharacterPrefabOf(selected).IsEmpty())
				return selected;
		}

		return null;
	}

	protected static ResourceName CharacterPrefabOf(IEntitySource entitySource)
	{
		if (!entitySource || !SCR_BaseContainerTools.FindComponentSource(entitySource, BaseLoadoutManagerComponent))
			return ResourceName.Empty;

		BaseContainer prefab = entitySource.GetAncestor();
		if (!prefab)
			return ResourceName.Empty;

		return prefab.GetResourceName();
	}

	protected ResourceName PickCharacter()
	{
		IEntitySource opened = SCR_WorldEditorToolHelper.GetPrefabEditModeEntitySource();
		if (opened)
			return CharacterPrefabOf(opened);

		if (m_LoadedDefinition && !m_BaseCharacter.IsEmpty() && m_BaseCharacter != m_LoadedDefinition.m_BaseCharacter)
			return m_BaseCharacter;

		ResourceName selected = CharacterPrefabOf(FindSelectedCharacter());
		if (!selected.IsEmpty())
			return selected;

		return m_BaseCharacter;
	}

	protected bool NeedsLoad()
	{
		return !m_LoadedDefinition || m_LoadedDefinition.m_BaseCharacter != m_BaseCharacter;
	}

	protected IEntitySource GetEditedPrefabSource()
	{
		IEntitySource opened = SCR_WorldEditorToolHelper.GetPrefabEditModeEntitySource();
		if (opened)
			return GetOpenPrefabSource(opened);

		return GetPickedPrefabSource();
	}

	protected IEntitySource GetOpenPrefabSource(notnull IEntitySource opened)
	{
		if (!SCR_BaseContainerTools.FindComponentSource(opened, BaseLoadoutManagerComponent))
		{
			Print("EPE: the open prefab has no loadout manager, so it is not a character this tool can edit.", LogLevel.WARNING);
			return null;
		}

		IEntitySource actualPrefab = IEntitySource.Cast(opened.GetAncestor());
		if (!actualPrefab)
		{
			Print("EPE: the open entity has no prefab behind it, nothing to edit", LogLevel.ERROR);
			return null;
		}

		ResourceName openedPrefab = actualPrefab.GetResourceName();
		if (m_BaseCharacter != openedPrefab)
		{
			m_BaseCharacter = openedPrefab;
			InvalidateDestinations();
			UpdatePropertyPanel();
		}

		return actualPrefab;
	}

	protected IEntitySource GetPickedPrefabSource()
	{
		if (m_BaseCharacter.IsEmpty())
		{
			Print("EPE: select a character in the world or pick one in Base Character, then press Load From Prefab. A character prefab opened with Edit Prefab works too.", LogLevel.WARNING);
			return null;
		}

		if (!m_TargetResource || m_TargetResourceName != m_BaseCharacter)
		{
			m_TargetResource = Resource.Load(m_BaseCharacter);
			m_TargetResourceName = m_BaseCharacter;
		}

		if (!m_TargetResource || !m_TargetResource.IsValid())
		{
			PrintFormat("EPE: cannot load %1", m_BaseCharacter, level: LogLevel.ERROR);
			m_TargetResource = null;
			return null;
		}

		IEntitySource prefab = m_TargetResource.GetResource().ToEntitySource();
		if (!prefab || !SCR_BaseContainerTools.FindComponentSource(prefab, BaseLoadoutManagerComponent))
		{
			PrintFormat("EPE: %1 has no loadout manager, so it is not a character this tool can edit. A character placed inside a composition is edited through its own prefab.", m_BaseCharacter, level: LogLevel.WARNING);
			return null;
		}

		return prefab;
	}

	protected ResourceName GetWornField(string slotName)
	{
		switch (slotName)
		{
			case EPE_LoadoutSlotNames.HAT: return m_Hat;
			case EPE_LoadoutSlotNames.JACKET: return m_Jacket;
			case EPE_LoadoutSlotNames.VEST: return m_Vest;
			case EPE_LoadoutSlotNames.ARMORED_VEST: return m_ArmoredVest;
			case EPE_LoadoutSlotNames.BACK: return m_Back;
			case EPE_LoadoutSlotNames.HANDWEAR: return m_HandWear;
			case EPE_LoadoutSlotNames.PANTS: return m_Pants;
			case EPE_LoadoutSlotNames.BOOTS: return m_Boots;
		}

		return ResourceName.Empty;
	}

	protected void SetWornField(string slotName, ResourceName prefab)
	{
		switch (slotName)
		{
			case EPE_LoadoutSlotNames.HAT: m_Hat = prefab; break;
			case EPE_LoadoutSlotNames.JACKET: m_Jacket = prefab; break;
			case EPE_LoadoutSlotNames.VEST: m_Vest = prefab; break;
			case EPE_LoadoutSlotNames.ARMORED_VEST: m_ArmoredVest = prefab; break;
			case EPE_LoadoutSlotNames.BACK: m_Back = prefab; break;
			case EPE_LoadoutSlotNames.HANDWEAR: m_HandWear = prefab; break;
			case EPE_LoadoutSlotNames.PANTS: m_Pants = prefab; break;
			case EPE_LoadoutSlotNames.BOOTS: m_Boots = prefab; break;
		}
	}

	protected EPE_LoadoutDefinition BuildDefinition()
	{
		EPE_LoadoutDefinition loadout = new EPE_LoadoutDefinition();
		loadout.m_BaseCharacter = m_BaseCharacter;

		foreach (string slotName : EPE_LoadoutSlotNames.GetAll())
		{
			loadout.m_aWorn.Insert(new EPE_LoadoutWornEntry(slotName, GetWornField(slotName)));
		}

		loadout.m_aWeapons.Insert(new EPE_LoadoutWeaponEntry(m_PrimaryWeapon, 0));
		loadout.m_aWeapons.Insert(new EPE_LoadoutWeaponEntry(m_Launcher, 1));
		loadout.m_aWeapons.Insert(new EPE_LoadoutWeaponEntry(m_Sidearm, 2));

		loadout.m_aGrenades.Insert(new EPE_LoadoutWeaponEntry(m_Grenade, 3));
		loadout.m_aGrenades.Insert(new EPE_LoadoutWeaponEntry(m_Throwable, 4));

		if (m_aGadgetSlots)
		{
			foreach (EPE_LoadoutGadgetEntry gadget : m_aGadgetSlots)
			{
				if (gadget && !gadget.m_sSlotName.IsEmpty())
					loadout.m_aGadgets.Insert(new EPE_LoadoutGadgetEntry(gadget.m_sSlotName, gadget.m_Prefab));
			}
		}

		ApplyItems(loadout);
		return loadout;
	}

	protected void ApplyItems(notnull EPE_LoadoutDefinition loadout)
	{
		if (!m_aWarnings)
			m_aWarnings = {};

		m_aWarnings.Clear();

		if (!m_aGroups)
			m_aGroups = {};

		array<ref EPE_StorageNode> nodes = {};
		CollectDestinations(nodes);

		EPE_ItemMetrics metrics = CreateMetrics();
		EPE_InventoryGroups.Convert(m_aGroups, nodes, metrics, loadout.m_aStorages);
		EPE_InventoryDiagnostics.Describe(m_aGroups, loadout.m_aStorages, nodes, metrics, m_aWarnings);
	}

	protected string BuildWarningReport()
	{
		if (!m_aWarnings || m_aWarnings.IsEmpty())
			return string.Empty;

		string report = "WARNINGS\n";
		foreach (string line : m_aWarnings)
		{
			report += string.Format("\n%1\n", line);
		}

		return report;
	}

	protected void CollectDestinations(notnull array<ref EPE_StorageNode> outNodes)
	{
		string gearKey;
		foreach (string slotName : EPE_LoadoutSlotNames.GetStorageCandidates())
		{
			gearKey += string.Format("%1=%2;", slotName, GetWornField(slotName));
		}

		if (!m_aNodeCache || m_sNodeCacheKey != gearKey)
		{
			m_aNodeCache = {};
			m_sNodeCacheKey = gearKey;
			foreach (string slotName : EPE_LoadoutSlotNames.GetStorageCandidates())
			{
				EPE_StoragePathResolver.CollectPathsForWornItem(slotName, GetWornField(slotName), m_aNodeCache);
			}
		}

		foreach (EPE_StorageNode node : m_aNodeCache)
		{
			outNodes.Insert(node);
		}
	}

	protected void InvalidateDestinations()
	{
		m_aNodeCache = null;
		m_sNodeCacheKey = string.Empty;
	}

	protected void CollectWornSlots(notnull array<string> outWorn)
	{
		foreach (string slotName : EPE_LoadoutSlotNames.GetStorageCandidates())
		{
			if (!GetWornField(slotName).IsEmpty())
				outWorn.Insert(slotName);
		}
	}

	protected void ReconcileWithWornGear()
	{
		if (!m_aGroups)
			m_aGroups = {};

		array<string> worn = {};
		CollectWornSlots(worn);
		EPE_InventoryGroups.DropRemovedGear(m_aGroups, worn);
	}

	protected EPE_ItemMetrics CreateMetrics()
	{
		return new EPE_ItemMetrics();
	}

	[ButtonAttribute("Load From Prefab")]
	void LoadFromPrefab()
	{
		ResourceName character = PickCharacter();
		if (!character.IsEmpty() && character != m_BaseCharacter)
		{
			m_BaseCharacter = character;
			UpdatePropertyPanel();
		}

		LoadCharacter();
	}

	protected void LoadCharacter()
	{
		if (!GetEditedPrefabSource())
			return;

		InvalidateDestinations();

		EPE_LoadoutDefinition loadout = new EPE_LoadoutDefinition();
		if (!EPE_PrefabWriter.ReadLoadout(m_BaseCharacter, loadout))
			return;

		foreach (string slotName : EPE_LoadoutSlotNames.GetAll())
		{
			EPE_LoadoutWornEntry worn = loadout.FindWorn(slotName);
			if (worn)
				SetWornField(slotName, worn.m_Prefab);
			else
				SetWornField(slotName, ResourceName.Empty);
		}

		m_PrimaryWeapon = ResourceName.Empty;
		m_Launcher = ResourceName.Empty;
		m_Sidearm = ResourceName.Empty;
		foreach (EPE_LoadoutWeaponEntry weapon : loadout.m_aWeapons)
		{
			if (!weapon)
				continue;

			if (weapon.m_iSlotIndex == 0)
				m_PrimaryWeapon = weapon.m_Prefab;
			else if (weapon.m_iSlotIndex == 1)
				m_Launcher = weapon.m_Prefab;
			else if (weapon.m_iSlotIndex == 2)
				m_Sidearm = weapon.m_Prefab;
		}

		m_Grenade = ResourceName.Empty;
		m_Throwable = ResourceName.Empty;
		foreach (EPE_LoadoutWeaponEntry grenade : loadout.m_aGrenades)
		{
			if (!grenade)
				continue;

			if (grenade.m_iSlotIndex == 3)
				m_Grenade = grenade.m_Prefab;
			else if (grenade.m_iSlotIndex == 4)
				m_Throwable = grenade.m_Prefab;
		}

		m_aGadgetSlots = {};
		foreach (EPE_LoadoutGadgetEntry gadget : loadout.m_aGadgets)
		{
			if (gadget && !gadget.m_sSlotName.IsEmpty())
				m_aGadgetSlots.Insert(new EPE_LoadoutGadgetEntry(gadget.m_sSlotName, gadget.m_Prefab));
		}

		RebuildGroups(loadout);
		m_LoadedDefinition = loadout;

		UpdatePropertyPanel();

		PrintFormat("EPE: loaded from %1", m_BaseCharacter);
	}

	[ButtonAttribute("Refresh Containers")]
	void RefreshContainers()
	{
		if (!m_aGroups)
			m_aGroups = {};

		InvalidateDestinations();
		ReconcileWithWornGear();

		array<ref EPE_StorageNode> nodes = {};
		CollectDestinations(nodes);

		EPE_InventoryGroups.Remap(m_aGroups, nodes);

		EPE_LoadoutDefinition preview = new EPE_LoadoutDefinition();
		ApplyItems(preview);
		UpdatePropertyPanel();

		string warningReport = BuildWarningReport();
		if (warningReport.IsEmpty())
			Print("EPE: containers refreshed for the gear selected in the panel.");
		else
			Print(warningReport, LogLevel.WARNING);
	}

	[ButtonAttribute("Redistribute Items")]
	void RedistributeItems()
	{
		if (!m_aGroups)
			m_aGroups = {};

		if (!m_aWarnings)
			m_aWarnings = {};

		array<ref EPE_StorageNode> nodes = {};
		CollectDestinations(nodes);

		EPE_ItemMetrics metrics = CreateMetrics();
		EPE_LoadoutDefinition repacked = new EPE_LoadoutDefinition();
		EPE_InventoryGroups.Convert(m_aGroups, nodes, metrics, repacked.m_aStorages, true);

		EPE_InventoryGroups.Build(repacked.m_aStorages, nodes, m_aGroups);
		EPE_InventoryDiagnostics.Describe(m_aGroups, repacked.m_aStorages, nodes, metrics, m_aWarnings);
		UpdatePropertyPanel();

		string warningReport = BuildWarningReport();
		if (warningReport.IsEmpty())
			Print("EPE: items redistributed across the containers of each group. Apply to write them to the prefab.");
		else
			Print(warningReport, LogLevel.WARNING);
	}

	protected void RebuildGroups(notnull EPE_LoadoutDefinition loadout)
	{
		if (!m_aGroups)
			m_aGroups = {};

		if (!m_aWarnings)
			m_aWarnings = {};

		array<ref EPE_StorageNode> nodes = {};
		CollectDestinations(nodes);

		EPE_InventoryGroups.Build(loadout.m_aStorages, nodes, m_aGroups);
		EPE_InventoryDiagnostics.Describe(m_aGroups, loadout.m_aStorages, nodes, CreateMetrics(), m_aWarnings);
	}

	[ButtonAttribute("List Destinations")]
	void ListDestinations()
	{
		if (!GetEditedPrefabSource())
			return;
		if (NeedsLoad())
			LoadCharacter();
		if (NeedsLoad())
			return;

		ReconcileWithWornGear();
		BuildDefinition();

		string report = BuildWarningReport();
		if (!report.IsEmpty())
			report += "\n";

		foreach (EPE_InventoryGroup group : m_aGroups)
		{
			if (!group)
				continue;

			report += string.Format("\n%1\n%2", group.m_sSummary, group.m_sDetails);
		}

		report += BuildWornSubSlotReport();
		report += BuildWeaponSlotReport();
		report += BuildInheritanceReport();

		EPE_LoadoutReportDialog dialog = new EPE_LoadoutReportDialog();
		dialog.SetReport(report);
		Workbench.ScriptDialog("ECHO Loadout - item destinations", report, dialog);
	}

	protected string BuildWeaponSlotReport()
	{
		IEntitySource prefab = GetEditedPrefabSource();
		if (!prefab)
			return string.Empty;

		string report = "\n\nWEAPON SLOTS\n";
		report += EPE_PrefabWriter.DescribeWeaponSlots(prefab);
		report += "\nGADGET SLOTS\n";
		report += EPE_PrefabWriter.DescribeGadgetSlots(prefab);
		return report;
	}

	protected string BuildInheritanceReport()
	{
		IEntitySource prefab = GetEditedPrefabSource();
		if (!prefab)
			return string.Empty;

		string report = "\n\nARRAY INHERITANCE\n";

		IEntityComponentSource storageManager = SCR_BaseContainerTools.FindComponentSource(prefab, SCR_InventoryStorageManagerComponent);
		report += "\n  InitialInventoryItems\n";
		if (storageManager)
			report += EPE_StoragePathResolver.DescribeArrayInheritance(storageManager, "InitialInventoryItems");
		else
			report += "    none\n";

		IEntityComponentSource loadoutManager = SCR_BaseContainerTools.FindComponentSource(prefab, BaseLoadoutManagerComponent);
		report += "\n  Slots\n";
		if (loadoutManager)
			report += EPE_StoragePathResolver.DescribeArrayInheritance(loadoutManager, "Slots");
		else
			report += "    none\n";

		return report;
	}

	protected string BuildWornSubSlotReport()
	{
		string report;

		foreach (string slotName : EPE_LoadoutSlotNames.GetAll())
		{
			ResourceName wornPrefab = GetWornField(slotName);
			if (wornPrefab.IsEmpty())
				continue;

			Resource wornResource = Resource.Load(wornPrefab);
			if (!wornResource || !wornResource.IsValid())
				continue;

			IEntityComponentSource cloth = SCR_BaseContainerTools.FindComponentSource(wornResource, BaseLoadoutClothComponent);
			if (!cloth)
				continue;

			map<string, BaseContainer> subSlots = new map<string, BaseContainer>();
			EPE_StoragePathResolver.GatherSlots(cloth, subSlots);
			if (subSlots.IsEmpty())
				continue;

			report += string.Format("\n%1 - %2\n", slotName, FilePath.StripPath(wornPrefab.GetPath()));

			foreach (string subName, BaseContainer subSlot : subSlots)
			{
				ResourceName fitted = EPE_StoragePathResolver.ResolvePrefab(subSlot);
				if (fitted.IsEmpty())
					report += string.Format("      %1: empty\n", subName);
				else
					report += string.Format("      %1: %2\n", subName, FilePath.StripPath(fitted.GetPath()));
			}
		}

		if (report.IsEmpty())
			return string.Empty;

		return "\n\nATTACHMENT SLOTS ON WORN GEAR (fitted by the item, not addressable here)\n" + report;
	}

	[ButtonAttribute("Apply")]
	void Apply()
	{
		if (!ApplyToEditedCharacter())
			return;

		IEntitySource actualPrefab = GetEditedPrefabSource();
		if (!actualPrefab)
			return;

		if (m_API.SaveEntityTemplate(actualPrefab))
			PrintFormat("EPE: applied and saved %1", m_BaseCharacter);
		else
			PrintFormat("EPE: applied to %1 but saving the file failed", m_BaseCharacter, level: LogLevel.ERROR);

		string warningReport = BuildWarningReport();
		if (!warningReport.IsEmpty())
			Print(warningReport, LogLevel.WARNING);
	}

	protected bool ApplyToEditedCharacter()
	{
		IEntitySource target = GetEditedPrefabSource();
		if (!target)
			return false;

		if (NeedsLoad())
		{
			LoadCharacter();
			PrintFormat("EPE: loaded %1. Review the fields before applying.", m_BaseCharacter, level: LogLevel.WARNING);
			return false;
		}

		if (!m_API)
		{
			Print("EPE: World Editor API unavailable", LogLevel.ERROR);
			return false;
		}

		ReconcileWithWornGear();

		EPE_LoadoutDefinition loadout = BuildDefinition();
		if (!m_API.BeginEntityAction("EPE apply loadout"))
			return false;

		bool applied = EPE_PrefabWriter.ApplyLoadout(m_API, target, loadout, m_LoadedDefinition);
		applied = m_API.EndEntityAction() && applied;

		if (!applied)
		{
			Print("EPE: apply failed; prefab was not saved and panel contents are kept. Slots and paths are checked before the first write, so the prefab is usually untouched - if the log shows a write that failed, undo the partial action before retrying.", LogLevel.ERROR);
			return false;
		}

		m_LoadedDefinition = loadout;
		RebuildGroups(loadout);
		UpdatePropertyPanel();
		return true;
	}
}

class EPE_LoadoutReportDialog
{
	protected string m_sReport;

	void SetReport(string report)
	{
		m_sReport = report;
	}

	[ButtonAttribute("Copy To Log")]
	void ButtonCopyToLog()
	{
		Print(m_sReport);
	}

	[ButtonAttribute("Close")]
	bool ButtonClose()
	{
		return false;
	}
}
#endif
