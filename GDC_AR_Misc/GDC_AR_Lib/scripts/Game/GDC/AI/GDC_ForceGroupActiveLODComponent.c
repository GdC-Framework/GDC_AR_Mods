[ComponentEditorProps(category: "GDC/AI", description: "Forces full simulation of the group and its agents (LOD=0, AI active) regardless of distance to players")]
class GDC_ForceGroupActiveLODComponentClass : ScriptComponentClass {}

class GDC_ForceGroupActiveLODComponent : ScriptComponent
{
    override void EOnInit(IEntity owner)
    {
        // Server only
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!gameMode || !gameMode.IsMaster())
            return;

        SCR_AIGroup group = SCR_AIGroup.Cast(owner);
        if (!group)
            return;

        // Force the group's LOD (collective simulation: waypoints, formation, coordination)
        group.SetPermanentLOD(0);

        // Wait for all members to finish spawning before forcing them
        group.GetOnInit().Insert(OnGroupFullyInitialized);

        // Covers agents added after the initial spawn
        group.GetOnAgentAdded().Insert(OnAgentAdded);

        super.EOnInit(owner);
    }

    //------------------------------------------------------------------------------------------------
    // Called once all group members have spawned
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
    // Called whenever an agent is added to the group
    protected void OnAgentAdded(AIAgent agent)
    {
        SetAgentPermanentLOD(agent);
    }

    //------------------------------------------------------------------------------------------------
    protected void SetAgentPermanentLOD(AIAgent agent)
    {
        if (!agent)
            return;

        // Blocks the Dynamic Simulation system
        agent.SetPermanentLOD(0);
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
}
