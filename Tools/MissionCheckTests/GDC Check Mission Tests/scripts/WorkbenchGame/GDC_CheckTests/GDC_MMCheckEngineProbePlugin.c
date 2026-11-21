#ifdef WORKBENCH
//! Engine behaviours the TILW mission event rules rely on (5.6, 5.7), run from the command line by
//! Tools/MissionCheckTests/run_tests.py (-wbModule=ResourceManager -plugin=GDC_MMCheckEngineProbePlugin -run
//! -probe=<name>). A probe may stop on a script error on purpose: Workbench.Exit is asked first, and each step writes
//! "[GDC_CHECK_PROBE] <marker>", so the markers written tell what the engine did.
//! - terms: TILW operators without operands, their array empty;
//! - loop: a method called on null inside a loop, as TILW_MissionFrameworkEntity.RecheckConditions calls
//!   TILW_MissionEvent.EvalExpression (does the loop go on?);
//! - nullarray: a TILW operator whose operand array was never created.
[WorkbenchPluginAttribute(name: "GDC Check Mission Engine Probe", wbModules: { "ResourceManager" })]
class GDC_MMCheckEngineProbePlugin : WorkbenchPlugin
{
	protected static const string PREFIX = "[GDC_CHECK_PROBE] ";

	//------------------------------------------------------------------------------------------------
	override void RunCommandline()
	{
		string probe;
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (resourceManager)
			resourceManager.GetCmdLine("-probe", probe);

		Workbench.Exit(0);
		Print(PREFIX + "START " + probe, LogLevel.NORMAL);
		if (probe == "terms")
			ProbeTerms();
		else if (probe == "loop")
			ProbeLoop();
		else if (probe == "nullarray")
			ProbeNullArray();

		Print(PREFIX + "END", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void ProbeTerms()
	{
		TILW_ConjunctionTerm all = new TILW_ConjunctionTerm();
		all.m_operands = {};
		TILW_DisjunctionTerm any = new TILW_DisjunctionTerm();
		any.m_operands = {};
		Print(PREFIX + "EMPTY_AND " + all.Eval().ToString(), LogLevel.NORMAL);
		Print(PREFIX + "EMPTY_OR " + any.Eval().ToString(), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void ProbeLoop()
	{
		array<ref TILW_BaseTerm> terms = { null, new TILW_ConjunctionTerm() };
		foreach (int i, TILW_BaseTerm term : terms)
		{
			Step(i, term);
		}

		Print(PREFIX + "AFTER_LOOP", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void Step(int i, TILW_BaseTerm term)
	{
		Print(PREFIX + "STEP " + i, LogLevel.NORMAL);
		// the test of TILW_MissionEvent.EvalExpression: the event runs when it does not return here
		bool ignoreCondition = false;
		if (!term.Eval() && !ignoreCondition)
			Print(PREFIX + "STEP_SKIPPED " + i, LogLevel.NORMAL);
		else
			Print(PREFIX + "STEP_RAN " + i, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void ProbeNullArray()
	{
		TILW_ConjunctionTerm all = new TILW_ConjunctionTerm();
		Print(PREFIX + "NULL_AND " + all.Eval().ToString(), LogLevel.NORMAL);
	}
}
#endif
