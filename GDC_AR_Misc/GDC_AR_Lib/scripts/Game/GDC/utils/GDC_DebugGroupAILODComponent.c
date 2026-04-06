[ComponentEditorProps(category: "Debug", description: "Debug: print AI LOD for each AI Agent in the group")]
class GDC_DebugGroupAILODComponentClass : ScriptComponentClass {}

class GDC_DebugGroupAILODComponent : ScriptComponent
{
    override void EOnInit(IEntity owner)
    {
        if (!GetGame().InPlayMode())
            return;

        SCR_AIGroup group = SCR_AIGroup.Cast(owner);
        if (!group)
        {
            Print("[DebugAILOD] Owner is not a SCR_AIGroup", LogLevel.ERROR);
            return;
        }

        GetGame().GetCallqueue().CallLater(PrintAgentsAILOD, 10000, true, group);
    }

    protected void PrintAgentsAILOD(SCR_AIGroup group)
    {
        // --- LOD du groupe lui-même ---
        Print(string.Format(
            "[DebugAILOD] Groupe '%1' — LOD=%2 | PermanentLOD=%3 | MaxLOD=%4 | IsActivated=%5",
            group,
            group.GetLOD(),
            group.GetPermanentLOD(),
            group.GetMaxLOD(),
            group.IsAIActivated()
        ), LogLevel.WARNING);

        // --- LOD de chaque agent membre ---
        array<AIAgent> agents = {};
        group.GetAgents(agents);

        Print(string.Format("[DebugAILOD]   %1 agent(s)", agents.Count()), LogLevel.WARNING);

        foreach (int i, AIAgent agent : agents)
        {
            Print(string.Format(
                "[DebugAILOD]   Agent[%1] LOD=%2 | PermanentLOD=%3 | MaxLOD=%4 | IsActivated=%5",
                i,
                agent.GetLOD(),
                agent.GetPermanentLOD(),
                agent.GetMaxLOD(),
                agent.IsAIActivated()
            ), LogLevel.WARNING);
        }
    }

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }
}