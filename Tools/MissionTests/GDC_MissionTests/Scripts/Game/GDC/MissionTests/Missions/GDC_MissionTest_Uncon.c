// GDC_MissionTest_Uncon.c
// Path in mod: Scripts/Game/GDC/MissionTests/Missions/GDC_MissionTest_Uncon.c

//! Test of the unconscious player rules of GDC Core, on the Arland test world (worlds/tanin69/_Tests/GDC_Teleport_Arland.ent):
//! a player who falls unconscious keeps his weapon (put away, not dropped) and his name tag never shows the unconscious state.
//! The server takes the lobby slots, knocks the first player out, then wakes him up. The clients check what only they can see.
class GDC_MissionTest_Uncon : GDC_MissionTest
{
	protected int m_iVictimId;
	protected int m_iObserverId;
	protected IEntity m_Victim;
	protected IEntity m_Weapon;

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "Unconscious players";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForPlayers(); return;
			case 2: AdvanceLobby(); NextStep(3); return;
			case 3: TakeSlots(); return;
			case 4: AdvanceLobby(); NextStep(5); return;
			case 5: AdvanceLobby(); NextStep(3); return;
			case 6: WaitForControl(); return;
			case 7: CheckWeaponInHand(); return;
			case 8: CheckUnconscious(); return;
			case 9: WakeUp(); return;
			case 10: CheckAwake(); return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForPlayers()
	{
		if (GetPlayerCount() >= 2 && GetGameState() != SCR_EGameModeState.NULL)
		{
			m_iVictimId = GetPlayerId(0);
			m_iObserverId = GetPlayerId(1);
			Info(string.Format("Players connected: victim %1, observer %2", m_iVictimId, m_iObserverId));
			NextStep(5);
			return;
		}

		if (StepElapsed() > 600)
		{
			Check("Two players connected", false, string.Format("%1 after 600 s", GetPlayerCount()));
			Finish();
			return;
		}

		RepeatStep(2);
	}

	//------------------------------------------------------------------------------------------------
	//! The two players take the first two soldiers of the playable group
	protected void TakeSlots()
	{
		array<IEntity> soldiers = {};
		PS_PlayableManager manager = PS_PlayableManager.GetInstance();
		map<RplId, ref PS_PlayableContainer> playables = manager.GetPlayables();
		foreach (RplId id, PS_PlayableContainer container : playables)
		{
			if (container && container.GetPlayableComponent())
				soldiers.Insert(container.GetPlayableComponent().GetOwner());
		}

		if (!Check("At least 2 playable slots", soldiers.Count() >= 2, string.Format("%1", soldiers.Count())))
		{
			Finish();
			return;
		}

		m_Victim = soldiers[0];
		Check("Victim slot taken", AssignSlot(m_iVictimId, soldiers[0]));
		Check("Observer slot taken", AssignSlot(m_iObserverId, soldiers[1]));
		NextStep(3);
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForControl()
	{
		if (GetPlayerEntity(m_iVictimId) == m_Victim && GetPlayerEntity(m_iObserverId))
		{
			Check("Players control their slot", true, string.Format("%1 s", Math.Round(StepElapsed())));
			// The victim's client draws its weapon a few seconds after taking control
			NextStep(20);
			return;
		}

		if (StepElapsed() > 60)
		{
			Check("Players control their slot", false, typename.EnumToString(SCR_EGameModeState, GetGameState()));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckWeaponInHand()
	{
		BaseWeaponManagerComponent weaponManager = GetWeaponManager(m_Victim);
		if (weaponManager && weaponManager.GetCurrentWeapon())
			m_Weapon = weaponManager.GetCurrentWeapon().GetOwner();

		if (!Check("Victim holds a weapon before falling unconscious", m_Weapon != null))
			Warn("The weapon checks below are not meaningful");

		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(SCR_ChimeraCharacter.Cast(m_Victim).GetDamageManager());
		damageManager.ForceUnconsciousness();
		Info("Victim knocked out");
		NextStep(8);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckUnconscious()
	{
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(SCR_ChimeraCharacter.Cast(m_Victim).GetCharacterController());
		Check("Victim is unconscious", controller.GetLifeState() == ECharacterLifeState.INCAPACITATED, typename.EnumToString(ECharacterLifeState, controller.GetLifeState()));
		CheckWeaponKept("while unconscious");

		BaseWeaponManagerComponent weaponManager = GetWeaponManager(m_Victim);
		Check("Weapon no longer in the hands", !weaponManager || !weaponManager.GetCurrentWeapon());

		// The clients check the name tags meanwhile
		NextStep(20);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckWeaponKept(string when)
	{
		if (!m_Weapon)
			return;

		IEntity root = m_Weapon.GetRootParent();
		string detail = "carried";
		if (root != m_Victim)
			detail = "on the ground at " + Pos(m_Weapon.GetOrigin());

		Check("Weapon kept by the victim " + when, root == m_Victim, detail);
	}

	//------------------------------------------------------------------------------------------------
	protected void WakeUp()
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_Victim);
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(character.GetDamageManager());
		damageManager.FullHeal();
		character.GetCharacterController().SetUnconscious(false);
		Info("Victim healed and woken up");
		NextStep(10);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAwake()
	{
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(SCR_ChimeraCharacter.Cast(m_Victim).GetCharacterController());
		Check("Victim is conscious again", controller.GetLifeState() == ECharacterLifeState.ALIVE, typename.EnumToString(ECharacterLifeState, controller.GetLifeState()));
		CheckWeaponKept("after waking up");
		CheckEditorBudgets();
		CheckSupplies();
		Finish();
	}

	//------------------------------------------------------------------------------------------------
	//! GDC Core disables supplies and lets the repair kit repair fully (replaces the "No Supply System" mod)
	protected void CheckSupplies()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		Check("Supplies disabled by the game mode", !gameMode.IsResourceTypeEnabled(EResourceType.SUPPLIES));

		Resource resource = Resource.Load("{4AF9664BEE9263A4}Prefabs/Items/Equipment/Kits/RepairKit_01/RepairKit_01_base.et");
		IEntitySource source;
		if (resource && resource.IsValid())
			source = resource.GetResource().ToEntitySource();

		IEntityComponentSource station;
		if (source)
		{
			for (int i = 0, count = source.GetComponentCount(); i < count; i++)
			{
				IEntityComponentSource component = source.GetComponent(i);
				if (component.GetClassName() == "SCR_RepairSupportStationComponent")
				{
					station = component;
					break;
				}
			}
		}

		if (!Check("Repair kit support station found", station != null))
			return;

		float maxHeal;
		int healed;
		station.Get("m_fMaxHealScaled", maxHeal);
		station.Get("m_iDamageHealedEachExecution", healed);
		Check("Repair kit repairs up to 100 %", maxHeal == 1, string.Format("%1", maxHeal));
		CheckInt("Repair kit repairs 50 per action", healed, 50);
	}

	//------------------------------------------------------------------------------------------------
	//! GDC Core raises the Game Master budgets (replaces the "FNG GM Budgetless" mod)
	protected void CheckEditorBudgets()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		SCR_EditorManagerEntity editorManager;
		if (core)
			editorManager = core.GetEditorManager(m_iVictimId);

		SCR_BudgetEditorComponent budgets;
		if (editorManager)
			budgets = SCR_BudgetEditorComponent.Cast(editorManager.FindComponent(SCR_BudgetEditorComponent));

		if (!Check("Editor budget component found", budgets != null))
			return;

		array<EEditableEntityBudget> types = {EEditableEntityBudget.PROPS, EEditableEntityBudget.AI, EEditableEntityBudget.VEHICLES, EEditableEntityBudget.SYSTEMS};
		foreach (EEditableEntityBudget type : types)
		{
			int maxBudget;
			budgets.GetMaxBudgetValue(type, maxBudget);
			CheckInt("GM max budget " + typename.EnumToString(EEditableEntityBudget, type), maxBudget, 100000);
		}
	}

	//------------------------------------------------------------------------------------------------
	static BaseWeaponManagerComponent GetWeaponManager(IEntity character)
	{
		if (!character)
			return null;

		return BaseWeaponManagerComponent.Cast(character.FindComponent(BaseWeaponManagerComponent));
	}
}

//! Client side: the victim draws a weapon and checks its own tag, the observer checks the victim's tag.
class GDC_MissionTestClient_Uncon : GDC_MissionTest
{
	protected bool m_bVictim;
	protected bool m_bTagChecked;
	protected bool m_bWasUnconscious;

	//------------------------------------------------------------------------------------------------
	override string GetName()
	{
		return "Unconscious players (client)";
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunStep(int step)
	{
		switch (step)
		{
			case 1: WaitForCharacter(); return;
			case 2: DrawWeapon(); return;
			case 3: Watch(); return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WaitForCharacter()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (controller && controller.GetPlayerId() > 0 && GetGameState() == SCR_EGameModeState.GAME && SCR_ChimeraCharacter.Cast(controller.GetControlledEntity()) && GetPlayerCount() >= 2)
		{
			m_bVictim = controller.GetPlayerId() == GetPlayerId(0);
			Info(string.Format("Client of player %1, role %2", controller.GetPlayerId(), GetRoleName()));
			NextStep(5);
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetRoleName()
	{
		if (m_bVictim)
			return "victim";

		return "observer";
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawWeapon()
	{
		if (m_bVictim)
		{
			IEntity character = SCR_PlayerController.GetLocalControlledEntity();
			BaseWeaponManagerComponent weaponManager = GDC_MissionTest_Uncon.GetWeaponManager(character);
			CharacterControllerComponent controller = SCR_ChimeraCharacter.Cast(character).GetCharacterController();
			// The rifle is the case that matters: the first slot with a weapon can be the grenade slot
			array<WeaponSlotComponent> slots = {};
			weaponManager.GetWeaponsSlots(slots);
			foreach (WeaponSlotComponent slot : slots)
			{
				IEntity weapon = slot.GetWeaponEntity();
				if (!weapon)
					continue;

				BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent));
				if (!weaponComp || weaponComp.GetWeaponType() != EWeaponType.WT_RIFLE)
					continue;

				Info("Victim draws " + GetPrefabName(weapon));
				controller.SelectWeapon(slot);
				break;
			}
		}

		NextStep(1);
	}

	//------------------------------------------------------------------------------------------------
	//! Waits for the victim to fall unconscious, checks the tag once, then waits for the wake up
	protected void Watch()
	{
		IEntity victim = GetPlayerEntity(GetPlayerId(0));
		SCR_ChimeraCharacter victimCharacter = SCR_ChimeraCharacter.Cast(victim);
		if (!victimCharacter)
		{
			RepeatStep(1);
			return;
		}

		bool unconscious = victimCharacter.GetCharacterController().IsUnconscious();
		if (unconscious && !m_bWasUnconscious)
		{
			m_bWasUnconscious = true;
			// Leaves time to the tag to process the life state change
			GetGame().GetCallqueue().CallLater(CheckTag, 3000, false, victim);
		}

		if (m_bWasUnconscious && !unconscious)
		{
			Info("Victim woke up");
			Finish();
			return;
		}

		if (StepElapsed() > 180)
		{
			Check("Victim fell unconscious and woke up", false, string.Format("unconscious seen %1", m_bWasUnconscious));
			Finish();
			return;
		}

		RepeatStep(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckTag(IEntity victim)
	{
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.GetHUDManager();
		SCR_NameTagDisplay display;
		if (hud)
			display = SCR_NameTagDisplay.Cast(hud.FindInfoDisplay(SCR_NameTagDisplay));

		if (!display)
		{
			Warn("No name tag display on this client");
			return;
		}

		SCR_NameTagData tag;
		if (m_bVictim)
			tag = display.m_CurrentPlayerTag;
		else
			tag = display.GetEntityNameTag(victim);

		if (!tag)
		{
			Warn(GetRoleName() + ": no name tag for the victim (not in view), tag not checked");
			return;
		}

		Check(GetRoleName() + ": victim's tag has no unconscious state", !(tag.m_eEntityStateFlags & ENameTagEntityState.UNCONSCIOUS), string.Format("flags %1, priority %2", tag.m_eEntityStateFlags, typename.EnumToString(ENameTagEntityState, tag.m_ePriorityEntityState)));
	}
}
