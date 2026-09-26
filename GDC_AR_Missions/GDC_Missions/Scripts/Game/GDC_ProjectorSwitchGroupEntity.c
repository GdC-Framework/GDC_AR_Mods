[EntityEditorProps(category: "GameScripted/Projectors", description: "Switch a projector group")]
class GDC_ProjectorSwitchGroupEntityClass : GenericEntityClass
{
}

class GDC_ProjectorSwitchGroupEntity : GenericEntity
{
    [Attribute("", UIWidgets.EditBox, "Layer / group name (for logging)")]
    protected string m_layerName;

    [Attribute("", UIWidgets.Object, "Layer entity containing projectors")]
    protected GenericEntity m_layerEntity;

    protected ref array<GDC_ProjectorController> m_controllers = new array<GDC_ProjectorController>();
    protected bool m_bDiscovered;

    protected void DiscoverProjectors()
    {
        if (m_bDiscovered)
            return;

        m_bDiscovered = true;
        m_controllers.Clear();

        if (!m_layerEntity)
        {
            Print("TILWMF | GDC | Layer entity not assigned", LogLevel.WARNING);
            return;
        }

        Print(string.Format("TILWMF | GDC | Scanning layer entity '%1' for projector controllers", m_layerEntity.GetName()), LogLevel.NORMAL);

        // Scan all children of the layer using GetChildren() and GetSibling()
        IEntity child = m_layerEntity.GetChildren();
        while (child)
        {
            GDC_ProjectorController controller = GDC_ProjectorController.Cast(child.FindComponent(GDC_ProjectorController));
            if (controller)
            {
                m_controllers.Insert(controller);
                Print(string.Format("TILWMF | GDC | Registered projector controller on '%1'", child.GetName()), LogLevel.NORMAL);
            }

            child = child.GetSibling();
        }

        Print(string.Format("TILWMF | GDC | Discovered %1 projector controllers", m_controllers.Count()), LogLevel.NORMAL);
    }

    void SetMissionState(bool active)
    {
        Print(string.Format("TILWMF | GDC | SetMissionState(%1) on layer '%2'", active, m_layerName), LogLevel.NORMAL);

        if (!m_bDiscovered)
            DiscoverProjectors();

        foreach (GDC_ProjectorController controller : m_controllers)
        {
            if (!controller)
                continue;

            controller.SetMissionState(active);
        }
    }
}