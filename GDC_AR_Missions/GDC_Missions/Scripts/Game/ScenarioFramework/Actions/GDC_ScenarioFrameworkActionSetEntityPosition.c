[BaseContainerProps(), SCR_ContainerActionTitle()]
modded class SCR_ScenarioFrameworkActionSetEntityPosition
{
	[Attribute(desc: "Entity to be teleported (Optional if action is attached on Slot that spawns target entity)")]
	ref SCR_ScenarioFrameworkGet m_EntityGetter;

	[Attribute(defvalue: "0 0 0", desc: "Position that the entity will be teleported to")]
	vector 	m_vDestination;

	[Attribute(desc: "Name of the entity that above selected entity will be teleported to (Optional)")]
	ref SCR_ScenarioFrameworkGet m_DestinationEntityGetter;

	[Attribute(defvalue: "0 0 0", desc: "Position that will be used in relation to the entity for the position to teleport to (Optional)")]
	vector 	m_vDestinationEntityRelativePosition;

	//------------------------------------------------------------------------------------------------
	override void OnActivate(IEntity object)
	{
		if (!CanActivate())
		{
			Print(string.Format("SF DEBUG | SetEntityPosition | skipped, CanActivate=false action=%1", this), LogLevel.WARNING);
			super.OnActivate(object);
			return;
		}

		IEntity sourceEntity;
		if (!ValidateInputEntity(object, m_EntityGetter, sourceEntity))
		{
			Print(string.Format("SF DEBUG | SetEntityPosition | input entity invalid action=%1", this), LogLevel.WARNING);
			super.OnActivate(object);
			return;
		}

		vector sourceBeforeWorld[4];
		sourceEntity.GetWorldTransform(sourceBeforeWorld);
		vector sourceBeforeLocal[4];
		sourceEntity.GetLocalTransform(sourceBeforeLocal);
		IEntity sourceParentBefore = sourceEntity.GetParent();
		IEntity sourceRootBefore = sourceEntity.GetRootParent();
		Print(string.Format("SF DEBUG | SetEntityPosition | before entity=%1 parent=%2 root=%3 origin=%4 localPos=%5 worldPos=%6", sourceEntity, sourceParentBefore, sourceRootBefore, sourceEntity.GetOrigin(), sourceBeforeLocal[3], sourceBeforeWorld[3]), LogLevel.WARNING);

		if (!m_DestinationEntityGetter)
		{
			Print(string.Format("SF DEBUG | SetEntityPosition | mode=absolute destination=%1", m_vDestination), LogLevel.WARNING);
		}
		else
		{
			SCR_ScenarioFrameworkParam<IEntity> destinationEntityWrapper = SCR_ScenarioFrameworkParam<IEntity>.Cast(m_DestinationEntityGetter.Get());
			if (!destinationEntityWrapper)
			{
				Print(string.Format("SF DEBUG | SetEntityPosition | destination wrapper invalid action=%1", this), LogLevel.WARNING);
			}
			else
			{
				IEntity destinationEntity = destinationEntityWrapper.GetValue();
				if (!destinationEntity)
				{
					Print(string.Format("SF DEBUG | SetEntityPosition | destination entity missing action=%1", this), LogLevel.WARNING);
				}
				else
				{
					vector destinationWorld[4];
					destinationEntity.GetWorldTransform(destinationWorld);
					vector destinationLocal[4];
					destinationEntity.GetLocalTransform(destinationLocal);
					IEntity destinationParent = destinationEntity.GetParent();
					IEntity destinationRoot = destinationEntity.GetRootParent();
					Print(string.Format("SF DEBUG | SetEntityPosition | mode=relative destinationEntity=%1 parent=%2 root=%3 destinationOrigin=%4 destinationLocalPos=%5 destinationWorldPos=%6 relativeOffset=%7", destinationEntity, destinationParent, destinationRoot, destinationEntity.GetOrigin(), destinationLocal[3], destinationWorld[3], m_vDestinationEntityRelativePosition), LogLevel.WARNING);
				}
			}
		}

		super.OnActivate(object);

		vector sourceAfterWorld[4];
		sourceEntity.GetWorldTransform(sourceAfterWorld);
		vector sourceAfterLocal[4];
		sourceEntity.GetLocalTransform(sourceAfterLocal);
		IEntity sourceParentAfter = sourceEntity.GetParent();
		IEntity sourceRootAfter = sourceEntity.GetRootParent();
		Print(string.Format("SF DEBUG | SetEntityPosition | after entity=%1 parent=%2 root=%3 origin=%4 localPos=%5 worldPos=%6", sourceEntity, sourceParentAfter, sourceRootAfter, sourceEntity.GetOrigin(), sourceAfterLocal[3], sourceAfterWorld[3]), LogLevel.WARNING);
	}
}
