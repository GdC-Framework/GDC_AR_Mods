#ifdef WORKBENCH
//! World Editor tool editing the loadout of a character prefab (vanilla, RHS, GDC...).
//! Only prefabs stored as files in an editable project can be modified; prefabs of packed mods and of the base game are read-only.
[WorkbenchToolAttribute(
	name: "GDC Loadout Editor",
	description: "Edit the loadout of a character prefab: clothing, weapons, equipment slots and inventory.\n1. Pick a character prefab in 'Character' and press 'Load'\n2. Edit, press 'Apply', then save with the Workbench",
	awesomeFontCode: 0xF553,
	wbModules: { "WorldEditor" })]
class GDC_LoadoutEditorTool : WorldEditorTool
{
	[Attribute(desc: "Character prefab to edit. Press 'Load' after picking it.", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", category: "Character")]
	protected ResourceName m_Prefab;

	[Attribute(desc: "Clothing slots of the character, read from the prefab", category: "Worn")]
	protected ref array<ref GDC_LoadoutWornSlot> m_aWorn;

	[Attribute(desc: "Weapon and grenade slots of the character, read from the prefab", category: "Weapons")]
	protected ref array<ref GDC_LoadoutWeaponSlot> m_aWeapons;

	[Attribute(desc: "Equipment slots of the character inventory (binoculars, watch...)", category: "Equipment")]
	protected ref array<ref GDC_LoadoutGadgetSlot> m_aGadgets;

	[Attribute(desc: "Inventory, one container per storage of the worn gear. Press 'Refresh Containers' after changing clothing.", category: "Inventory")]
	protected ref array<ref GDC_LoadoutContainer> m_aContainers;

	[Attribute(desc: "Items to spread over the storages by 'Distribute Items'. The list is emptied once they are placed.", category: "Automatic Distribution")]
	protected ref array<ref GDC_LoadoutItem> m_aItemsToDistribute;

	[Attribute(defvalue: "Vest,ArmoredVest,Jacket,Pants,Back", desc: "Gear filled first by 'Distribute Items', comma-separated slot names. Other storages come after.", category: "Automatic Distribution")]
	protected string m_sDistributionOrder;

	//! Inventory entries left empty are removed (only those defined in the edited prefab itself; inherited ones are emptied).
	protected static const bool REMOVE_EMPTY_ENTRIES = true;

	protected ref GDC_LoadoutSnapshot m_LoadedSnapshot;
	protected ref array<ref GDC_LoadoutStorageNode> m_aNodes = {};
	protected ref array<string> m_aWarnings = {};

	//! Entity created only to reach a prefab source during a write, deleted right after.
	protected IEntitySource m_TempEntity;

	// Hidden state of the panel entries (slot keys, container bindings), kept on the tool because the Workbench
	// may rebuild the panel objects without their hidden fields. Parallel to the panel arrays, by position.
	protected ref array<string> m_aWornKeys = {};
	protected ref array<string> m_aWeaponKeys = {};
	protected ref array<string> m_aWeaponLabels = {};
	protected ref array<string> m_aGadgetKeys = {};
	protected ref array<ref GDC_LoadoutContainer> m_aContainerBindings = {};

	//! Prefab that failed to load, not retried until the field changes.
	protected ResourceName m_FailedPrefab;

	//------------------------------------------------------------------------------------------------
	// Automatic loading
	// The Workbench does not notify tools of property changes, so the Character field is also checked
	// when the mouse enters or moves over the viewport.
	//------------------------------------------------------------------------------------------------

	override void OnActivate()
	{
		SyncWithField();
	}

	override void OnEnterEvent()
	{
		SyncWithField();
	}

	override void OnMouseMoveEvent(float x, float y)
	{
		SyncWithField();
	}

	//! Loads the prefab picked in the Character field when it differs from the one shown in the panel.
	protected void SyncWithField()
	{
		if (m_Prefab.IsEmpty() || m_Prefab == m_FailedPrefab)
			return;

		if (m_LoadedSnapshot && m_LoadedSnapshot.m_Prefab == m_Prefab)
			return;

		LoadPrefab(m_Prefab);
	}

	//------------------------------------------------------------------------------------------------
	// Buttons
	//------------------------------------------------------------------------------------------------

	//! Reads the picked prefab, discarding the changes not applied.
	[ButtonAttribute("Load")]
	protected void Load()
	{
		m_FailedPrefab = ResourceName.Empty;
		LoadPrefab(m_Prefab);
	}

	[ButtonAttribute("Refresh Containers")]
	protected void RefreshContainers()
	{
		if (!PreparePanel())
			return;

		UpdatePropertyPanel();
		PrintWarnings("containers refreshed for the clothing in the panel.");
	}

	[ButtonAttribute("Distribute Items")]
	protected void DistributeItems()
	{
		if (!m_aItemsToDistribute || m_aItemsToDistribute.IsEmpty())
		{
			Print("GDC Loadout Editor: add items to 'Items To Distribute' first.", LogLevel.WARNING);
			return;
		}

		if (!PreparePanel())
			return;

		GDC_LoadoutVolumeCache volumes = new GDC_LoadoutVolumeCache();
		array<string> report = {};
		GDC_LoadoutStorageTree.Distribute(m_aItemsToDistribute, m_aContainers, m_aNodes, m_sDistributionOrder, volumes, report);
		m_aItemsToDistribute.Clear();

		UpdateLabels();
		StoreBindings();
		UpdatePropertyPanel();

		foreach (string line : report)
		{
			Print("GDC Loadout Editor: " + line, LogLevel.WARNING);
		}

		PrintWarnings("items distributed. Review the containers, then press 'Apply'.");
	}

	[ButtonAttribute("Report")]
	protected void ShowReport()
	{
		if (!PreparePanel())
			return;

		UpdatePropertyPanel();

		string report = BuildReport();
		Workbench.ScriptDialog("GDC Loadout Editor - report", report, new GDC_LoadoutReportDialog(report));
	}

	//! Writes the panel changes into the loaded prefab file.
	[ButtonAttribute("Apply")]
	protected void Apply()
	{
		if (!PreparePanel())
			return;

		string absolutePath;
		string reason;
		if (!FindWritableFile(m_Prefab, absolutePath, reason))
		{
			ShowError(string.Format("%1 cannot be modified: %2.\n\nPrefabs of compiled (packed) mods and of the base game are read-only.", m_Prefab.GetPath(), reason));
			return;
		}

		if (!ConfirmPlan())
			return;

		if (ApplyTo(m_Prefab))
			LoadPrefab(m_Prefab);
		else
			ShowError(string.Format("The changes could not be applied to %1. See the console log for details.", m_Prefab.GetPath()));
	}

	protected void ShowError(string message)
	{
		Print("GDC Loadout Editor: " + message, LogLevel.ERROR);
		Workbench.Dialog("GDC Loadout Editor - error", message, string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	// Loading
	//------------------------------------------------------------------------------------------------

	protected bool LoadPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
		{
			Print("GDC Loadout Editor: pick a character prefab first.", LogLevel.WARNING);
			return false;
		}

		// remembered before any failure so that automatic loading does not retry it on every event
		m_FailedPrefab = prefab;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("GDC Loadout Editor: cannot load %1", prefab), LogLevel.ERROR);
			return false;
		}

		IEntitySource source = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!source)
		{
			Print(string.Format("GDC Loadout Editor: %1 is not an entity prefab", prefab), LogLevel.ERROR);
			return false;
		}

		GDC_LoadoutSnapshot snapshot = new GDC_LoadoutSnapshot();
		if (!GDC_LoadoutPrefabIO.Read(source, prefab, snapshot))
			return false;

		m_FailedPrefab = ResourceName.Empty;
		m_LoadedSnapshot = snapshot;
		m_Prefab = prefab;

		m_aWorn = {};
		foreach (GDC_LoadoutSlotRef worn : snapshot.m_aWorn)
		{
			m_aWorn.Insert(GDC_LoadoutWornSlot.Create(worn.m_sKey, worn.m_Prefab));
		}

		m_aWeapons = {};
		foreach (GDC_LoadoutSlotRef weapon : snapshot.m_aWeapons)
		{
			m_aWeapons.Insert(GDC_LoadoutWeaponSlot.Create(weapon.m_sKey, weapon.m_sLabel, weapon.m_Prefab));
		}

		m_aGadgets = {};
		foreach (GDC_LoadoutSlotRef gadget : snapshot.m_aGadgets)
		{
			m_aGadgets.Insert(GDC_LoadoutGadgetSlot.Create(gadget.m_sKey, gadget.m_Prefab));
		}

		if (!m_aItemsToDistribute)
			m_aItemsToDistribute = {};

		m_aContainers = {};
		GDC_LoadoutStorageTree.CollectNodes(m_aWorn, m_aNodes);
		GDC_LoadoutStorageTree.BuildContainers(snapshot.m_aEntries, m_aNodes, m_aContainers);
		UpdateLabels();
		StoreBindings();
		UpdatePropertyPanel();

		string absolutePath;
		string reason;
		string mode;
		if (FindWritableFile(prefab, absolutePath, reason))
			mode = string.Format("'Apply' modifies %1", absolutePath);
		else
			mode = string.Format("READ-ONLY, it cannot be modified (%1)", reason);

		Print(string.Format("GDC Loadout Editor: loaded %1 - %2", prefab.GetPath(), mode));
		PrintWarnings(string.Empty);
		return true;
	}

	//! True when the panel shows the prefab of the Character field. A newly picked prefab is loaded instead,
	//! and the caller stops so the user sees the loaded loadout before acting on it.
	protected bool IsLoaded()
	{
		if (m_Prefab.IsEmpty())
		{
			Print("GDC Loadout Editor: pick a character prefab in 'Character', then press 'Load'.", LogLevel.WARNING);
			return false;
		}

		if (m_LoadedSnapshot && m_LoadedSnapshot.m_Prefab == m_Prefab)
			return true;

		if (LoadPrefab(m_Prefab))
			Print("GDC Loadout Editor: the newly picked character was loaded. Review it, then press the button again.", LogLevel.WARNING);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Containers
	//------------------------------------------------------------------------------------------------

	protected void RebuildContainers()
	{
		if (!m_aContainers)
			m_aContainers = {};

		GDC_LoadoutStorageTree.CollectNodes(m_aWorn, m_aNodes);
		GDC_LoadoutStorageTree.Remap(m_aContainers, m_aNodes);
		UpdateLabels();
	}

	protected void UpdateLabels()
	{
		m_aWarnings.Clear();
		GDC_LoadoutStorageTree.UpdateLabels(m_aContainers, m_aNodes, new GDC_LoadoutVolumeCache(), m_aWarnings);
	}

	protected void PrintWarnings(string doneMessage)
	{
		if (!doneMessage.IsEmpty())
			Print("GDC Loadout Editor: " + doneMessage);

		foreach (string warning : m_aWarnings)
		{
			Print("GDC Loadout Editor: " + warning, LogLevel.WARNING);
		}
	}

	protected string BuildReport()
	{
		string report = string.Format("%1\n", m_Prefab.GetPath());

		if (!m_aWarnings.IsEmpty())
		{
			report += "\nWARNINGS\n";
			foreach (string warning : m_aWarnings)
			{
				report += string.Format("  %1\n", warning);
			}
		}

		report += "\nWORN\n";
		foreach (GDC_LoadoutWornSlot worn : m_aWorn)
		{
			if (worn)
				report += string.Format("  %1: %2\n", worn.m_sSlotName, GDC_LoadoutUtils.ShortName(worn.m_Prefab));
		}

		report += "\nWEAPONS\n";
		foreach (GDC_LoadoutWeaponSlot weapon : m_aWeapons)
		{
			if (weapon)
				report += string.Format("  %1: %2\n", weapon.m_sLabel, GDC_LoadoutUtils.ShortName(weapon.m_Prefab));
		}

		report += "\nEQUIPMENT SLOTS\n";
		foreach (GDC_LoadoutGadgetSlot gadget : m_aGadgets)
		{
			if (gadget)
				report += string.Format("  %1: %2\n", gadget.m_sSlotName, GDC_LoadoutUtils.ShortName(gadget.m_Prefab));
		}

		report += "\nINVENTORY\n";
		foreach (GDC_LoadoutContainer container : m_aContainers)
		{
			if (!container)
				continue;

			report += string.Format("  %1\n", container.m_sLabel);
			if (!container.m_sTargetStorage.IsEmpty())
				report += string.Format("    TargetStorage: %1\n", container.m_sTargetStorage);

			foreach (GDC_LoadoutItem item : container.m_aItems)
			{
				if (item && !item.m_Prefab.IsEmpty() && item.m_iCount > 0)
					report += string.Format("    %1 x %2\n", item.m_iCount, GDC_LoadoutUtils.ShortName(item.m_Prefab));
			}
		}

		return report;
	}

	//------------------------------------------------------------------------------------------------
	// Write checks
	//------------------------------------------------------------------------------------------------

	//! A prefab can be modified in place when a loaded, non-vanilla addon holds it as a loose file on disk.
	//! When several addons hold the same path, the last loaded one wins, as for resource loading.
	protected bool FindWritableFile(ResourceName prefab, out string absolutePath, out string reason)
	{
		absolutePath = string.Empty;
		array<string> addonIDs = SCR_AddonTool.GetAllAddonIDs();
		foreach (string addonID : addonIDs)
		{
			if (SCR_AddonTool.IsVanillaAddon(addonID))
				continue;

			string candidate;
			if (Workbench.GetAbsolutePath(SCR_AddonTool.ToFileSystem(addonID) + prefab.GetPath(), candidate, true) && FileIO.FileExists(candidate))
				absolutePath = candidate;
		}

		if (absolutePath.IsEmpty())
		{
			reason = "no editable addon holds this file (base game or packed addon)";
			return false;
		}

		return true;
	}

	//! Asks for confirmation before a write that looks like corrupted panel data: large inventory loss,
	//! several clothing slots emptied, or several slots receiving the same item.
	protected bool ConfirmPlan()
	{
		GDC_LoadoutPlan plan = new GDC_LoadoutPlan();
		array<string> errors = {};
		if (!GDC_LoadoutPrefabIO.Plan(m_LoadedSnapshot, m_aWorn, m_aWeapons, m_aGadgets, m_aContainers, REMOVE_EMPTY_ENTRIES, plan, errors))
			return true; // errors are reported by the real write, which stops before writing anything

		string reasons;
		if (plan.m_iItemsBefore > 0 && plan.m_iItemsAfter * 2 < plan.m_iItemsBefore)
			reasons += string.Format("- only %1 of the %2 inventory items are kept\n", plan.m_iItemsAfter, plan.m_iItemsBefore);

		if (plan.m_iWornCleared >= 2)
			reasons += string.Format("- %1 clothing slots are emptied\n", plan.m_iWornCleared);

		int sameValueWrites = plan.GetMaxSameValueWrites();
		if (sameValueWrites >= 2)
			reasons += string.Format("- %1 slots receive the same item\n", sameValueWrites);

		if (reasons.IsEmpty())
			return true;

		string message = "This write looks unusual:\n" + reasons + "\nChanges:\n";
		foreach (string change : plan.m_aChanges)
		{
			message += change + "\n";
		}

		message += "\nWrite anyway?";
		if (Workbench.ScriptDialog("GDC Loadout Editor - confirm", message, new GDC_LoadoutConfirmDialog()) != 0)
			return true;

		Print("GDC Loadout Editor: cancelled, nothing was written.", LogLevel.WARNING);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Panel state
	//------------------------------------------------------------------------------------------------

	//! Restores the hidden state of the panel entries and rebinds the containers to the current clothing.
	//! Refuses when the panel no longer matches what was loaded, instead of writing wrong data.
	protected bool PreparePanel()
	{
		if (!IsLoaded() || !RestoreBindings())
			return false;

		RebuildContainers();
		StoreBindings();
		UpdatePropertyPanel();
		return true;
	}

	//! Called after the tool itself filled the panel, so every entry is set.
	protected void StoreBindings()
	{
		m_aWornKeys.Clear();
		foreach (GDC_LoadoutWornSlot worn : m_aWorn)
		{
			if (worn)
				m_aWornKeys.Insert(worn.m_sSlotName);
		}

		m_aWeaponKeys.Clear();
		m_aWeaponLabels.Clear();
		foreach (GDC_LoadoutWeaponSlot weapon : m_aWeapons)
		{
			if (!weapon)
				continue;

			m_aWeaponKeys.Insert(weapon.m_sKey);
			m_aWeaponLabels.Insert(weapon.m_sLabel);
		}

		m_aGadgetKeys.Clear();
		foreach (GDC_LoadoutGadgetSlot gadget : m_aGadgets)
		{
			if (gadget)
				m_aGadgetKeys.Insert(gadget.m_sSlotName);
		}

		m_aContainerBindings.Clear();
		foreach (GDC_LoadoutContainer container : m_aContainers)
		{
			if (!container)
				continue;

			GDC_LoadoutContainer binding = new GDC_LoadoutContainer();
			CopyBinding(container, binding);
			m_aContainerBindings.Insert(binding);
		}
	}

	protected bool RestoreBindings(bool quiet = false)
	{
		if (!m_aWorn || !m_aWeapons || !m_aGadgets || !m_aContainers
			|| m_aWorn.Count() != m_aWornKeys.Count()
			|| m_aWeapons.Count() != m_aWeaponKeys.Count()
			|| m_aGadgets.Count() != m_aGadgetKeys.Count()
			|| m_aContainers.Count() != m_aContainerBindings.Count())
		{
			if (!quiet)
				Print("GDC Loadout Editor: the panel no longer matches the loaded prefab (entries added or removed in Worn, Weapons, Equipment or Inventory, or the tool was reset by the Workbench). Press 'Load'. Nothing was written.", LogLevel.ERROR);
			return false;
		}

		foreach (int wornIndex, GDC_LoadoutWornSlot worn : m_aWorn)
		{
			if (!worn)
				return FailRestore(quiet);

			worn.m_sSlotName = m_aWornKeys[wornIndex];
		}

		foreach (int weaponIndex, GDC_LoadoutWeaponSlot weapon : m_aWeapons)
		{
			if (!weapon)
				return FailRestore(quiet);

			weapon.m_sKey = m_aWeaponKeys[weaponIndex];
			weapon.m_sLabel = m_aWeaponLabels[weaponIndex];
		}

		foreach (int gadgetIndex, GDC_LoadoutGadgetSlot gadget : m_aGadgets)
		{
			if (!gadget)
				return FailRestore(quiet);

			gadget.m_sSlotName = m_aGadgetKeys[gadgetIndex];
		}

		foreach (int containerIndex, GDC_LoadoutContainer container : m_aContainers)
		{
			if (!container)
				return FailRestore(quiet);

			CopyBinding(m_aContainerBindings[containerIndex], container);
			if (!container.m_aItems)
				container.m_aItems = {};
		}

		return true;
	}

	protected bool FailRestore(bool quiet)
	{
		if (!quiet)
			Print("GDC Loadout Editor: the panel contains empty entries. Press 'Load'. Nothing was written.", LogLevel.ERROR);
		return false;
	}

	//! Copies the hidden fields (not the items) of a container.
	protected static void CopyBinding(notnull GDC_LoadoutContainer source, notnull GDC_LoadoutContainer target)
	{
		target.m_sLabel = source.m_sLabel;
		target.m_sTargetStorage = source.m_sTargetStorage;
		target.m_iTargetPurpose = source.m_iTargetPurpose;
		target.m_sNodePath = source.m_sNodePath;

		array<int> sourceIndices = {};
		if (source.m_aSourceIndices)
			sourceIndices.Copy(source.m_aSourceIndices);

		target.m_aSourceIndices = sourceIndices;
	}

	//------------------------------------------------------------------------------------------------
	// Writing
	//------------------------------------------------------------------------------------------------

	protected bool ApplyTo(ResourceName target)
	{
		if (!m_API)
		{
			Print("GDC Loadout Editor: World Editor API unavailable", LogLevel.ERROR);
			return false;
		}

		bool manageAction = !m_API.IsDoingEditAction();
		if (manageAction)
			m_API.BeginEntityAction("GDC Loadout Editor");

		IEntitySource prefab = AcquirePrefabSource(target);
		bool success;
		if (prefab)
			success = WriteChanges(prefab, target);

		if (m_TempEntity)
			m_API.DeleteEntity(m_TempEntity);

		m_TempEntity = null;

		if (manageAction)
			m_API.EndEntityAction();

		return success;
	}

	//! Prefab entity source to write into: the opened prefab in Edit Prefab mode, otherwise the ancestor of a temporary entity (vanilla SCR_PrefabHelper approach).
	//! The temporary entity, if any, is stored in m_TempEntity and must be deleted by the caller.
	protected IEntitySource AcquirePrefabSource(ResourceName target)
	{
		m_TempEntity = null;
		if (m_API.IsPrefabEditMode())
		{
			IEntitySource opened = SCR_WorldEditorToolHelper.GetPrefabEditModeEntitySource();
			if (opened && opened.GetAncestor() && GDC_LoadoutUtils.SamePath(opened.GetAncestor().GetResourceName(), target))
				return IEntitySource.Cast(opened.GetAncestor());

			Print("GDC Loadout Editor: in Edit Prefab mode, only the opened prefab can be written. Open a world to edit another prefab.", LogLevel.ERROR);
			return null;
		}

		m_TempEntity = m_API.CreateEntity(target, string.Empty, m_API.GetCurrentEntityLayerId(), null, vector.Zero, vector.Zero);
		if (!m_TempEntity)
		{
			Print(string.Format("GDC Loadout Editor: cannot instantiate %1 in the current layer (is it locked?)", target.GetPath()), LogLevel.ERROR);
			return null;
		}

		IEntitySource prefab = IEntitySource.Cast(m_TempEntity.GetAncestor());
		if (!prefab)
			Print(string.Format("GDC Loadout Editor: cannot reach the prefab of %1", target.GetPath()), LogLevel.ERROR);

		return prefab;
	}

	protected bool WriteChanges(notnull IEntitySource prefab, ResourceName target)
	{
		GDC_LoadoutSnapshot current = new GDC_LoadoutSnapshot();
		if (!GDC_LoadoutPrefabIO.Read(prefab, target, current))
			return false;

		string difference = m_LoadedSnapshot.Compare(current);
		if (!difference.IsEmpty())
		{
			Print(string.Format("GDC Loadout Editor: the prefab changed since it was loaded (%1). Press 'Load' and redo your changes.", difference), LogLevel.ERROR);
			return false;
		}

		GDC_LoadoutPlan plan = new GDC_LoadoutPlan();
		array<string> errors = {};
		if (!GDC_LoadoutPrefabIO.Plan(current, m_aWorn, m_aWeapons, m_aGadgets, m_aContainers, REMOVE_EMPTY_ENTRIES, plan, errors))
		{
			foreach (string planError : errors)
			{
				Print("GDC Loadout Editor: " + planError, LogLevel.ERROR);
			}

			Print("GDC Loadout Editor: nothing was written.", LogLevel.ERROR);
			return false;
		}

		if (plan.IsEmpty())
		{
			Print(string.Format("GDC Loadout Editor: %1 already matches the panel.", target.GetPath()));
			return true;
		}

		if (!GDC_LoadoutPrefabIO.Execute(m_API, prefab, current, plan))
		{
			Print("GDC Loadout Editor: applying stopped halfway. Undo (Ctrl+Z) or reload the prefab before retrying.", LogLevel.ERROR);
			return false;
		}

		// the file itself is written by the Workbench save, like any other prefab change
		Print(string.Format("GDC Loadout Editor: applied to %1. Save with the Workbench to write the file.", target.GetPath()));
		foreach (string change : plan.m_aChanges)
		{
			Print("  " + change);
		}

		PrintWarnings(string.Empty);
		return true;
	}
}
#endif
