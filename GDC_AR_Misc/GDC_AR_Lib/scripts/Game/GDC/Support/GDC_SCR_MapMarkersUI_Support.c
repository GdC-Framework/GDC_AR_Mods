// GDC_SCR_MapMarkersUI_Support.c
// Path in mod: Scripts/Game/GDC/Support/GDC_SCR_MapMarkersUI_Support.c

//! Adds the support entry to the radial menu of the map, for players who have access to a support.
modded class SCR_MapMarkersUI
{
	protected const ResourceName GDC_SUPPORT_ICONS = "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset";
	protected const string GDC_SUPPORT_ICON = "artilleryFireMission";
	protected const float GDC_SUPPORT_ACCESS_DELAY = 0.25;

	protected ref GDC_SupportMapPanel m_GDC_SupportPanel;
	protected SCR_SelectionMenuEntry m_GDC_SupportEntry;
	protected bool m_bGDC_RadialMenuReady;
	protected float m_fGDC_SupportAccessTimer;

	//------------------------------------------------------------------------------------------------
	//! The game clears the menu then fills it here, the first time it opens: entries added before are lost.
	override protected void OnRadialMenuInit()
	{
		super.OnRadialMenuInit();

		m_bGDC_RadialMenuReady = true;
		m_GDC_SupportEntry = null;
		GDC_UpdateSupportEntry();
	}

	//------------------------------------------------------------------------------------------------
	//! The entry only exists for a player who has access to at least one support.
	protected void GDC_UpdateSupportEntry()
	{
		if (!m_bGDC_RadialMenuReady)
			return;

		SCR_MapRadialUI radialUI = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (!radialUI)
			return;

		// The content of a menu being displayed must not change.
		SCR_RadialMenu radialMenu = radialUI.GetRadialController().GetRadialMenu();
		if (radialMenu && radialMenu.IsOpened())
			return;

		array<GDC_SupportModuleComponent> modules = {};
		GDC_SupportModuleComponent.GetAccessibleModules(SCR_PlayerController.GetLocalControlledEntity(), modules);
		bool hasAccess = !modules.IsEmpty();
		if (hasAccess == (m_GDC_SupportEntry != null))
			return;

		#ifdef DEBUG
		Print(string.Format("[GDC_SupportPanel] Radial entry available: %1", hasAccess));
		#endif

		if (hasAccess)
		{
			m_GDC_SupportEntry = radialUI.AddRadialEntry("Appui feu");
			m_GDC_SupportEntry.SetIcon(GDC_SUPPORT_ICONS, GDC_SUPPORT_ICON);
			m_GDC_SupportEntry.GetOnPerform().Insert(GDC_OnSupportRadialPerformed);
		}
		else
		{
			radialUI.RemoveRadialEntry(m_GDC_SupportEntry);
			m_GDC_SupportEntry = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GDC_OnSupportRadialPerformed(SCR_SelectionMenuEntry entry)
	{
		SCR_MapRadialUI radialUI = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (radialUI)
			GDC_GetSupportPanel().Open(m_RootWidget, radialUI.GetMenuWorldPosition());
	}

	//------------------------------------------------------------------------------------------------
	protected GDC_SupportMapPanel GDC_GetSupportPanel()
	{
		if (!m_GDC_SupportPanel)
			m_GDC_SupportPanel = new GDC_SupportMapPanel(m_MapEntity, m_CursorModule, m_MarkerMgr);

		return m_GDC_SupportPanel;
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		if (m_GDC_SupportPanel)
			m_GDC_SupportPanel.Close();

		super.OnMapClose(config);
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);

		if (m_GDC_SupportPanel)
			m_GDC_SupportPanel.Update(timeSlice);

		m_fGDC_SupportAccessTimer += timeSlice;
		if (m_fGDC_SupportAccessTimer >= GDC_SUPPORT_ACCESS_DELAY)
		{
			m_fGDC_SupportAccessTimer = 0;
			GDC_UpdateSupportEntry();
		}
	}
}
