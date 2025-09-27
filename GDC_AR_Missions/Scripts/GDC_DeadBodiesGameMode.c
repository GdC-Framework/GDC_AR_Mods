//! Exemple de Game Mode avec unités mortes au démarrage
//! Hérite du Game Mode de base et ajoute la fonctionnalité de kill des unités

[EntityEditorProps(category: "GameScripted/GameMode", description: "Game Mode avec unités pré-tuées")]
class GDC_DeadBodiesGameMode : SCR_BaseGameMode
{
	[Attribute("", UIWidgets.EditBox, "Noms des groupes à tuer (séparés par des virgules)")]
	protected string m_sGroupsToKill;
	
	[Attribute("", UIWidgets.EditBox, "Factions à éliminer complètement (séparées par des virgules)")]
	protected string m_sFactionsToKill;
	
	[Attribute("2.0", UIWidgets.SpinBox, "Délai avant élimination (secondes)", "0 60 0.1")]
	protected float m_fKillDelay;
	
	[Attribute("1", UIWidgets.CheckBox, "Afficher les messages de debug")]
	protected bool m_bDebugMode;
	
	//! Appelé au démarrage du jeu
	override void OnGameStart()
	{
		super.OnGameStart();
		
		if (m_bDebugMode)
			Print("[GDC_DeadBodiesGameMode] Game Mode démarré, préparation de l'élimination des unités...", LogLevel.NORMAL);
		
		// Planifier l'élimination des unités après le délai spécifié
		GetGame().GetCallqueue().CallLater(ExecuteUnitKilling, m_fKillDelay * 1000, false);
	}
	
	//! Exécute l'élimination des unités configurées
	protected void ExecuteUnitKilling()
	{
		// Tuer les groupes spécifiés
		if (!m_sGroupsToKill.IsEmpty())
		{
			array<string> groupNames = {};
			m_sGroupsToKill.Split(",", groupNames, true);
			
			foreach (string groupName : groupNames)
			{
				string trimmedName = groupName;
				trimmedName.Trim(); // Supprimer les espaces
				
				if (!trimmedName.IsEmpty())
				{
					if (m_bDebugMode)
						Print(string.Format("[GDC_DeadBodiesGameMode] Élimination du groupe: %1", trimmedName), LogLevel.NORMAL);
					
					GDC_UnitKiller.KillGroupByName(trimmedName);
				}
			}
		}
		
		// Tuer les factions spécifiées
		if (!m_sFactionsToKill.IsEmpty())
		{
			array<string> factionKeys = {};
			m_sFactionsToKill.Split(",", factionKeys, true);
			
			foreach (string factionKey : factionKeys)
			{
				string trimmedKey = factionKey;
				trimmedKey.Trim(); // Supprimer les espaces
				
				if (!trimmedKey.IsEmpty())
				{
					if (m_bDebugMode)
						Print(string.Format("[GDC_DeadBodiesGameMode] Élimination de la faction: %1", trimmedKey), LogLevel.NORMAL);
					
					GDC_UnitKiller.KillAllFromFaction(trimmedKey);
				}
			}
		}
		
		if (m_bDebugMode)
			Print("[GDC_DeadBodiesGameMode] Élimination des unités terminée", LogLevel.NORMAL);
	}
	
	//! Fonction d'assistance pour lister tous les groupes disponibles (pour le debug)
	void PrintAvailableGroups()
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
		{
			Print("[GDC_DeadBodiesGameMode] Gestionnaire de groupes introuvable!", LogLevel.ERROR);
			return;
		}
		
		array<SCR_AIGroup> allGroups = {};
		groupManager.GetAllGroups(allGroups);
		
		Print("=== GROUPES DISPONIBLES ===", LogLevel.NORMAL);
		foreach (SCR_AIGroup group : allGroups)
		{
			string groupName = group.GetCustomName();
			if (groupName.IsEmpty())
				groupName = "[Groupe sans nom]";
			
			SCR_Faction faction = group.GetFaction();
			string factionName = faction ? faction.GetFactionKey() : "[Pas de faction]";
			
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			
			Print(string.Format("• %1 | Faction: %2 | Unités: %3", 
				groupName, factionName, agents.Count()), LogLevel.NORMAL);
		}
		Print("=== FIN DE LA LISTE ===", LogLevel.NORMAL);
	}
}

/*
INSTRUCTIONS D'UTILISATION:

1. Dans World Editor:
   - Créez votre mission normalement avec vos groupes d'IA
   - Remplacez le Game Mode par défaut par "GDC_DeadBodiesGameMode"
   - Configurez les paramètres :
     * "Noms des groupes à tuer" : "Patrol_1, Guard_Post, Checkpoint_Alpha"
     * "Factions à éliminer" : "USSR, RHS_AFRF" 
     * "Délai avant élimination" : 2.0 (secondes)

2. Les unités seront automatiquement tuées au démarrage de la mission

3. Configuration typique pour une mission avec cadavres:
   - Placez vos groupes d'IA normalement dans l'éditeur
   - Donnez-leur des noms explicites (ex: "DeadGuards", "Casualties")
   - Configurez le Game Mode avec ces noms
   - Les unités apparaîtront mortes dès le début

4. Pour du debug:
   - Activez "Afficher les messages de debug"
   - Utilisez la console pour appeler PrintAvailableGroups() si besoin
*/