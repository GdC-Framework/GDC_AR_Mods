// GDC_MarkerIconEntity.c
// Path in mod: Scripts/Game/GDC/Markers/GDC_MarkerIconEntity.c

[EntityEditorProps(category: "GDC/Markers", description: "Icon shown on the in-game map, with a label and a description on hover.", sizeMin: "-1 0 -1", sizeMax: "1 2 1")]
class GDC_MarkerIconEntityClass : GenericEntityClass
{
}

//! Shows the description of a marker icon while the mouse is over it.
class GDC_MarkerIconHoverHandler : ScriptedWidgetEventHandler
{
	protected Widget m_wDescriptionPanel;

	//------------------------------------------------------------------------------------------------
	void GDC_MarkerIconHoverHandler(Widget descriptionPanel)
	{
		m_wDescriptionPanel = descriptionPanel;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_wDescriptionPanel)
			m_wDescriptionPanel.SetVisible(true);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_wDescriptionPanel)
			m_wDescriptionPanel.SetVisible(false);

		return true;
	}
}

//! Fixed icon shown on the in-game map: an icon of an image set, a label under it and a description on hover.
//! Purely local: every machine shows it from the world entity, nothing is replicated.
//! Each marker prefab sets the image set (one prefab per image set); the mission maker sets the rest.
class GDC_MarkerIconEntity : GenericEntity
{
	[Attribute("{E23427CAC80DA8B7}UI/Textures/Icons/icons_mapMarkersUI.imageset", UIWidgets.ResourceNamePicker, "Image set holding the icon. Set by the marker prefab.", "imageset", category: "Icon")]
	protected ResourceName m_sImageSet;

	[Attribute("point-of-interest", UIWidgets.EditBox, "Name of the icon in the image set. Open the image set to find it.", category: "Icon")]
	protected string m_sIconName;

	[Attribute("0 0 0 1", UIWidgets.ColorPicker, "Color of the icon.", category: "Icon")]
	protected ref Color m_Color;

	[Attribute("32", UIWidgets.EditBox, "Size of the icon: in meters if 'Size In Meters' is checked, else in pixels on the screen.", "1 100000", category: "Icon")]
	protected float m_fSize;

	[Attribute("0", UIWidgets.CheckBox, "Checked: the size is in meters and the icon grows with the zoom. Unchecked: the icon keeps the same size on the screen.", category: "Icon")]
	protected bool m_bSizeInMeters;

	[Attribute("16", UIWidgets.EditBox, "Smallest size on the screen, in pixels, when the size is in meters. 0 = no minimum.", "0 1000", category: "Icon")]
	protected float m_fMinScreenSize;

	[Attribute("0", UIWidgets.CheckBox, "The icon turns with the entity (arrows). An icon pointing right on its image points to where the entity faces.", category: "Icon")]
	protected bool m_bFollowRotation;

	[Attribute("", UIWidgets.EditBox, "Text always shown under the icon.", category: "Text")]
	protected string m_sLabel;

	[Attribute("", UIWidgets.EditBoxMultiline, "Text shown when the mouse is over the icon.", category: "Text")]
	protected string m_sDescription;

	[Attribute("", UIWidgets.Auto, "Faction keys of the players who see the marker (e.g. US, USSR, FIA). Empty: every player.", category: "Visibility")]
	protected ref array<string> m_aFactionKeys;

	protected static ref array<GDC_MarkerIconEntity> s_aShownMarkers = {};

	protected const ResourceName LAYOUT = "{2AD1A1AD9AC928D0}UI/layouts/Markers/GDC_MarkerIcon.layout";
	protected const float LABEL_GAP = 2;
	protected const float DESCRIPTION_GAP = 4;

	protected Widget m_wRoot;
	protected ImageWidget m_wIcon;
	protected Widget m_wLabel;
	protected Widget m_wDescriptionPanel;
	protected ref GDC_MarkerIconHoverHandler m_HoverHandler;

	//! Height / width of the icon image, to keep its proportions.
	protected float m_fIconRatio = 1;

	//------------------------------------------------------------------------------------------------
	//! \return The markers of the current world shown on the map of this machine
	static array<GDC_MarkerIconEntity> GetShownMarkers()
	{
		return s_aShownMarkers;
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if the local player may see this marker
	bool IsShownToLocalPlayer()
	{
		if (!m_aFactionKeys || m_aFactionKeys.IsEmpty())
			return true;

		if (!GetGame().GetFactionManager())
			return false;

		Faction faction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (!faction)
			return false;

		return m_aFactionKeys.Contains(faction.GetFactionKey());
	}

	//------------------------------------------------------------------------------------------------
	//! Creates the widget of the marker in the map frame, if the local player may see it.
	void CreateMapWidget(notnull SCR_MapEntity mapEntity, notnull Widget mapFrame)
	{
		if (m_wRoot || !IsShownToLocalPlayer())
			return;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, mapFrame);
		if (!m_wRoot)
			return;

		m_wIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("Icon"));
		m_wLabel = m_wRoot.FindAnyWidget("Label");
		m_wDescriptionPanel = m_wRoot.FindAnyWidget("DescriptionPanel");

		if (m_wIcon)
		{
			if (!m_wIcon.LoadImageFromSet(0, m_sImageSet, m_sIconName))
				Print(string.Format("[GDC_Marker] %1: no icon '%2' in %3.", this, m_sIconName, m_sImageSet), LogLevel.WARNING);

			m_wIcon.SetColor(m_Color);

			int imageWidth, imageHeight;
			m_wIcon.GetImageSize(0, imageWidth, imageHeight);
			if (imageWidth > 0 && imageHeight > 0)
				m_fIconRatio = (float)imageHeight / (float)imageWidth;

			m_HoverHandler = new GDC_MarkerIconHoverHandler(m_wDescriptionPanel);
			m_wIcon.AddHandler(m_HoverHandler);
		}

		TextWidget label = TextWidget.Cast(m_wLabel);
		if (label)
		{
			label.SetText(m_sLabel);
			label.SetVisible(!m_sLabel.IsEmpty());
		}

		RichTextWidget descriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionText"));
		if (descriptionText)
			descriptionText.SetText(m_sDescription);

		if (m_wDescriptionPanel)
			m_wDescriptionPanel.SetVisible(false);

		if (m_sDescription.IsEmpty())
			m_HoverHandler = null;

		UpdateMapWidget(mapEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Follows the pan and the zoom of the map.
	void UpdateMapWidget(notnull SCR_MapEntity mapEntity)
	{
		if (!m_wRoot || !m_wIcon)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		vector origin = GetOrigin();

		int screenX, screenY;
		mapEntity.WorldToScreen(origin[0], origin[2], screenX, screenY, true);
		FrameSlot.SetPos(m_wRoot, workspace.DPIUnscale(screenX), workspace.DPIUnscale(screenY));

		float width = m_fSize;
		if (m_bSizeInMeters)
		{
			int sizeEndX, sizeEndY;
			mapEntity.WorldToScreen(origin[0] + m_fSize, origin[2], sizeEndX, sizeEndY, true);
			width = workspace.DPIUnscale(Math.AbsFloat(sizeEndX - screenX));
			if (width < m_fMinScreenSize)
				width = m_fMinScreenSize;
		}

		float height = width * m_fIconRatio;
		FrameSlot.SetSize(m_wIcon, width, height);

		// Icons pointing right on their image point to where the entity faces
		if (m_bFollowRotation)
			m_wIcon.SetRotation(GetYawPitchRoll()[0] - 90);

		if (m_wLabel)
			FrameSlot.SetPos(m_wLabel, 0, height * 0.5 + LABEL_GAP);

		if (m_wDescriptionPanel)
			FrameSlot.SetPos(m_wDescriptionPanel, 0, -height * 0.5 - DESCRIPTION_GAP);
	}

	//------------------------------------------------------------------------------------------------
	void DeleteMapWidget()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wIcon = null;
		m_wLabel = null;
		m_wDescriptionPanel = null;
		m_HoverHandler = null;
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		if (GetGame().InPlayMode() && !System.IsConsoleApp())
			s_aShownMarkers.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	void GDC_MarkerIconEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	void ~GDC_MarkerIconEntity()
	{
		DeleteMapWidget();
		if (s_aShownMarkers)
			s_aShownMarkers.RemoveItem(this);
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_VISIBLE;
	}

	//------------------------------------------------------------------------------------------------
	//! Shows the icon name and the label above the entity in the World Editor, the entity having no model.
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		WorldEditorAPI api = _WB_GetEditorAPI();
		if (!api)
			return;

		IEntitySource src = api.EntityToSource(this);
		if (!src || !api.IsEntityLayerVisible(src.GetSubScene(), src.GetLayerID()))
			return;

		string text = m_sIconName;
		if (!m_sLabel.IsEmpty())
			text = string.Format("%1 - %2", m_sIconName, m_sLabel);

		vector cameraTransform[4];
		GetWorld().GetCurrentCamera(cameraTransform);
		float textSize = Math.Clamp(vector.Distance(cameraTransform[3], GetOrigin()) * 0.02, 0.3, 20);

		vector textTransform[4];
		GetWorldTransform(textTransform);
		textTransform[3] = textTransform[3] + vector.Up * (2 + textSize);

		int flags = DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA | DebugTextFlags.ONCE;
		DebugTextWorldSpace.CreateInWorld(GetWorld(), text, flags, textTransform, textSize, m_Color.PackToInt(), ARGBF(0.5, 1, 1, 1));
	}
#endif
}
