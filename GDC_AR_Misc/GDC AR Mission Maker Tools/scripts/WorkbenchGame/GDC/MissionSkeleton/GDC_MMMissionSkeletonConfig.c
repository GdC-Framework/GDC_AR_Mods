#ifdef WORKBENCH
//! Content of the mission skeleton created by GDC_MMMissionSkeletonPlugin (Create Mission Skeleton).
//! GDC's own is Configs/GDC MM/MissionSkeleton.conf; a mission maker wanting another one creates a config inheriting it
//! in their GDC Mission Maker Local addon and selects it in the plugin settings.
[BaseContainerProps(configRoot: true)]
class GDC_MMMissionSkeletonConfig
{
	[Attribute(desc: "Root layers of the mission sub-scene, created when missing (an existing layer is matched whatever its case)")]
	ref array<string> m_aLayers;

	[Attribute(desc: "Folders of layers. A folder holding no layer gets its first layer, since git does not keep empty folders")]
	ref array<ref GDC_MMSkeletonFolder> m_aFolders;

	[Attribute(desc: "Entities placed when the world has none of their class, in this order")]
	ref array<ref GDC_MMSkeletonEntity> m_aEntities;

	[Attribute(desc: "Briefing sections, placed when the mission sub-scene has no entity of their prefab's class")]
	ref GDC_MMSkeletonBriefing m_Briefing;

	[Attribute(desc: "Navmesh files set on a newly placed AI world whose navmesh has none, per terrain: the base game's AI world prefabs only set the LowRes one")]
	ref array<ref GDC_MMSkeletonTerrainNavmeshes> m_aTerrainNavmeshes;

	[Attribute("1", desc: "Write an empty Coalition VON frequency config next to the world (Configs/<world>_Frequencies.conf) and link it on the game mode")]
	bool m_bFrequencyConfig;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class GDC_MMSkeletonFolder
{
	[Attribute(desc: "Folder name, at the root of the sub-scene")]
	string m_sName;

	[Attribute(desc: "Layer created in the folder when it holds none")]
	string m_sFirstLayer;
}

//------------------------------------------------------------------------------------------------
[EnumLinear()]
enum GDC_EMMSkeletonEntityRole
{
	NONE,
	AI_WORLD,	//!< replaced by the terrain's own AI world prefab when one exists; navmeshes filled and checked
	GAME_MODE,	//!< receives the frequency config; an existing game mode of another class is reported
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_Prefab", true)]
class GDC_MMSkeletonEntity
{
	[Attribute(desc: "Entity prefab", params: "et")]
	ResourceName m_Prefab;

	[Attribute(desc: "Addon providing the prefab, named in the error when it is not loaded")]
	string m_sAddon;

	[Attribute(desc: "Entity name, empty for none")]
	string m_sName;

	[Attribute(desc: "Root layer receiving the entity (one of the layers above)")]
	string m_sLayer;

	[Attribute(desc: "Class looked for in the world: the entity is placed only when no entity inherits it. Empty: the prefab's class")]
	string m_sUniqueClass;

	[Attribute("0", desc: "Only look in the mission sub-scene (game mode, briefing...), not in the terrain")]
	bool m_bMissionSubSceneOnly;

	[Attribute("0", uiwidget: UIWidgets.ComboBox, desc: "Special handling", enumType: GDC_EMMSkeletonEntityRole)]
	GDC_EMMSkeletonEntityRole m_eRole;

	[Attribute(desc: "Values set on the new entity (not on an existing one)")]
	ref array<ref GDC_MMSkeletonValue> m_aValues;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sKey")]
class GDC_MMSkeletonValue
{
	[Attribute(desc: "Property name, as in the layer file (e.g. m_bTeamSwitch, \"Active AI limit\" without quotes)")]
	string m_sKey;

	[Attribute(desc: "Value, as in the layer file")]
	string m_sValue;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class GDC_MMSkeletonBriefing
{
	[Attribute(desc: "Mission description prefab, one entity per section", params: "et")]
	ResourceName m_Prefab;

	[Attribute(desc: "Addon providing the prefab, named in the error when it is not loaded")]
	string m_sAddon;

	[Attribute(desc: "Root layer receiving the sections")]
	string m_sLayer;

	[Attribute(desc: "Property receiving the section title")]
	string m_sTitleKey;

	[Attribute(desc: "Property receiving the section text")]
	string m_sTextKey;

	[Attribute(desc: "Property receiving the section index (0, 1, 2...), empty for none")]
	string m_sOrderKey;

	[Attribute(desc: "Values set on every section")]
	ref array<ref GDC_MMSkeletonValue> m_aValues;

	[Attribute(desc: "Sections, in display order")]
	ref array<ref GDC_MMSkeletonBriefingSection> m_aSections;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTitle")]
class GDC_MMSkeletonBriefingSection
{
	[Attribute(desc: "Entity name (a letter prefix keeps the sections sorted in the hierarchy)")]
	string m_sName;

	[Attribute(desc: "Title shown to players")]
	string m_sTitle;

	[Attribute(desc: "Starting text: a hint for the mission maker", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sText;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTerrain")]
class GDC_MMSkeletonTerrainNavmeshes
{
	[Attribute(desc: "Terrain world file name without extension (Arland for worlds/Arland/Arland.ent), any case")]
	string m_sTerrain;

	[Attribute(desc: "Navmesh of the Soldiers navmesh project", params: "nmn")]
	ResourceName m_SoldiersNavmesh;

	[Attribute(desc: "Navmesh of the BTRlike (vehicles) navmesh project", params: "nmn")]
	ResourceName m_VehiclesNavmesh;
}
#endif
