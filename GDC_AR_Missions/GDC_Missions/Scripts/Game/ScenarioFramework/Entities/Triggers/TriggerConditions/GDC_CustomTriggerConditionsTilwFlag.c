[BaseContainerProps()]
class GDC_CustomTriggerConditionsTilwFlag : SCR_CustomTriggerConditions
{
    [Attribute(desc: "Name of the TILW flag to evaluate")]
    string m_sFlagName;

    [Attribute(defvalue: "1", UIWidgets.CheckBox, desc: "If true, the trigger activates when the flag is set. If false, when it is cleared.")]
    bool m_bFlagMustBeTrue = true;

    //------------------------------------------------------------------------------------------------
    //! Called by the native SFM trigger to evaluate this custom condition.
    override bool Init(IEntity entity)
    {
        if (m_sFlagName.IsEmpty())
            return false;

        TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
        if (!fw)
            return false;

        bool flagValue = fw.IsMissionFlag(m_sFlagName);

        if (m_bFlagMustBeTrue)
            return flagValue;

        return !flagValue;
    }
}