#ifdef WORKBENCH
//! World Editor tool editing the loadout of a character prefab (vanilla, RHS, GDC...).
//! Only prefabs stored as files in an editable project can be modified; prefabs of packed mods and of the base game are read-only.
[WorkbenchToolAttribute(
	name: "GDC Loadout Editor",
	description: "Edit the loadout of a character prefab: clothing, weapons, equipment slots and inventory.\n1. Pick a character prefab in 'Character' and press 'Load'\n2. Edit, press 'Apply', then save with the Workbench",
	icon: "WBData/GDC/GDC_LoadoutEditor.png",
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

	//! Prefab resource kept loaded during Apply, so that its entity source stays valid while it is written.
	protected ref Resource m_WriteResource;

	//! Progress dialog shown during Apply, closed when released.
	protected ref WBProgressDialog m_Progress;

	//! Set by the last Apply when it actually changed the prefab, to remind the user to save.
	protected bool m_bPrefabModified;

	//! Prefabs of the loaded loadout that cannot be loaded (missing addon).
	protected ref array<ResourceName> m_aMissingPrefabs = {};

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

	//! Adapts the containers to the clothing in the panel. Items whose storage is no longer worn are moved
	//! to 'Items To Distribute'; this happens only here, never implicitly before another action.
	[ButtonAttribute("Refresh Containers")]
	protected void RefreshContainers()
	{
		if (!PreparePanel())
			return;

		if (!m_aItemsToDistribute)
			m_aItemsToDistribute = {};

		array<string> moved = {};
		if (m_aMissingPrefabs.IsEmpty())
		{
			GDC_LoadoutStorageTree.Remap(m_aContainers, m_aNodes, m_aItemsToDistribute, moved);
		}
		else
		{
			// gear that cannot be loaded looks absent: its items are kept where they are instead of being moved
			GDC_LoadoutStorageTree.Remap(m_aContainers, m_aNodes);
			Print("[GDC LoadoutEditor] some prefabs of this loadout cannot be loaded (missing addon), so no item was moved to 'Items To Distribute'.", LogLevel.WARNING);
		}
		UpdateLabels();
		StoreBindings();
		UpdatePropertyPanel();

		foreach (string line : moved)
		{
			Print("[GDC LoadoutEditor] " + line, LogLevel.WARNING);
		}

		PrintWarnings("containers refreshed for the clothing in the panel.");
	}

	//! Empties every inventory container of the panel, typically before 'Distribute Items'. Nothing is written before 'Apply'.
	[ButtonAttribute("Clear Inventory")]
	protected void ClearInventory()
	{
		if (!PreparePanel())
			return;

		int itemCount;
		foreach (GDC_LoadoutContainer counted : m_aContainers)
		{
			if (counted)
				itemCount += counted.CountItems();
		}

		if (itemCount == 0)
		{
			Print("[GDC LoadoutEditor] the inventory is already empty.");
			return;
		}

		string question = string.Format("Remove all %1 items from the inventory of %2?\n\nNothing is written to the prefab before 'Apply'; 'Load' restores the inventory until then.", itemCount, GDC_LoadoutUtils.ShortName(m_Prefab));
		if (Workbench.ScriptDialog("GDC Loadout Editor - clear inventory", GDC_LoadoutUtils.DialogText(GDC_LoadoutUtils.DIALOG_QUESTION, question), new GDC_LoadoutClearDialog()) == 0)
			return;

		int removed;
		foreach (GDC_LoadoutContainer container : m_aContainers)
		{
			if (!container)
				continue;

			removed += container.CountItems();
			container.m_aItems.Clear();
		}

		UpdateLabels();
		StoreBindings();
		UpdatePropertyPanel();
		Print(string.Format("[GDC LoadoutEditor] %1 items removed from the inventory. Fill 'Items To Distribute' and press 'Distribute Items', then 'Apply'. Press 'Load' to cancel.", removed));
	}

	[ButtonAttribute("Distribute Items")]
	protected void DistributeItems()
	{
		if (!m_aItemsToDistribute || m_aItemsToDistribute.IsEmpty())
		{
			Print("[GDC LoadoutEditor] add items to 'Items To Distribute' first.", LogLevel.WARNING);
			return;
		}

		if (!PreparePanel())
			return;

		GDC_LoadoutItemCache itemCache = new GDC_LoadoutItemCache();
		array<string> report = {};
		GDC_LoadoutStorageTree.Distribute(m_aItemsToDistribute, m_aContainers, m_aNodes, m_sDistributionOrder, itemCache, report);
		m_aItemsToDistribute.Clear();

		UpdateLabels();
		StoreBindings();
		UpdatePropertyPanel();

		foreach (string line : report)
		{
			Print("[GDC LoadoutEditor] " + line, LogLevel.WARNING);
		}

		PrintWarnings("items distributed. Review the containers, then press 'Apply'.");
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

		// without the addon providing some gear, its storages look absent and the loadout would be written inconsistently
		if (!m_aMissingPrefabs.IsEmpty())
		{
			ShowError(string.Format("%1 prefabs used by this loadout cannot be loaded (e.g. %2), probably because their addon (such as RHS) is not loaded in this Workbench session.\n\nLoad the missing addon, then press 'Load' before applying changes.", m_aMissingPrefabs.Count(), GDC_LoadoutUtils.ShortName(m_aMissingPrefabs[0])));
			return;
		}

		if (!ConfirmPlan())
			return;

		// the dialog stays open while m_Progress holds it
		m_Progress = new WBProgressDialog("GDC Loadout Editor", Workbench.GetModule(WorldEditor));
		SetProgressStep("Preparing the prefab", 0);

		m_bPrefabModified = false;
		bool applied = ApplyTo(m_Prefab);
		if (applied)
		{
			SetProgressStep("Reloading the loadout", 1);
			LoadPrefab(m_Prefab);
		}

		m_Progress = null;

		if (!applied)
		{
			ShowError(string.Format("The changes could not be applied to %1. See the console log for details.", m_Prefab.GetPath()));
			return;
		}

		// the tool only changes the prefab in memory: the file is written by the Workbench save
		if (m_bPrefabModified)
			OfferSave();
	}

	protected void OfferSave()
	{
		string message = string.Format("The loadout was applied to %1, but the file is not saved yet.\n\n'Save world' saves the world and the modified prefab (like Ctrl+S, other unsaved changes of the world are saved too).", GDC_LoadoutUtils.ShortName(m_Prefab));
		if (Workbench.ScriptDialog("GDC Loadout Editor - save", GDC_LoadoutUtils.DialogText(GDC_LoadoutUtils.DIALOG_QUESTION, message), new GDC_LoadoutSaveDialog()) == 0)
		{
			Print(string.Format("[GDC LoadoutEditor] %1 is modified but NOT saved. Save the world (Ctrl+S) to write the file.", m_Prefab.GetPath()), LogLevel.WARNING);
			return;
		}

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (worldEditor && worldEditor.Save())
			Print(string.Format("[GDC LoadoutEditor] world saved, %1 written.", m_Prefab.GetPath()));
		else
			ShowError(string.Format("Saving failed. Save the world with Ctrl+S to write %1.", m_Prefab.GetPath()));
	}

	protected void SetProgressStep(string action, float progress)
	{
		if (!m_Progress)
			return;

		m_Progress.SetMainTitle("Applying the loadout");
		m_Progress.SetActionTitle(action);
		m_Progress.SetProgress(progress);
	}

	protected void ShowError(string message)
	{
		Print("[GDC LoadoutEditor] " + message, LogLevel.ERROR);
		Workbench.Dialog("GDC Loadout Editor - error", GDC_LoadoutUtils.DialogText(GDC_LoadoutUtils.DIALOG_WARNING, message), string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	// Loading
	//------------------------------------------------------------------------------------------------

	protected bool LoadPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
		{
			Print("[GDC LoadoutEditor] pick a character prefab first.", LogLevel.WARNING);
			return false;
		}

		// remembered before any failure so that automatic loading does not retry it on every event
		m_FailedPrefab = prefab;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("[GDC LoadoutEditor] cannot load %1", prefab), LogLevel.ERROR);
			return false;
		}

		IEntitySource source = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!source)
		{
			Print(string.Format("[GDC LoadoutEditor] %1 is not an entity prefab", prefab), LogLevel.ERROR);
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

		Print(string.Format("[GDC LoadoutEditor] loaded %1 - %2", prefab.GetPath(), mode));
		ReportMissingPrefabs(snapshot);
		PrintWarnings(string.Empty);
		return true;
	}

	//! Lists once the prefabs of the loadout that cannot be loaded, typically because the addon providing them
	//! (e.g. RHS) is not loaded in this Workbench session. Their storages cannot be read.
	protected void ReportMissingPrefabs(notnull GDC_LoadoutSnapshot snapshot)
	{
		array<ResourceName> used = {};
		foreach (GDC_LoadoutSlotRef worn : snapshot.m_aWorn)
		{
			used.Insert(worn.m_Prefab);
		}

		foreach (GDC_LoadoutSlotRef weapon : snapshot.m_aWeapons)
		{
			used.Insert(weapon.m_Prefab);
		}

		foreach (GDC_LoadoutSlotRef gadget : snapshot.m_aGadgets)
		{
			used.Insert(gadget.m_Prefab);
		}

		foreach (GDC_LoadoutEntryRef entry : snapshot.m_aEntries)
		{
			used.InsertAll(entry.m_aPrefabs);
		}

		m_aMissingPrefabs.Clear();
		foreach (ResourceName candidate : used)
		{
			if (candidate.IsEmpty() || m_aMissingPrefabs.Contains(candidate))
				continue;

			Resource resource = Resource.Load(candidate);
			if (!resource || !resource.IsValid())
				m_aMissingPrefabs.Insert(candidate);
		}

		if (m_aMissingPrefabs.IsEmpty())
			return;

		string names;
		foreach (int missingIndex, ResourceName missing : m_aMissingPrefabs)
		{
			if (missingIndex == 8)
			{
				names += ", ...";
				break;
			}

			if (missingIndex > 0)
				names += ", ";

			names += GDC_LoadoutUtils.ShortName(missing);
		}

		Print(string.Format("[GDC LoadoutEditor] %1 prefabs used by this loadout cannot be loaded, probably because the addon providing them (e.g. RHS) is not loaded in this Workbench session: %2. The storages of the missing gear cannot be read, so their items are shown as NOT ON CURRENT GEAR. Load the missing addon before editing this loadout.", m_aMissingPrefabs.Count(), names), LogLevel.WARNING);
	}

	//! True when the panel shows the prefab of the Character field. A newly picked prefab is loaded instead,
	//! and the caller stops so the user sees the loaded loadout before acting on it.
	protected bool IsLoaded()
	{
		if (m_Prefab.IsEmpty())
		{
			Print("[GDC LoadoutEditor] pick a character prefab in 'Character', then press 'Load'.", LogLevel.WARNING);
			return false;
		}

		if (m_LoadedSnapshot && m_LoadedSnapshot.m_Prefab == m_Prefab)
			return true;

		if (LoadPrefab(m_Prefab))
			Print("[GDC LoadoutEditor] the newly picked character was loaded. Review it, then press the button again.", LogLevel.WARNING);

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
		GDC_LoadoutStorageTree.UpdateLabels(m_aContainers, m_aNodes, new GDC_LoadoutItemCache(), m_aWarnings);
	}

	protected void PrintWarnings(string doneMessage)
	{
		if (!doneMessage.IsEmpty())
			Print("[GDC LoadoutEditor] " + doneMessage);

		foreach (string warning : m_aWarnings)
		{
			Print("[GDC LoadoutEditor] " + warning, LogLevel.WARNING);
		}
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
		if (Workbench.ScriptDialog("GDC Loadout Editor - confirm", GDC_LoadoutUtils.DialogText(GDC_LoadoutUtils.DIALOG_WARNING, message), new GDC_LoadoutConfirmDialog()) != 0)
			return true;

		Print("[GDC LoadoutEditor] cancelled, nothing was written.", LogLevel.WARNING);
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
				Print("[GDC LoadoutEditor] the panel no longer matches the loaded prefab (entries added or removed in Worn, Weapons, Equipment or Inventory, or the tool was reset by the Workbench). Press 'Load'. Nothing was written.", LogLevel.ERROR);
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
			Print("[GDC LoadoutEditor] the panel contains empty entries. Press 'Load'. Nothing was written.", LogLevel.ERROR);
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
			Print("[GDC LoadoutEditor] World Editor API unavailable", LogLevel.ERROR);
			return false;
		}

		bool manageAction = !m_API.IsDoingEditAction();
		if (manageAction)
			m_API.BeginEntityAction("GDC Loadout Editor");

		IEntitySource prefab = AcquirePrefabSource(target);
		bool success;
		if (prefab)
			success = WriteChanges(prefab, target);

		m_WriteResource = null;

		if (manageAction)
			m_API.EndEntityAction();

		return success;
	}

	//! Prefab entity source to write into: the opened prefab in Edit Prefab mode, otherwise the ancestor of a temporary entity
	//! (vanilla SCR_PrefabHelper approach). The temporary entity is deleted right away, before any write: while it exists,
	//! the World Editor recreates it (character, gear, weapons) after every change made to the prefab.
	protected IEntitySource AcquirePrefabSource(ResourceName target)
	{
		if (m_API.IsPrefabEditMode())
		{
			IEntitySource opened = SCR_WorldEditorToolHelper.GetPrefabEditModeEntitySource();
			if (opened && opened.GetAncestor() && GDC_LoadoutUtils.SamePath(opened.GetAncestor().GetResourceName(), target))
				return IEntitySource.Cast(opened.GetAncestor());

			Print("[GDC LoadoutEditor] in Edit Prefab mode, only the opened prefab can be written. Open a world to edit another prefab.", LogLevel.ERROR);
			return null;
		}

		// keeps the prefab resource loaded while its source is written
		m_WriteResource = Resource.Load(target);

		IEntitySource tempEntity = m_API.CreateEntity(target, string.Empty, m_API.GetCurrentEntityLayerId(), null, vector.Zero, vector.Zero);
		if (!tempEntity)
		{
			Print(string.Format("[GDC LoadoutEditor] cannot instantiate %1 in the current layer (is it locked?)", target.GetPath()), LogLevel.ERROR);
			return null;
		}

		IEntitySource prefab = IEntitySource.Cast(tempEntity.GetAncestor());
		m_API.DeleteEntity(tempEntity);

		if (!prefab)
			Print(string.Format("[GDC LoadoutEditor] cannot reach the prefab of %1", target.GetPath()), LogLevel.ERROR);

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
			Print(string.Format("[GDC LoadoutEditor] the prefab changed since it was loaded (%1). Press 'Load' and redo your changes.", difference), LogLevel.ERROR);
			return false;
		}

		GDC_LoadoutPlan plan = new GDC_LoadoutPlan();
		array<string> errors = {};
		if (!GDC_LoadoutPrefabIO.Plan(current, m_aWorn, m_aWeapons, m_aGadgets, m_aContainers, REMOVE_EMPTY_ENTRIES, plan, errors))
		{
			foreach (string planError : errors)
			{
				Print("[GDC LoadoutEditor] " + planError, LogLevel.ERROR);
			}

			Print("[GDC LoadoutEditor] nothing was written.", LogLevel.ERROR);
			return false;
		}

		if (plan.IsEmpty())
		{
			Print(string.Format("[GDC LoadoutEditor] %1 already matches the panel.", target.GetPath()));
			return true;
		}

		if (!GDC_LoadoutPrefabIO.Execute(m_API, prefab, current, plan, m_Progress))
		{
			Print("[GDC LoadoutEditor] applying stopped halfway. Undo (Ctrl+Z) or reload the prefab before retrying.", LogLevel.ERROR);
			return false;
		}

		// the file itself is written by the Workbench save, like any other prefab change
		m_bPrefabModified = true;
		Print(string.Format("[GDC LoadoutEditor] applied to %1. Save with the Workbench to write the file.", target.GetPath()));
		foreach (string change : plan.m_aChanges)
		{
			Print("[GDC LoadoutEditor]   " + change);
		}

		PrintWarnings(string.Empty);
		return true;
	}
}
#endif
