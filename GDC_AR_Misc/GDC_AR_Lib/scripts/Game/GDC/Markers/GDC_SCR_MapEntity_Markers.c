// GDC_SCR_MapEntity_Markers.c
// Path in mod: Scripts/Game/GDC/Markers/GDC_SCR_MapEntity_Markers.c

//! Draws the areas marked with GDC_MarkerAreaComponent on the map.
modded class SCR_MapEntity
{
	protected const ResourceName GDC_AREA_CANVAS_LAYOUT = "{A4D17E3C6B9025F8}UI/layouts/Markers/GDC_MarkerAreaCanvas.layout";

	protected CanvasWidget m_wGDC_AreaCanvas;
	protected ref array<ref CanvasWidgetCommand> m_aGDC_AreaCommands;

	//------------------------------------------------------------------------------------------------
	//! The canvas is created first, so that the markers created when the map opens are on top of it.
	override protected void OnMapOpen(MapConfiguration config)
	{
		GDC_CreateAreaCanvas();
		super.OnMapOpen(config);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnMapClose()
	{
		if (m_wGDC_AreaCanvas)
			m_wGDC_AreaCanvas.RemoveFromHierarchy();

		m_wGDC_AreaCanvas = null;
		m_aGDC_AreaCommands = null;
		super.OnMapClose();
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateMap(float timeSlice)
	{
		super.UpdateMap(timeSlice);

		if (!m_wGDC_AreaCanvas)
			return;

		foreach (GDC_MarkerAreaComponent area : GDC_MarkerAreaComponent.GetShownAreas())
		{
			area.UpdateScreenVertices(this);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void GDC_CreateAreaCanvas()
	{
		array<GDC_MarkerAreaComponent> areas = GDC_MarkerAreaComponent.GetShownAreas();
		if (areas.IsEmpty())
			return;

		Widget mapFrame = m_wMapRoot.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			return;

		m_wGDC_AreaCanvas = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidgets(GDC_AREA_CANVAS_LAYOUT, mapFrame));
		if (!m_wGDC_AreaCanvas)
			return;

		m_aGDC_AreaCommands = {};
		foreach (GDC_MarkerAreaComponent area : areas)
		{
			area.CreateDrawCommands(this, m_aGDC_AreaCommands);
		}

		m_wGDC_AreaCanvas.SetDrawCommands(m_aGDC_AreaCommands);
	}
}
