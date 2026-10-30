// GDC_SupportMapPanel.c
// Path in mod: Scripts/Game/GDC/Support/GDC_SupportMapPanel.c

//! Why the panel has no usable target.
enum GDC_ESupportTargetError
{
	NONE,
	BAD_FORMAT,	//!< The text is not grid coordinates
	OFF_MAP		//!< The position is outside the map
}

//! Target marker: a cross whose center is exactly on the target.
//! The vanilla marker layout puts its icon at the top of the widget, away from the marker position.
class GDC_SupportTargetMarker : SCR_MapMarkerBase
{
	protected const ResourceName ICONS = "{E23427CAC80DA8B7}UI/Textures/Icons/icons_mapMarkersUI.imageset";
	protected const string ICON = "cross";

	protected bool m_bIconCentered;

	//------------------------------------------------------------------------------------------------
	override void OnCreateMarker(bool skipProfanityFilter = false)
	{
		super.OnCreateMarker(skipProfanityFilter);

		m_bIconCentered = false;
		if (m_MarkerWidgetComp)
			m_MarkerWidgetComp.SetImage(ICONS, ICON);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnUpdate(vector visibleMin = vector.Zero, vector visibleMax = vector.Zero)
	{
		bool isVisible = super.OnUpdate(visibleMin, visibleMax);

		if (isVisible && !m_bIconCentered)
			CenterIcon();

		return isVisible;
	}

	//------------------------------------------------------------------------------------------------
	//! Moves the content of the marker so that the icon is at its center. Widget sizes are only known once they have been drawn.
	protected void CenterIcon()
	{
		if (!m_wRoot)
			return;

		Widget icon = m_wRoot.FindAnyWidget("MarkerIcon");
		Widget content = m_wRoot.FindAnyWidget("VerticalLayout0");
		if (!icon || !content)
			return;

		float rootX, rootY, rootSizeX, rootSizeY;
		m_wRoot.GetScreenPos(rootX, rootY);
		m_wRoot.GetScreenSize(rootSizeX, rootSizeY);

		float iconX, iconY, iconSizeX, iconSizeY;
		icon.GetScreenPos(iconX, iconY);
		icon.GetScreenSize(iconSizeX, iconSizeY);
		if (iconSizeX <= 0 || iconSizeY <= 0)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		vector contentPos = FrameSlot.GetPos(content);
		float offsetX = workspace.DPIUnscale(rootX + rootSizeX * 0.5 - iconX - iconSizeX * 0.5);
		float offsetY = workspace.DPIUnscale(rootY + rootSizeY * 0.5 - iconY - iconSizeY * 0.5);
		FrameSlot.SetPos(content, contentPos[0] + offsetX, contentPos[1] + offsetY);
		m_bIconCentered = true;
	}
}

//! Support request panel of the map. Purely local: its checks only drive the display, the server decides.
//! It doesn't block the map, so the player can pan, zoom and click to choose a target while it is open.
class GDC_SupportMapPanel : Managed
{
	protected const ResourceName LAYOUT = "{7C2E91A4D05B36F8}UI/layouts/Support/GDC_SupportPanel.layout";
	protected const ResourceName CIRCLE_LAYOUT = "{3B7F16C8E92A04D5}UI/layouts/Support/GDC_SupportCircle.layout";
	protected const int CIRCLE_SEGMENTS = 64;
	protected const int MARKER_COLOR_RED = 4;
	protected const float CIRCLE_LINE_WIDTH = 3;
	protected const float REFRESH_DELAY = 0.25;
	protected const int CLICK_TOLERANCE = 5;

	//! Markers of the missions under way, by support name. They stay after the panel is closed.
	protected static ref map<string, ref SCR_MapMarkerBase> s_mMissionMarkers = new map<string, ref SCR_MapMarkerBase>();

	protected SCR_MapEntity m_MapEntity;
	protected SCR_MapCursorModule m_CursorModule;
	protected SCR_MapMarkerManagerComponent m_MarkerManager;

	protected Widget m_wRoot;
	protected CanvasWidget m_wCircle;
	protected ref LineDrawCommand m_CircleLine;
	protected ref array<ref CanvasWidgetCommand> m_aCircleCommands;
	protected RichTextWidget m_wStatus;
	protected ref Color m_StatusDefaultColor;
	protected RichTextWidget m_wMissions;
	protected SCR_ComboBoxComponent m_ComboSupport;
	protected SCR_ComboBoxComponent m_ComboOption;
	protected SCR_SliderComponent m_SliderQuantity;
	protected SCR_SliderComponent m_SliderSpread;
	protected SCR_EditBoxComponent m_EditTarget;
	protected SCR_InputButtonComponent m_ButtonFire;

	protected GDC_SupportModuleComponent m_Module;

	//! Modules listed in the panel: those the player has access to when opening it.
	protected ref array<GDC_SupportModuleComponent> m_aModules = {};
	protected ref SCR_MapMarkerBase m_TargetMarker;
	protected vector m_vTarget;
	protected bool m_bHasTarget;
	protected GDC_ESupportTargetError m_eTargetError;
	protected bool m_bIgnoreEmptyText;
	protected float m_fRefreshTimer;
	protected int m_iClickX;
	protected int m_iClickY;

	#ifdef DEBUG
	protected string m_sLastStatus;
	#endif

	//------------------------------------------------------------------------------------------------
	void GDC_SupportMapPanel(notnull SCR_MapEntity mapEntity, SCR_MapCursorModule cursorModule, SCR_MapMarkerManagerComponent markerManager)
	{
		m_MapEntity = mapEntity;
		m_CursorModule = cursorModule;
		m_MarkerManager = markerManager;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOpen()
	{
		return m_wRoot != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Opens the panel, or just moves the target if it is already open.
	//! \param parent Map root widget
	void Open(notnull Widget parent, vector target)
	{
		if (!m_wRoot && !CreateWidgets(parent))
			return;

		if (IsArcade())
			SetTarget(target, true);

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected bool CreateWidgets(Widget parent)
	{
		GDC_SupportModuleComponent.GetAccessibleModules(SCR_PlayerController.GetLocalControlledEntity(), m_aModules);
		if (m_aModules.IsEmpty())
			return false;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, parent);
		if (!m_wRoot)
			return false;

		float screenX, screenY;
		m_MapEntity.GetMapWidget().GetScreenSize(screenX, screenY);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		FrameSlot.SetPos(m_wRoot, workspace.DPIUnscale(screenX) - 520, workspace.DPIUnscale(screenY) * 0.2);

		m_wStatus = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Status"));
		m_StatusDefaultColor = m_wStatus.GetColor();
		m_wMissions = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Missions"));
		m_ComboSupport = SCR_ComboBoxComponent.Cast(m_wRoot.FindAnyWidget("ComboSupport").FindHandler(SCR_ComboBoxComponent));
		m_ComboOption = SCR_ComboBoxComponent.Cast(m_wRoot.FindAnyWidget("ComboOption").FindHandler(SCR_ComboBoxComponent));
		m_SliderQuantity = SCR_SliderComponent.Cast(m_wRoot.FindAnyWidget("SliderQuantity").FindHandler(SCR_SliderComponent));
		m_SliderSpread = SCR_SliderComponent.Cast(m_wRoot.FindAnyWidget("SliderSpread").FindHandler(SCR_SliderComponent));
		m_EditTarget = SCR_EditBoxComponent.Cast(m_wRoot.FindAnyWidget("EditTarget").FindHandler(SCR_EditBoxComponent));
		m_ButtonFire = SCR_InputButtonComponent.Cast(m_wRoot.FindAnyWidget("ButtonFire").FindHandler(SCR_InputButtonComponent));

		SCR_InputButtonComponent buttonCancel = SCR_InputButtonComponent.Cast(m_wRoot.FindAnyWidget("ButtonCancel").FindHandler(SCR_InputButtonComponent));
		buttonCancel.m_OnClicked.Insert(OnCancelClicked);
		m_ButtonFire.m_OnClicked.Insert(OnFireClicked);

		// The map must not react to the keyboard while typing, but stays usable the rest of the time.
		m_EditTarget.m_OnWriteModeEnter.Insert(OnWriteModeEnter);
		m_EditTarget.m_OnWriteModeLeave.Insert(OnWriteModeLeave);
		m_EditTarget.m_OnTextChange.Insert(OnTargetTextChanged);

		foreach (GDC_SupportModuleComponent module : m_aModules)
		{
			m_ComboSupport.AddItem(module.GetDisplayName());
		}

		m_ComboSupport.SetCurrentItem(0, false, false, false);
		m_ComboSupport.m_OnChanged.Insert(OnSupportChanged);
		m_wRoot.FindAnyWidget("ComboSupport").SetVisible(m_aModules.Count() > 1);
		CreateCircle();
		SelectModule(m_aModules[0]);

		InputManager inputManager = GetGame().GetInputManager();
		inputManager.AddActionListener("MapSelect", EActionTrigger.DOWN, OnMapSelectDown);
		inputManager.AddActionListener("MapSelect", EActionTrigger.UP, OnMapSelectUp);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Creates the canvas of the area of effect, under the rest of the map interface.
	protected void CreateCircle()
	{
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			return;

		m_wCircle = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidgets(CIRCLE_LAYOUT, mapFrame));
		if (!m_wCircle)
			return;

		m_CircleLine = new LineDrawCommand();
		m_CircleLine.m_iColor = ARGB(220, 255, 60, 40);
		m_CircleLine.m_fWidth = CIRCLE_LINE_WIDTH;
		m_CircleLine.m_bShouldEnclose = true;
		m_CircleLine.m_Vertices = {};

		m_aCircleCommands = {};
		m_aCircleCommands.Insert(m_CircleLine);
		m_wCircle.SetDrawCommands(m_aCircleCommands);
	}

	//------------------------------------------------------------------------------------------------
	//! Closes the panel. The target marker is removed, unless a mission was requested on it.
	void Close()
	{
		if (!m_wRoot)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		inputManager.RemoveActionListener("MapSelect", EActionTrigger.DOWN, OnMapSelectDown);
		inputManager.RemoveActionListener("MapSelect", EActionTrigger.UP, OnMapSelectUp);

		if (m_CursorModule)
			m_CursorModule.HandleDialog(false);

		// Removing the widgets makes the edit box send a last change, which must be ignored.
		m_EditTarget.m_OnWriteModeEnter.Remove(OnWriteModeEnter);
		m_EditTarget.m_OnWriteModeLeave.Remove(OnWriteModeLeave);
		m_EditTarget.m_OnTextChange.Remove(OnTargetTextChanged);

		RemoveTargetMarker();

		if (m_wCircle)
			m_wCircle.RemoveFromHierarchy();

		m_wCircle = null;
		m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
		m_bHasTarget = false;
		m_eTargetError = GDC_ESupportTargetError.NONE;
		m_bIgnoreEmptyText = false;
	}

	//------------------------------------------------------------------------------------------------
	void Update(float timeSlice)
	{
		if (!m_wRoot)
			return;

		UpdateCircle();

		m_fRefreshTimer += timeSlice;
		if (m_fRefreshTimer < REFRESH_DELAY)
			return;

		m_fRefreshTimer = 0;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Redraws the area of effect, which follows the pan and the zoom of the map.
	protected void UpdateCircle()
	{
		if (!m_wCircle)
			return;

		bool isVisible = IsArcade() && m_bHasTarget;
		m_wCircle.SetVisible(isVisible);
		if (!isVisible)
			return;

		int radius = GetSpread();
		float angleStep = Math.PI2 / CIRCLE_SEGMENTS;

		m_CircleLine.m_Vertices.Clear();
		for (int i = 0; i < CIRCLE_SEGMENTS; i++)
		{
			float angle = i * angleStep;
			int screenX, screenY;
			m_MapEntity.WorldToScreen(m_vTarget[0] + Math.Cos(angle) * radius, m_vTarget[2] + Math.Sin(angle) * radius, screenX, screenY, true);
			m_CircleLine.m_Vertices.Insert(screenX);
			m_CircleLine.m_Vertices.Insert(screenY);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Removes the marker of a mission that is over. Safe if there is none.
	static void RemoveMissionMarker(string supportName)
	{
		SCR_MapMarkerBase marker = s_mMissionMarkers.Get(supportName);
		if (!marker)
			return;

		s_mMissionMarkers.Remove(supportName);

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			DeleteMarker(markerManager, marker);
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectModule(GDC_SupportModuleComponent module)
	{
		m_Module = module;

		array<string> options = {};
		module.GetOptionNames(options);

		m_ComboOption.ClearAll();
		foreach (string option : options)
		{
			m_ComboOption.AddItem(option);
		}

		if (!options.IsEmpty())
			m_ComboOption.SetCurrentItem(0, false, false, false);

		m_wRoot.FindAnyWidget("ComboOption").SetVisible(options.Count() > 1);

		int maxQuantity = module.GetMaxQuantity();
		m_SliderQuantity.SetSliderSettings(1, maxQuantity, 1);
		m_SliderQuantity.SetValue(maxQuantity);
		m_wRoot.FindAnyWidget("SliderQuantity").SetVisible(maxQuantity > 1);

		int minSpread = module.GetMinSpread();
		int maxSpread = module.GetMaxSpread();
		m_SliderSpread.SetSliderSettings(minSpread, maxSpread, 1);
		m_SliderSpread.SetValue(Math.Max(minSpread, maxSpread * 0.5));
		m_wRoot.FindAnyWidget("SliderSpread").SetVisible(maxSpread > minSpread);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSupportChanged(SCR_ComboBoxComponent comp, int index)
	{
		if (!m_aModules.IsIndexValid(index))
			return;

		SelectModule(m_aModules[index]);

		// The target of a mode must not leak into the other one.
		if (IsArcade())
		{
			if (m_bHasTarget)
				SetTarget(m_vTarget, false);
		}
		else
		{
			ClearTarget();
			m_EditTarget.SetValue(string.Empty);
		}

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsArcade()
	{
		return m_Module && m_Module.GetTargetingMode() == GDC_ESupportTargetingMode.ARCADE;
	}

	//------------------------------------------------------------------------------------------------
	//! \param updateText Whether to write the position in the coordinates box
	protected void SetTarget(vector target, bool updateText)
	{
		m_vTarget = target;
		m_vTarget[1] = GetGame().GetWorld().GetSurfaceY(target[0], target[2]);
		m_bHasTarget = true;
		m_eTargetError = GDC_ESupportTargetError.NONE;

		if (updateText)
			m_EditTarget.SetValue(FormatGrid(m_vTarget));

		if (!m_MarkerManager || !IsArcade())
			return;

		// A single marker is moved around. Enabling it makes the manager check its new position at the next update.
		if (m_TargetMarker)
		{
			m_TargetMarker.SetWorldPos(m_vTarget[0], m_vTarget[2]);
			m_MarkerManager.SetStaticMarkerDisabled(m_TargetMarker, false);
			return;
		}

		m_TargetMarker = new GDC_SupportTargetMarker();
		m_TargetMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		m_TargetMarker.SetColorEntry(MARKER_COLOR_RED);
		m_TargetMarker.SetCustomText("Cible");
		m_TargetMarker.SetWorldPos(m_vTarget[0], m_vTarget[2]);
		m_TargetMarker.SetCanBeRemovedByOwner(false);
		m_MarkerManager.InsertStaticMarker(m_TargetMarker, true);

		// Without an owner, the player can neither edit nor drag it. The insertion sets the owner, hence this order.
		m_TargetMarker.SetMarkerOwnerID(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearTarget()
	{
		RemoveTargetMarker();
		m_bHasTarget = false;
		m_eTargetError = GDC_ESupportTargetError.NONE;
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveTargetMarker()
	{
		if (m_TargetMarker && m_MarkerManager)
			DeleteMarker(m_MarkerManager, m_TargetMarker);

		m_TargetMarker = null;
	}

	//------------------------------------------------------------------------------------------------
	//! The manager only removes enabled markers. An off-screen marker is disabled: it would stay in
	//! its disabled list and come back when the map shows its position again.
	protected static void DeleteMarker(notnull SCR_MapMarkerManagerComponent markerManager, notnull SCR_MapMarkerBase marker)
	{
		markerManager.SetStaticMarkerDisabled(marker, false);
		markerManager.RemoveStaticMarker(marker);
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_wRoot || !m_Module)
			return;

		// Access lost while the panel is open (item dropped...): the support disappears, without a message.
		if (m_Module.CheckAccess(SCR_PlayerController.GetLocalControlledEntity()) != GDC_ESupportRefusal.NONE)
		{
			Close();
			return;
		}

		GDC_ESupportRefusal refusal = GDC_ESupportRefusal.NONE;
		string status;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity character;
		if (controller)
			character = controller.GetControlledEntity();

		if (m_eTargetError == GDC_ESupportTargetError.BAD_FORMAT)
		{
			status = "Format invalide : saisis 6, 8 ou 10 chiffres.";
		}
		else if (m_eTargetError == GDC_ESupportTargetError.OFF_MAP)
		{
			status = "Coordonnées hors de la carte.";
		}
		else if (!m_bHasTarget)
		{
			// The state of the support is shown as soon as the panel opens, without waiting for a target.
			refusal = m_Module.CheckAvailability();
			if (refusal != GDC_ESupportRefusal.NONE && controller)
				status = GetUnavailableText(controller, refusal);
			else if (IsArcade())
				status = "Clique sur la carte ou saisis des coordonnées.";
			else
				status = "Saisis les coordonnées de la cible.";
		}
		else
		{
			refusal = m_Module.CheckRequest(character, m_vTarget, GetOption(), GetQuantity(), GetSpread());

			// A problem with the target comes first, whatever the state of the support.
			if (!m_Module.IsTargetAllowed(m_vTarget))
			{
				refusal = GDC_ESupportRefusal.TARGET_FORBIDDEN;
				status = "Zone d'interdiction de tir.";
			}
			else if (!m_Module.IsTargetInRange(m_vTarget, GetOption()))
			{
				refusal = GDC_ESupportRefusal.OUT_OF_RANGE;
				status = "Cible hors de portée.";
			}
			else if (refusal != GDC_ESupportRefusal.NONE && controller)
				status = GetUnavailableText(controller, refusal);
			else if (!IsArcade())
				status = "Coordonnées valides.";
			else if (character)
				status = string.Format("Cible valide, à %1 m de toi.", Math.Round(vector.DistanceXZ(character.GetOrigin(), m_vTarget)));
		}

		#ifdef DEBUG
		if (status != m_sLastStatus)
			Print(string.Format("[GDC_SupportPanel] Status: %1 (mode %2, rounds %3, spread %4)", status, typename.EnumToString(GDC_ESupportTargetingMode, m_Module.GetTargetingMode()), GetQuantity(), GetSpread()));

		m_sLastStatus = status;
		#endif

		m_wStatus.SetText(status);

		// Orange: the player only has to wait. Red: something has to change.
		if (refusal == GDC_ESupportRefusal.COOLDOWN || refusal == GDC_ESupportRefusal.MISSION_ACTIVE)
			m_wStatus.SetColor(Color.FromRGBA(255, 165, 40, 255));
		else if (m_eTargetError != GDC_ESupportTargetError.NONE || refusal != GDC_ESupportRefusal.NONE)
			m_wStatus.SetColor(Color.FromRGBA(255, 70, 60, 255));
		else
			m_wStatus.SetColor(m_StatusDefaultColor);

		int missionsLeft = m_Module.GetMissionsLeft();
		if (missionsLeft < 0)
			m_wMissions.SetText("Missions restantes : illimitées");
		else
			m_wMissions.SetText(string.Format("Missions restantes : %1", missionsLeft));

		m_ButtonFire.SetEnabled(m_bHasTarget && refusal == GDC_ESupportRefusal.NONE);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetUnavailableText(notnull SCR_PlayerController controller, GDC_ESupportRefusal refusal)
	{
		if (refusal == GDC_ESupportRefusal.COOLDOWN)
			return string.Format("Indisponible pendant %1 s.", Math.Ceil(m_Module.GetCooldownRemaining()));

		return "Indisponible : " + controller.GDC_GetSupportRefusalText(refusal, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetOption()
	{
		int option = m_ComboOption.GetCurrentIndex();
		if (option < 0)
			return 0;

		return option;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetQuantity()
	{
		if (m_Module.GetMaxQuantity() <= 1)
			return 1;

		return Math.Round(m_SliderQuantity.GetValue());
	}

	//------------------------------------------------------------------------------------------------
	protected int GetSpread()
	{
		if (m_Module.GetMaxSpread() <= m_Module.GetMinSpread())
			return m_Module.GetMinSpread();

		return Math.Round(m_SliderSpread.GetValue());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCancelClicked()
	{
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFireClicked()
	{
		if (!m_bHasTarget)
			return;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
			return;

		#ifdef DEBUG
		Print(string.Format("[GDC_SupportPanel] Fire: '%1' on %2, option %3, rounds %4, spread %5", m_Module.GetDisplayName(), m_vTarget, GetOption(), GetQuantity(), GetSpread()));
		#endif

		controller.GDC_RequestSupport(m_Module, m_vTarget, GetOption(), GetQuantity(), GetSpread());

		// The marker now belongs to the mission, until the server says it is over.
		RemoveMissionMarker(m_Module.GetDisplayName());
		if (m_TargetMarker)
			s_mMissionMarkers.Set(m_Module.GetDisplayName(), m_TargetMarker);

		m_TargetMarker = null;
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWriteModeEnter()
	{
		if (m_CursorModule)
			m_CursorModule.HandleDialog(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWriteModeLeave(string text)
	{
		if (m_CursorModule)
			m_CursorModule.HandleDialog(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTargetTextChanged(string text)
	{
		// The box was just emptied by a click outside the map: that error must stay.
		if (text.IsEmpty() && m_bIgnoreEmptyText)
		{
			m_bIgnoreEmptyText = false;
			return;
		}

		vector target;
		bool isValid = ParseGrid(text, target);
		if (isValid && IsOnMap(target))
		{
			SetTarget(target, false);
		}
		else
		{
			ClearTarget();
			if (!text.IsEmpty())
			{
				if (isValid)
					m_eTargetError = GDC_ESupportTargetError.OFF_MAP;
				else
					m_eTargetError = GDC_ESupportTargetError.BAD_FORMAT;
			}
		}

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapSelectDown()
	{
		m_iClickX = SCR_MapCursorInfo.x;
		m_iClickY = SCR_MapCursorInfo.y;
	}

	//------------------------------------------------------------------------------------------------
	//! A click on the map moves the target. A drag or a click on an interface element doesn't.
	protected void OnMapSelectUp()
	{
		if (!m_wRoot || !m_CursorModule || !IsArcade())
			return;

		if (m_CursorModule.GetCursorState() & SCR_MapCursorModule.STATE_POPUP_RESTRICTED)
			return;

		if (Math.AbsInt(SCR_MapCursorInfo.x - m_iClickX) > CLICK_TOLERANCE || Math.AbsInt(SCR_MapCursorInfo.y - m_iClickY) > CLICK_TOLERANCE)
			return;

		if (IsCursorOverInterface())
			return;

		float worldX, worldZ;
		m_MapEntity.GetMapCursorWorldPosition(worldX, worldZ);

		vector target = Vector(worldX, 0, worldZ);
		if (IsOnMap(target))
		{
			SetTarget(target, true);
		}
		else
		{
			ClearTarget();
			m_eTargetError = GDC_ESupportTargetError.OFF_MAP;
			m_bIgnoreEmptyText = !m_EditTarget.GetValue().IsEmpty();
			m_EditTarget.SetValue(string.Empty);
		}

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCursorOverInterface()
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		foreach (Widget widget : widgets)
		{
			if (BelongsToInterface(widget))
				return true;
		}

		// Parts of the panel that can't be traced (background...) are covered by its geometry.
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float cursorX = workspace.DPIScale(SCR_MapCursorInfo.x);
		float cursorY = workspace.DPIScale(SCR_MapCursorInfo.y);

		float posX, posY, sizeX, sizeY;
		m_wRoot.GetScreenPos(posX, posY);
		m_wRoot.GetScreenSize(sizeX, sizeY);
		return cursorX >= posX && cursorX <= posX + sizeX && cursorY >= posY && cursorY <= posY + sizeY;
	}

	//------------------------------------------------------------------------------------------------
	//! Tells interface elements from the map, by the names of the widgets of the map menu.
	//! An unknown element counts as the map: at worst a click on it moves the target, whereas a
	//! list of map widgets would make the map unclickable as soon as a mod adds a layer to it.
	protected bool BelongsToInterface(Widget widget)
	{
		Widget current = widget;
		while (current)
		{
			if (current == m_wRoot)
				return true;

			string name = current.GetName();
			if (name == "ToolMenu" || name == "CompassFrame" || name == "MissionDescriptionFrame")
				return true;

			// These containers cover the whole map: only what they contain is interface.
			if (current != widget && (name == "ToolFramesOverlay" || name == "JournalFrame" || name == "MapWeatherFrame"))
				return true;

			current = current.GetParent();
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsOnMap(vector position)
	{
		return position[0] >= 0 && position[0] <= m_MapEntity.GetMapSizeX() && position[2] >= 0 && position[2] <= m_MapEntity.GetMapSizeY();
	}

	//------------------------------------------------------------------------------------------------
	//! Converts grid coordinates to a position, at the center of the designated square.
	//! Accepts 6 digits (100 m), 8 digits (10 m) or 10 digits (1 m), with or without spaces.
	//! \return false if the text is not valid coordinates
	static bool ParseGrid(string text, out vector position)
	{
		string digits;
		for (int i = 0, length = text.Length(); i < length; i++)
		{
			string character = text.Get(i);
			if (character == " ")
				continue;

			if (!"0123456789".Contains(character))
				return false;

			digits += character;
		}

		int digitCount = digits.Length();
		if (digitCount != 6 && digitCount != 8 && digitCount != 10)
			return false;

		int half = digitCount / 2;
		int squareSize = Math.Pow(10, 5 - half);
		float offset = squareSize * 0.5;
		if (squareSize == 1)
			offset = 0;

		position[0] = digits.Substring(0, half).ToInt() * squareSize + offset;
		position[1] = 0;
		position[2] = digits.Substring(half, half).ToInt() * squareSize + offset;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \return The position as 8-digit grid coordinates (10 m), e.g. "1086 2453"
	static string FormatGrid(vector position)
	{
		return FormatGridPart(position[0]) + " " + FormatGridPart(position[2]);
	}

	//------------------------------------------------------------------------------------------------
	protected static string FormatGridPart(float value)
	{
		int square = value / 10;
		string text = square.ToString();
		while (text.Length() < 4)
		{
			text = "0" + text;
		}

		return text;
	}
}
