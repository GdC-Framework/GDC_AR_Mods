//! Debug variant of TILW spawn instruction with detailed transform logs.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Prefab (Debug)")]
class GDC_SpawnPrefabInstruction_Debug : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to spawn", "et")]
	ResourceName m_prefab;

	[Attribute("", UIWidgets.Auto, desc: "Name of existing entity at which the prefab should be spawned")]
	protected string m_locationName;

	[Attribute("", UIWidgets.Auto, desc: "Optional UNIQUE name assigned to the spawned entity")]
	protected string m_setEntityName;

	protected IEntity m_spawnedEntity;

	override void Execute()
	{
		IEntity locationEntity = GetGame().GetWorld().FindEntityByName(m_locationName);
		if (!locationEntity)
		{
			Print(string.Format("TILW DEBUG | SpawnPrefab(Debug) | location '%1' not found", m_locationName), LogLevel.WARNING);
			return;
		}

		vector locationWorld[4];
		locationEntity.GetWorldTransform(locationWorld);
		vector locationLocal[4];
		locationEntity.GetLocalTransform(locationLocal);
		IEntity locationParent = locationEntity.GetParent();
		IEntity locationRoot = locationEntity.GetRootParent();
		Print(string.Format("TILW DEBUG | SpawnPrefab(Debug) | location='%1' entity=%2 parent=%3 root=%4 origin=%5 localPos=%6 worldPos=%7", m_locationName, locationEntity, locationParent, locationRoot, locationEntity.GetOrigin(), locationLocal[3], locationWorld[3]), LogLevel.WARNING);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform = locationWorld;
		m_spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_prefab), GetGame().GetWorld(), spawnParams);

		if (m_spawnedEntity && m_setEntityName != "")
			m_spawnedEntity.SetName(m_setEntityName);

		if (!m_spawnedEntity)
		{
			Print(string.Format("TILW DEBUG | SpawnPrefab(Debug) | spawn failed prefab='%1'", m_prefab), LogLevel.WARNING);
			return;
		}

		vector spawnedWorld[4];
		m_spawnedEntity.GetWorldTransform(spawnedWorld);
		vector spawnedLocal[4];
		m_spawnedEntity.GetLocalTransform(spawnedLocal);
		IEntity spawnedParent = m_spawnedEntity.GetParent();
		IEntity spawnedRoot = m_spawnedEntity.GetRootParent();
		Print(string.Format("TILW DEBUG | SpawnPrefab(Debug) | spawned=%1 name='%2' parent=%3 root=%4 origin=%5 localPos=%6 worldPos=%7", m_spawnedEntity, m_spawnedEntity.GetName(), spawnedParent, spawnedRoot, m_spawnedEntity.GetOrigin(), spawnedLocal[3], spawnedWorld[3]), LogLevel.WARNING);
	}
}
