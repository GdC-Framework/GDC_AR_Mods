//! Script pour tuer toutes les unités d'un groupe au démarrage de la mission
//! Utilisation : Placer ce script sur un trigger ou l'appeler depuis un Game Mode

//! Classe pour gérer la mort des unités d'un groupe
class KillGroupUnits : GenericEntity
{
	[Attribute("", UIWidgets.EditBox, "Nom du groupe à tuer")]
	protected string m_sGroupName;
	
	[Attribute("1", UIWidgets.CheckBox, "Activer au démarrage de la mission")]
	protected bool m_bActivateOnStart;
	
	[Attribute("0", UIWidgets.SpinBox, "Délai avant exécution (secondes)", "0 10 0.1")]
	protected float m_fDelay;
	
	//! Méthode appelée au démarrage de l'entité
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if (m_bActivateOnStart)
		{
			if (m_fDelay > 0)
				GetGame().GetCallqueue().CallLater(KillGroupByName, m_fDelay * 1000, false, m_sGroupName);
			else
				KillGroupByName(m_sGroupName);
		}
	}
	
	//! Fonction principale pour tuer toutes les unités d'un groupe par son nom
	void KillGroupByName(string groupName)
	{
		if (groupName.IsEmpty())
		{
			Print("KillGroupUnits: Nom du groupe vide!", LogLevel.WARNING);
			return;
		}
		
		// Récupérer le gestionnaire de groupes
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
		{
			Print("KillGroupUnits: Impossible de récupérer le gestionnaire de groupes!", LogLevel.ERROR);
			return;
		}
		
		// Chercher le groupe par son nom
		array<SCR_AIGroup> allGroups = {};
		groupManager.GetAllGroups(allGroups);
		
		SCR_AIGroup targetGroup = null;
		foreach (SCR_AIGroup group : allGroups)
		{
			if (group.GetCustomName() == groupName)
			{
				targetGroup = group;
				break;
			}
		}
		
		if (!targetGroup)
		{
			Print(string.Format("KillGroupUnits: Groupe '%1' introuvable!", groupName), LogLevel.WARNING);
			return;
		}
		
		KillGroup(targetGroup);
	}
	
	//! Fonction pour tuer toutes les unités d'un groupe spécifique
	void KillGroup(SCR_AIGroup group)
	{
		if (!group)
			return;
			
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		
		Print(string.Format("KillGroupUnits: Suppression de %1 unités du groupe '%2'", 
			agents.Count(), group.GetCustomName()), LogLevel.NORMAL);
		
		foreach (AIAgent agent : agents)
		{
			KillUnit(agent);
		}
	}
	
	//! Fonction pour tuer une unité individuelle
	void KillUnit(AIAgent agent)
	{
		if (!agent)
			return;
			
		IEntity entity = agent.GetControlledEntity();
		if (!entity)
			return;
		
		// Méthode 1: Utiliser le composant de dégâts
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (damageManager)
		{
			// Infliger des dégâts fatals
			damageManager.SetHealthScaled(0.0);
			Print(string.Format("KillGroupUnits: Unité %1 éliminée via DamageManager", entity.GetName()), LogLevel.VERBOSE);
			return;
		}
		
		// Méthode 2: Utiliser le composant de caractère (fallback)
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
		{
			// Forcer la mort du personnage
			characterController.SetLifeState(ECharacterLifeState.DEAD);
			Print(string.Format("KillGroupUnits: Unité %1 éliminée via CharacterController", entity.GetName()), LogLevel.VERBOSE);
			return;
		}
		
		Print(string.Format("KillGroupUnits: Impossible de tuer l'unité %1 - composants manquants", entity.GetName()), LogLevel.WARNING);
	}
	
	//! Fonction utilitaire pour tuer toutes les unités d'une faction
	void KillAllUnitsFromFaction(string factionKey)
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;
		
		array<SCR_AIGroup> allGroups = {};
		groupManager.GetAllGroups(allGroups);
		
		foreach (SCR_AIGroup group : allGroups)
		{
			SCR_Faction groupFaction = group.GetFaction();
			if (groupFaction && groupFaction.GetFactionKey() == factionKey)
			{
				KillGroup(group);
			}
		}
	}
	
	//! Fonction utilitaire pour obtenir tous les groupes disponibles (debug)
	void PrintAllGroups()
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;
		
		array<SCR_AIGroup> allGroups = {};
		groupManager.GetAllGroups(allGroups);
		
		Print("=== Liste des groupes disponibles ===", LogLevel.NORMAL);
		foreach (SCR_AIGroup group : allGroups)
		{
			string groupName = group.GetCustomName();
			if (groupName.IsEmpty())
				groupName = "Groupe sans nom";
			
			SCR_Faction faction = group.GetFaction();
			string factionName = faction ? faction.GetFactionKey() : "Pas de faction";
			
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			
			Print(string.Format("Groupe: %1 | Faction: %2 | Unités: %3", 
				groupName, factionName, agents.Count()), LogLevel.NORMAL);
		}
		Print("=== Fin de la liste ===", LogLevel.NORMAL);
	}
}