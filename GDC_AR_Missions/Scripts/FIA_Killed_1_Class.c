//! Classe de groupe avec élimination automatique des unités à l'initialisation
class FIA_Killed_1_Class : SCR_AIGroup
{
	//! Appelé à l'initialisation du groupe
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Exécuter à la prochaine frame pour s'assurer que tout est initialisé
		GetGame().GetCallqueue().CallLater(KillAllUnits, 1, false);
	}
	
	//! Fonction pour tuer toutes les unités du groupe
	protected void KillAllUnits()
	{
		Print("[FIA_Killed_1_Class] Début de l'élimination des unités du groupe", LogLevel.NORMAL);
		
		// Récupérer tous les agents du groupe
		array<AIAgent> agents = {};
		GetAgents(agents);
		
		Print(string.Format("[FIA_Killed_1_Class] Nombre d'agents trouvés: %1", agents.Count()), LogLevel.NORMAL);
		
		foreach (AIAgent agent : agents)
		{
			if (!agent)
			{
				Print("[FIA_Killed_1_Class] Agent null trouvé, ignoré", LogLevel.WARNING);
				continue;
			}
			
			Print(string.Format("[FIA_Killed_1_Class] Traitement de l'agent: %1", agent.ToString()), LogLevel.NORMAL);
			
			// Récupérer l'entité contrôlée par l'agent
			IEntity controlledEntity = agent.GetControlledEntity();
			if (!controlledEntity)
			{
				Print("[FIA_Killed_1_Class] Aucune entité contrôlée trouvée pour cet agent", LogLevel.WARNING);
				continue;
			}
			
			Print(string.Format("[FIA_Killed_1_Class] Entité contrôlée trouvée: %1", controlledEntity.GetName()), LogLevel.NORMAL);
			
			// Utiliser DamageManagerComponent pour tuer l'unité
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(controlledEntity.FindComponent(DamageManagerComponent));
			if (damageManager)
			{
				Print(string.Format("[FIA_Killed_1_Class] DamageManagerComponent trouvé, santé avant: %1", damageManager.GetHealthScaled()), LogLevel.NORMAL);
				damageManager.SetHealthScaled(0);
				Print(string.Format("[FIA_Killed_1_Class] Unité tuée - %1, santé après: %2", controlledEntity.GetName(), damageManager.GetHealthScaled()), LogLevel.NORMAL);
			}
			else
			{
				Print(string.Format("[FIA_Killed_1_Class] DamageManagerComponent non trouvé pour: %1", controlledEntity.GetName()), LogLevel.ERROR);
			}
		}
		
		Print("[FIA_Killed_1_Class] Fin de l'élimination des unités du groupe", LogLevel.NORMAL);
	}
};