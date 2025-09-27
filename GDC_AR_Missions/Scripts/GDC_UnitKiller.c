//! Script simple pour tuer des unités - Version utilitaire
//! À utiliser dans des triggers ou des game modes personnalisés

//! Classe utilitaire avec fonctions statiques
class GDC_UnitKiller
{
	//! Fonction statique pour tuer un groupe par son nom
	//! @param groupName Nom du groupe à éliminer
	//! @param delay Délai avant exécution (optionnel)
	static void KillGroupByName(string groupName, float delay = 0.0)
	{
		if (delay > 0)
		{
			GetGame().GetCallqueue().CallLater(ExecuteKillGroup, delay * 1000, false, groupName);
		}
		else
		{
			ExecuteKillGroup(groupName);
		}
	}
	
	//! Fonction statique pour tuer toutes les unités d'une faction
	//! @param factionKey Clé de la faction (ex: "USSR", "US", "RHS_AFRF")
	//! @param delay Délai avant exécution (optionnel)
	static void KillAllFromFaction(string factionKey, float delay = 0.0)
	{
		if (delay > 0)
		{
			GetGame().GetCallqueue().CallLater(ExecuteKillFaction, delay * 1000, false, factionKey);
		}
		else
		{
			ExecuteKillFaction(factionKey);
		}
	}
	
	//! Fonction statique pour tuer des unités dans un rayon autour d'une position
	//! @param position Position centrale
	//! @param radius Rayon en mètres
	//! @param factionKey Faction ciblée (optionnel, si vide = toutes)
	static void KillUnitsInRadius(vector position, float radius, string factionKey = "")
	{
		array<IEntity> entities = {};
		GetGame().GetWorld().QueryEntitiesBySphere(position, radius, null, null, entities);
		
		foreach (IEntity entity : entities)
		{
			// Vérifier si c'est un personnage
			SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
			if (!characterController)
				continue;
			
			// Vérifier la faction si spécifiée
			if (!factionKey.IsEmpty())
			{
				SCR_FactionAffiliationComponent factionComp = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent));
				if (!factionComp)
					continue;
				
				SCR_Faction entityFaction = factionComp.GetAffiliatedFaction();
				if (!entityFaction || entityFaction.GetFactionKey() != factionKey)
					continue;
			}
			
			// Tuer l'unité
			KillSingleUnit(entity);
		}
	}
	
	//! Exécution du kill group (fonction privée)
	protected static void ExecuteKillGroup(string groupName)
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
		{
			Print("[GDC_UnitKiller] Gestionnaire de groupes introuvable!", LogLevel.ERROR);
			return;
		}
		
		array<SCR_AIGroup> allGroups = {};
		groupManager.GetAllGroups(allGroups);
		
		foreach (SCR_AIGroup group : allGroups)
		{
			if (group.GetCustomName() == groupName)
			{
				array<AIAgent> agents = {};
				group.GetAgents(agents);
				
				Print(string.Format("[GDC_UnitKiller] Élimination de %1 unités du groupe '%2'", 
					agents.Count(), groupName), LogLevel.NORMAL);
				
				foreach (AIAgent agent : agents)
				{
					IEntity entity = agent.GetControlledEntity();
					if (entity)
						KillSingleUnit(entity);
				}
				return;
			}
		}
		
		Print(string.Format("[GDC_UnitKiller] Groupe '%1' introuvable!", groupName), LogLevel.WARNING);
	}
	
	//! Exécution du kill faction (fonction privée)
	protected static void ExecuteKillFaction(string factionKey)
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;
		
		array<SCR_AIGroup> allGroups = {};
		groupManager.GetAllGroups(allGroups);
		
		int killedCount = 0;
		foreach (SCR_AIGroup group : allGroups)
		{
			SCR_Faction groupFaction = group.GetFaction();
			if (groupFaction && groupFaction.GetFactionKey() == factionKey)
			{
				array<AIAgent> agents = {};
				group.GetAgents(agents);
				
				foreach (AIAgent agent : agents)
				{
					IEntity entity = agent.GetControlledEntity();
					if (entity)
					{
						KillSingleUnit(entity);
						killedCount++;
					}
				}
			}
		}
		
		Print(string.Format("[GDC_UnitKiller] %1 unités de la faction '%2' éliminées", 
			killedCount, factionKey), LogLevel.NORMAL);
	}
	
	//! Tuer une unité individuelle (fonction privée)
	protected static void KillSingleUnit(IEntity entity)
	{
		if (!entity)
			return;
		
		// Méthode principale: Damage Manager
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (damageManager)
		{
			// Méthode 1: Réduire la santé à 0
			damageManager.SetHealthScaled(0.0);
			
			// Méthode 2: Infliger des dégâts fatals (alternative plus réaliste)
			// float maxHealth = damageManager.GetMaxHealth();
			// damageManager.HandleDamage(maxHealth * 2, EDamageType.TRUE, entity);
			
			return;
		}
		
		// Méthode alternative: Character Controller
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
		{
			characterController.SetLifeState(ECharacterLifeState.DEAD);
			return;
		}
		
		Print(string.Format("[GDC_UnitKiller] Impossible de tuer l'entité %1", entity.GetName()), LogLevel.WARNING);
	}
}

//! Exemples d'utilisation dans un Game Mode ou Trigger:
/*

// Dans un Game Mode (méthode OnGameStart)
class MissionGameMode : SCR_BaseGameMode
{
	override void OnGameStart()
	{
		super.OnGameStart();
		
		// Tuer le groupe "EnemyPatrol" après 5 secondes
		GDC_UnitKiller.KillGroupByName("EnemyPatrol", 5.0);
		
		// Tuer toutes les unités USSR immédiatement
		GDC_UnitKiller.KillAllFromFaction("USSR");
		
		// Tuer toutes les unités dans un rayon de 100m autour d'une position
		vector position = "1000 0 1000"; // Coordonnées X Y Z
		GDC_UnitKiller.KillUnitsInRadius(position, 100.0, "RHS_AFRF");
	}
}

// Dans un Trigger
class DeadBodyTrigger : SCR_BaseTriggerEntity
{
	override void OnActivate(IEntity ent)
	{
		super.OnActivate(ent);
		
		// Tuer un groupe spécifique quand le trigger est activé
		GDC_UnitKiller.KillGroupByName("DeadBodyGroup");
	}
}

*/