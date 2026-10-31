// GDC_MarkerAreaComponent.c
// Path in mod: Scripts/Game/GDC/Markers/GDC_MarkerAreaComponent.c

class GDC_MarkerAreaComponentClass : ScriptComponentClass {}

//! Shows a polyline entity on the in-game map, with an outline and, if the polyline is closed, a fill.
//! Purely local: every machine draws it from the points of the world entity.
class GDC_MarkerAreaComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.CheckBox, "Show this area on the in-game map.")]
	protected bool m_bShowOnMap;

	[Attribute("1 0.5 0 1", UIWidgets.ColorPicker, "Color of the outline.")]
	protected ref Color m_OutlineColor;

	[Attribute("3", UIWidgets.Slider, "Width of the outline, in pixels. 0 = no outline.", "0 20 0.5")]
	protected float m_fOutlineWidth;

	[Attribute("1 0.5 0 1", UIWidgets.ColorPicker, "Color of the fill. Its transparency comes from the setting below.")]
	protected ref Color m_FillColor;

	[Attribute("0.25", UIWidgets.Slider, "Opacity of the fill. 0 = fully transparent (no fill), 1 = opaque.", "0 1 0.05")]
	protected float m_fFillOpacity;

	protected const float COLLINEAR_TOLERANCE = 0.001;

	protected static ref array<GDC_MarkerAreaComponent> s_aShownAreas = {};

	//! Points in world coordinates, built on first use.
	protected ref array<vector> m_aPoints;
	protected ref array<int> m_aTriangles;

	//! Points on the screen, as [x0, y0, x1, y1...]. Shared by the outline and the fill.
	protected ref array<float> m_aScreenVertices = {};

	//------------------------------------------------------------------------------------------------
	//! \return The areas of the current world shown on the map
	static array<GDC_MarkerAreaComponent> GetShownAreas()
	{
		return s_aShownAreas;
	}

	//------------------------------------------------------------------------------------------------
	//! Adds what this area draws on the map.
	void CreateDrawCommands(notnull SCR_MapEntity mapEntity, notnull array<ref CanvasWidgetCommand> commands)
	{
		PolylineShapeEntity shape = PolylineShapeEntity.Cast(GetOwner());
		if (!shape)
		{
			Print(string.Format("[GDC_Marker] '%1': the owner entity is not a polyline.", GetOwner().GetName()), LogLevel.WARNING);
			return;
		}

		if (!m_aPoints)
			BuildPolygon(shape);

		if (m_aPoints.Count() < 2)
			return;

		UpdateScreenVertices(mapEntity);

		if (shape.IsClosed() && m_fFillOpacity > 0 && !m_aTriangles.IsEmpty())
		{
			Color fillColor = m_FillColor.Copy();
			fillColor.SetA(m_fFillOpacity);

			TriMeshDrawCommand fill = new TriMeshDrawCommand();
			fill.m_iColor = fillColor.PackToInt();
			fill.m_Vertices = m_aScreenVertices;
			fill.m_Indices = m_aTriangles;
			commands.Insert(fill);
		}

		if (m_fOutlineWidth > 0)
		{
			LineDrawCommand outline = new LineDrawCommand();
			outline.m_iColor = m_OutlineColor.PackToInt();
			outline.m_fWidth = m_fOutlineWidth;
			outline.m_bShouldEnclose = shape.IsClosed();
			outline.m_Vertices = m_aScreenVertices;
			commands.Insert(outline);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Follows the pan and the zoom of the map.
	void UpdateScreenVertices(notnull SCR_MapEntity mapEntity)
	{
		if (!m_aPoints)
			return;

		m_aScreenVertices.Clear();
		foreach (vector point : m_aPoints)
		{
			int screenX, screenY;
			mapEntity.WorldToScreen(point[0], point[2], screenX, screenY, true);
			m_aScreenVertices.Insert(screenX);
			m_aScreenVertices.Insert(screenY);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildPolygon(notnull PolylineShapeEntity shape)
	{
		m_aPoints = {};
		m_aTriangles = {};

		shape.GetPointsPositions(m_aPoints);
		for (int i = 0, count = m_aPoints.Count(); i < count; i++)
		{
			m_aPoints[i] = shape.CoordToParent(m_aPoints[i]);
		}

		if (shape.IsClosed() && !Triangulate(m_aPoints, m_aTriangles))
			Print(string.Format("[GDC_Marker] '%1': the outline crosses itself, the fill will be incomplete.", shape.GetName()), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	//! Splits a polygon, concave or not, into triangles (ear clipping, in the XZ plane).
	//! \param[out] triangles Indices of the points, three per triangle
	//! \return false if the polygon crosses itself and could only be split in part
	static bool Triangulate(notnull array<vector> polygon, notnull array<int> triangles)
	{
		triangles.Clear();

		array<int> remaining = {};
		int count = polygon.Count();
		float doubleArea = 0;
		for (int i = 0; i < count; i++)
		{
			vector point = polygon[i];
			vector nextPoint = polygon[(i + 1) % count];
			doubleArea += point[0] * nextPoint[2] - nextPoint[0] * point[2];
			remaining.Insert(i);
		}

		float orientation = 1;
		if (doubleArea < 0)
			orientation = -1;

		while (remaining.Count() >= 3)
		{
			if (!ClipEar(polygon, remaining, orientation, triangles))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Removes one vertex from the remaining polygon: an ear, or a vertex in the middle of a straight line.
	//! \return false if no vertex can be removed
	protected static bool ClipEar(notnull array<vector> polygon, notnull array<int> remaining, float orientation, notnull array<int> triangles)
	{
		int count = remaining.Count();
		for (int i = 0; i < count; i++)
		{
			int previousIndex = remaining[(i + count - 1) % count];
			int index = remaining[i];
			int nextIndex = remaining[(i + 1) % count];
			vector a = polygon[previousIndex];
			vector b = polygon[index];
			vector c = polygon[nextIndex];

			float cross = ((b[0] - a[0]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[0] - a[0])) * orientation;
			if (Math.AbsFloat(cross) <= COLLINEAR_TOLERANCE)
			{
				remaining.RemoveOrdered(i);
				return true;
			}

			if (cross < 0 || ContainsOtherVertex(polygon, remaining, previousIndex, index, nextIndex))
				continue;

			triangles.Insert(previousIndex);
			triangles.Insert(index);
			triangles.Insert(nextIndex);
			remaining.RemoveOrdered(i);
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ContainsOtherVertex(notnull array<vector> polygon, notnull array<int> remaining, int index1, int index2, int index3)
	{
		foreach (int other : remaining)
		{
			if (other != index1 && other != index2 && other != index3 && Math2D.IsPointInTriangleXZ(polygon[index1], polygon[index2], polygon[index3], polygon[other]))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		if (m_bShowOnMap && GetGame().InPlayMode() && !System.IsConsoleApp())
			s_aShownAreas.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		s_aShownAreas.RemoveItem(this);
	}
}
