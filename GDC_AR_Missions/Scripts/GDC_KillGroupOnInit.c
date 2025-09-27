//! Script à attacher directement sur un groupe pour tuer toutes ses unités à l'initialisation
//! Utilisation: Ajouter ce script dans la section "Script" du prefab du groupe

[ComponentEditorProps(category: "GDC/Group", description: "Tue toutes les unités du groupe à l'initialisation")]
class GDC_KillGroupOnInitComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.SpinBox, "Délai avant exécution (secondes)", "0 10 0.1")]
	protected float m_fDelay;
	
	[Attribute("1", UIWidgets.CheckBox, "Activer le mode debug (afficher les messages)")]
	protected bool m_bDebugMode;
	
	[Attribute("1", UIWidgets.CheckBox, "Utiliser les ragdolls (physique des corps)")]
	protected bool m_bUseRagdoll;
	
	//! Référence vers le groupe propriétaire
	protected SCR_AIGroup m_Group;
	
	//! Appelé à l'initialisation du composant
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Récupérer la référence du groupe
		m_Group = SCR_AIGroup.Cast(owner);
		if (!m_Group)
		{
			Print("[GDC_KillGroupOnInit] ERREUR: Ce script doit être attaché à un SCR_AIGroup!", LogLevel.ERROR);
			return;
		}
		
		if (m_bDebugMode)
		{
			string groupName = m_Group.GetCustomName();
			if (groupName.IsEmpty())
				groupName = "[Groupe sans nom]";
			
			Print(string.Format("[GDC_KillGroupOnInit] Script attaché au groupe '%1'", groupName), LogLevel.NORMAL);
		}
		
		// Programmer l'exécution
		if (m_fDelay > 0)
		{
			GetGame().GetCallqueue().CallLater(KillAllUnitsInGroup, m_fDelay * 1000, false);
		}
		else
		{
			// Exécuter à la prochaine frame pour s'assurer que tout est initialisé
			GetGame().GetCallqueue().CallLater(KillAllUnitsInGroup, 1, false);
		}
	}
	
	//! Fonction principale pour tuer toutes les unités du groupe
	protected void KillAllUnitsInGroup()
	{
		if (!m_Group)
		{
			if (m_bDebugMode)
				Print("[GDC_KillGroupOnInit] Groupe non valide!", LogLevel.ERROR);
			return;
		}
		
		string groupName = m_Group.GetCustomName();
		if (groupName.IsEmpty())
			groupName = "[Groupe sans nom]";
		
		// Récupérer tous les agents du groupe
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		if (agents.IsEmpty())
		{
			if (m_bDebugMode)
				Print(string.Format("[GDC_KillGroupOnInit] Groupe '%1' ne contient aucune unité", groupName), LogLevel.WARNING);
			return;
		}
		
		if (m_bDebugMode)
		{
			Print(string.Format("[GDC_KillGroupOnInit] Élimination de %1 unité(s) du groupe '%2'", 
				agents.Count(), groupName), LogLevel.NORMAL);
		}
		
		// Tuer chaque unité
		foreach (AIAgent agent : agents)
		{
			KillUnit(agent);
		}
		
		if (m_bDebugMode)
		{
			Print(string.Format("[GDC_KillGroupOnInit] Élimination terminée pour le groupe '%1'", groupName), LogLevel.NORMAL);
		}
	}
	
	//! Fonction pour tuer une unité individuelle
	protected void KillUnit(AIAgent agent)
	{
		if (!agent)
			return;
		
		IEntity entity = agent.GetControlledEntity();
		if (!entity)
			return;
		
		string unitName = entity.GetName();
		if (unitName.IsEmpty())
			unitName = "[Unité sans nom]";
		
		// Méthode 1: Utiliser le Damage Manager (méthode recommandée)
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (damageManager)
		{
			// Réduire la santé à 0
			damageManager.SetHealthScaled(0.0);
			
			// Optionnel: Activer les ragdolls pour un effet plus réaliste
			if (m_bUseRagdoll)
			{
				SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
				if (characterController)
				{
					characterController.SetLifeState(ECharacterLifeState.DEAD);
				}
			}
			
			if (m_bDebugMode)
				Print(string.Format("[GDC_KillGroupOnInit] ✓ Unité '%1' éliminée", unitName), LogLevel.VERBOSE);
			
			return;
		}
		
		// Méthode 2: Fallback avec Character Controller
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
		{
			characterController.SetLifeState(ECharacterLifeState.DEAD);
			
			if (m_bDebugMode)
				Print(string.Format("[GDC_KillGroupOnInit] ✓ Unité '%1' éliminée (fallback)", unitName), LogLevel.VERBOSE);
			
			return;
		}
		
		// Si aucune méthode n'a fonctionné
		if (m_bDebugMode)
		{
			Print(string.Format("[GDC_KillGroupOnInit] ✗ Impossible de tuer l'unité '%1' - composants manquants", unitName), LogLevel.WARNING);
		}
	}
	
	//! Fonction publique pour tuer manuellement (peut être appelée depuis l'extérieur)
	void ManualKill()
	{
		KillAllUnitsInGroup();
	}
	
	//! Fonction pour vérifier l'état des unités (debug)
	void CheckUnitsStatus()
	{
		if (!m_Group)
			return;
		
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		string groupName = m_Group.GetCustomName();
		if (groupName.IsEmpty())
			groupName = "[Groupe sans nom]";
		
		Print(string.Format("=== État des unités du groupe '%1' ===", groupName), LogLevel.NORMAL);
		
		foreach (AIAgent agent : agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			string unitName = entity.GetName();
			string status = "Inconnue";
			
			SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
			if (characterController)
			{
				ECharacterLifeState lifeState = characterController.GetLifeState();
				switch (lifeState)
				{
					case ECharacterLifeState.ALIVE:
						status = "Vivante";
						break;
					case ECharacterLifeState.INCAPACITATED:
						status = "Incapacitée";
						break;
					case ECharacterLifeState.DEAD:
						status = "Morte";
						break;
				}
			}
			
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
			float health = 0.0;
			if (damageManager)
			{
				health = damageManager.GetHealthScaled();
			}
			
			Print(string.Format("• %1 | État: %2 | Santé: %3%%", unitName, status, Math.Round(health * 100)), LogLevel.NORMAL);
		}
		
		Print("=== Fin de l'état ===", LogLevel.NORMAL);
	}
}

/*
INSTRUCTIONS D'UTILISATION:

1. Dans World Editor:
   - Sélectionnez votre groupe d'IA (SCR_AIGroup)
   - Dans la section "Script", cliquez sur le + pour ajouter un composant
   - Choisissez "GDC_KillGroupOnInitComponent"
   - Configurez les paramètres selon vos besoins

2. Paramètres disponibles:
   - "Délai avant exécution": Temps d'attente en secondes avant de tuer les unités
   - "Activer le mode debug": Affiche des messages dans la console pour le debug
   - "Utiliser les ragdolls": Active la physique des corps morts (plus réaliste)

3. Le script s'exécutera automatiquement à l'initialisation du groupe

4. Fonctions utilitaires (accessibles via script):
   - ManualKill(): Force l'exécution manuelle
   - CheckUnitsStatus(): Affiche l'état de toutes les unités du groupe

EXEMPLE D'USAGE AVANCÉ:
// Pour appeler manuellement depuis un autre script
SCR_AIGroup myGroup = // référence vers votre groupe
GDC_KillGroupOnInitComponent killScript = GDC_KillGroupOnInitComponent.Cast(myGroup.FindComponent(GDC_KillGroupOnInitComponent));
if (killScript)
{
    killScript.ManualKill(); // Tuer maintenant
    // ou
    killScript.CheckUnitsStatus(); // Vérifier l'état
}
*/