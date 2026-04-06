[ComponentEditorProps(category: "GDC/AI", description: "Force la simulation complète du groupe et de ses agents (LOD=0, AI activée) indépendamment de la distance aux joueurs")]
class GDC_ForceGroupActiveLODComponentClass : ScriptComponentClass {}

class GDC_ForceGroupActiveLODComponent : ScriptComponent
{
    override void EOnInit(IEntity owner)
    {
        // Serveur uniquement
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!gameMode || !gameMode.IsMaster())
            return;

        SCR_AIGroup group = SCR_AIGroup.Cast(owner);
        if (!group)
            return;

        // Force le LOD du groupe (simulation collective : waypoints, formation, coordination)
        group.SetPermanentLOD(0);

        // Attend la fin du spawn de tous les membres avant de les forcer
        group.GetOnInit().Insert(OnGroupFullyInitialized);

        // Couvre les agents ajoutés après le spawn initial
        group.GetOnAgentAdded().Insert(OnAgentAdded);

        super.EOnInit(owner);
    }

    //------------------------------------------------------------------------------------------------
    // Appelé une fois que tous les membres du groupe sont spawnés
    protected void OnGroupFullyInitialized(SCR_AIGroup group)
    {
        array<AIAgent> agents = {};
        group.GetAgents(agents);
        foreach (AIAgent agent : agents)
        {
            SetAgentPermanentLOD(agent);
        }
    }

    //------------------------------------------------------------------------------------------------
    // Appelé à chaque ajout d'un agent au groupe
    protected void OnAgentAdded(AIAgent agent)
    {
        SetAgentPermanentLOD(agent);
    }

    //------------------------------------------------------------------------------------------------
    protected void SetAgentPermanentLOD(AIAgent agent)
    {
        if (!agent)
            return;

        // Bloque le système Dynamic Simulation
        agent.SetPermanentLOD(0);
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
}
