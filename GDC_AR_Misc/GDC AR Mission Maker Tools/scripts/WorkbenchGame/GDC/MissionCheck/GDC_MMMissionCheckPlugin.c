#ifdef WORKBENCH
//! Mission maker plugin checking the open mission world, without changing it. The blocks of checks to run are chosen in
//! the plugin settings (Plugins > Settings > Check Mission). Results: a summary per block in a dialog, and a report in
//! the Log Console, every line starting with [GDC_CHECK] (see GDC_MMCheckLog and GDC_MMCheckFinding.Write).
//! Command line (automatic tests, see Tools/MissionCheckTests): -wbModule=WorldEditor -plugin=GDC_MMMissionCheckPlugin
//! -run -checkWorld="<world .ent>" [-checkLanguage=fr]. Every block runs; each finding is also written as a
//! "[GDC_CHECK_DATA] rule|severity|where" line; exit code 0 when the world was checked, 2 when it could not be opened.
[WorkbenchPluginAttribute(
	name: "Check Mission",
	description: "Check the open mission world (blocks to run in Plugins > Settings)",
	shortcut: "ctrl+alt+c",
	wbModules: { "WorldEditor" },
	category: "GDC",
	awesomeFontCode: 0xF46C)]
class GDC_MMMissionCheckPlugin : WorkbenchPlugin
{
	// The Workbench builds each label from the variable name ("m_bResourcesAndAddons" -> "Resources And Addons"):
	// the names are the labels. Rules of each block: GDC_MMCheckCatalog (listed above the boxes).
	[Attribute("1", desc: "Block 1: layers, game mode, managers, AI world navmeshes, briefing, compared with the mission skeleton below.", category: "Blocks")]
	protected bool m_bMissionStructure;

	[Attribute("1", desc: "Block 2: resources the mission points to, MM library proxies, entities that lost their prefab.", category: "Blocks")]
	protected bool m_bResourcesAndAddons;

	[Attribute("1", desc: "Block 3: playable groups, their factions, slot frequency lists, teleport ranks.", category: "Blocks")]
	protected bool m_bPlayersAndSlots;

	[Attribute("1", desc: "Block 4: settings of the game mode, managers and system entities.", category: "Blocks")]
	protected bool m_bSystemSettings;

	[Attribute("1", desc: "Block 5: triggers, events, flags, interactions, spawners.", category: "Blocks")]
	protected bool m_bMissionLogic;

	[Attribute("1", desc: "Block 6: AI groups, waypoints, AI count.", category: "Blocks")]
	protected bool m_bAI;

	[Attribute("1", desc: "Block 7: what must be clean before publishing the mission addon.", category: "Blocks")]
	protected bool m_bPrePublication;

	[Attribute("{5C3D8A2E9F71B046}Configs/GDC MM/MissionSkeleton.conf", desc: "Mission skeleton block 1 compares the mission with: the one selected in Create Mission Skeleton.", params: "conf class=GDC_MMMissionSkeletonConfig", category: "Blocks")]
	protected ResourceName m_SkeletonConfig;

	[Attribute("1", desc: "List the Info findings in the Log Console. They are always counted in the result.", category: "Report")]
	protected bool m_bShowInfo;

	[Attribute("0", UIWidgets.ComboBox, "Language of the dialogs and the report. The Workbench itself stays in English.", enums: { ParamEnum("English", "0"), ParamEnum("Français", "1") }, category: "Report")]
	protected GDC_EMMCheckLanguage m_eLanguage;

	protected static const string CAPTION = "Check Mission";
	static const string DATA_PREFIX = "[GDC_CHECK_DATA]";
	protected bool m_bCommandLine; //!< run from the command line: every block, Info shown, data lines written

	//------------------------------------------------------------------------------------------------
	override void Configure()
	{
		ApplyLanguage();
		string text = GDC_MMCheckText.T(
			"Check Mission reads the open world and changes nothing.\nTick the blocks to run. TILW and GDC rules only run when the mission uses them.",
			"Check Mission lit le monde ouvert et ne modifie rien.\nCochez les blocs à lancer. Les règles TILW et GDC ne tournent que si la mission les utilise.");
		text += "\n\n" + GDC_MMCheckCatalog.DescribeBlocks();
		Workbench.ScriptDialog(CAPTION + GDC_MMCheckText.T(" - settings", " - réglages"), text, this);
		ApplyLanguage(); // the language may have just been changed in this dialog
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyLanguage()
	{
		GDC_MMCheckText.s_eLanguage = m_eLanguage;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("OK", true)]
	protected int ButtonOK()
	{
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		ApplyLanguage();
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = worldEditor.GetApi();

		string worldPath;
		api.GetWorldPath(worldPath);
		if (worldPath.IsEmpty())
		{
			GDC_MMCheckLog.Write(GDC_MMCheckText.T("No world open: open the mission world first.", "Aucun monde ouvert : ouvrez d'abord le monde de la mission."), LogLevel.WARNING);
			Workbench.Dialog(CAPTION, GDC_MMCheckText.T("Error: open the mission world first.", "Erreur : ouvrez d'abord le monde de la mission."));
			return;
		}

		string summary = RunChecks(api);
		int choice = Workbench.ScriptDialog(CAPTION, summary, new GDC_MMMissionCheckResultDialog());
		while (choice != GDC_MMMissionCheckResultDialog.CLOSE)
		{
			if (choice == GDC_MMMissionCheckResultDialog.BLOCKS)
				Configure();

			summary = RunChecks(api);

			choice = Workbench.ScriptDialog(CAPTION, summary, new GDC_MMMissionCheckResultDialog());
		}
	}

	//------------------------------------------------------------------------------------------------
	override void RunCommandline()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		string world;
		string language;
		if (worldEditor)
		{
			worldEditor.GetCmdLine("-checkWorld", world);
			worldEditor.GetCmdLine("-checkLanguage", language);
		}

		m_eLanguage = GDC_EMMCheckLanguage.ENGLISH;
		if (language == "fr")
			m_eLanguage = GDC_EMMCheckLanguage.FRENCH;

		ApplyLanguage();
		if (!worldEditor || world.IsEmpty() || !worldEditor.SetOpenedResource(world))
		{
			Print(DATA_PREFIX + " OPEN_FAILED|" + world, LogLevel.ERROR);
			Workbench.Exit(2);
			return;
		}

		m_bCommandLine = true;
		m_bMissionStructure = true;
		m_bResourcesAndAddons = true;
		m_bPlayersAndSlots = true;
		m_bSystemSettings = true;
		m_bMissionLogic = true;
		m_bAI = true;
		m_bPrePublication = true;
		m_bShowInfo = true;
		RunChecks(worldEditor.GetApi());
		Print(DATA_PREFIX + " DONE", LogLevel.NORMAL);
		Workbench.Exit(0);
	}

	//------------------------------------------------------------------------------------------------
	//! Runs the checked blocks and writes their findings to the log.
	//! \return text of the summary dialog
	protected string RunChecks(WorldEditorAPI api)
	{
		int start = System.GetTickCount();
		GDC_MMCheckContext context = new GDC_MMCheckContext(api);
		context.m_SkeletonConfig = m_SkeletonConfig;
		string worldName = FilePath.StripPath(context.m_sWorldPath);
		string origins = DescribeOrigins(context);

		GDC_MMCheckLog.Write();
		GDC_MMCheckLog.Separator();
		string worldLabel = GDC_MMCheckText.T("World", "Monde");
		string entitiesLabel = GDC_MMCheckText.T("Entities", "Entités");
		string originsLabel = GDC_MMCheckText.T("Origins", "Origines");
		int width = Math.Max(GDC_MMCheckLog.CharCount(worldLabel), Math.Max(GDC_MMCheckLog.CharCount(entitiesLabel), GDC_MMCheckLog.CharCount(originsLabel)));
		string colon = GDC_MMCheckText.T(": ", " : "); // dialogs and sentences; log columns use " : "
		GDC_MMCheckLog.Write(GDC_MMCheckText.T("CHECK MISSION - ", "VÉRIFICATION DE MISSION - ") + worldName);
		GDC_MMCheckLog.Write(GDC_MMCheckLog.Pad(worldLabel, width) + " : " + context.m_sWorldPath);
		GDC_MMCheckLog.Write(GDC_MMCheckLog.Pad(entitiesLabel, width) + " : " + context.m_aEntities.Count().ToString() + GDC_MMCheckText.T(" (terrain excluded)", " (hors terrain)"));
		GDC_MMCheckLog.Write(GDC_MMCheckLog.Pad(originsLabel, width) + " : " + origins);
		GDC_MMCheckLog.Separator();
		GDC_MMCheckLog.Write();

		string summary = worldLabel + colon + worldName + "\n" + originsLabel + colon + origins + "\n";
		int errors;
		int warnings;
		int infos;
		foreach (GDC_MMCheckBlock block : GetBlocks())
		{
			string title = block.GetNumber().ToString() + ". " + block.GetTitle();
			string result;
			GDC_MMCheckReport report;
			if (!IsBlockChecked(block.GetNumber()))
			{
				result = GDC_MMCheckText.T("skipped (unchecked in the settings)", "ignoré (décoché dans les réglages)");
			}
			else if (!block.IsImplemented())
			{
				result = GDC_MMCheckText.T("not written yet", "pas encore écrit");
			}
			else
			{
				report = new GDC_MMCheckReport(context);
				block.Run(context, report);
				if (m_bCommandLine)
				{
					foreach (GDC_MMCheckFinding finding : report.m_aFindings)
					{
						Print(string.Format("%1 %2|%3|%4", DATA_PREFIX, finding.m_sRule, typename.EnumToString(GDC_EMMCheckSeverity, finding.m_eSeverity), finding.m_sWhere), LogLevel.NORMAL);
					}
				}
				int blockErrors = report.Count(GDC_EMMCheckSeverity.ERROR);
				int blockWarnings = report.Count(GDC_EMMCheckSeverity.WARNING);
				int blockInfos = report.Count(GDC_EMMCheckSeverity.INFO);
				errors += blockErrors;
				warnings += blockWarnings;
				infos += blockInfos;
				result = DescribeCounts(blockErrors, blockWarnings, blockInfos);
			}

			GDC_MMCheckLog.Section(title, result);
			GDC_MMCheckLog.Write();
			if (report)
				WriteFindings(report);

			summary += "\n" + title + colon + result;
		}

		int duration = System.GetTickCount() - start;
		GDC_MMCheckLog.Separator();
		// at the level of the worst finding, so that it stays visible whatever the Log Console level filter
		LogLevel resultLevel = LogLevel.NORMAL;
		if (errors > 0)
			resultLevel = LogLevel.ERROR;
		else if (warnings > 0)
			resultLevel = LogLevel.WARNING;

		GDC_MMCheckLog.Write(GDC_MMCheckText.T("RESULT", "RÉSULTAT") + " - " + worldName + colon + DescribeResult(errors, warnings, infos) + string.Format(" (%1 ms)", duration), resultLevel);
		GDC_MMCheckLog.Separator();

		summary += "\n\n" + GDC_MMCheckText.T("Result", "Résultat") + colon + DescribeResult(errors, warnings, infos) + ".";
		summary += "\n" + GDC_MMCheckText.T("Details: Log Console, filter ", "Détails : Log Console, filtre ") + GDC_MMCheckLog.PREFIX + ".";
		summary += "\n" + GDC_MMCheckText.T("After a fix: Ctrl+Alt+C to check again.", "Après une correction : Ctrl+Alt+C pour vérifier à nouveau.");
		return summary;
	}

	//------------------------------------------------------------------------------------------------
	//! Closing line, short: "1 error to fix, 2 warnings to review, no info"
	protected string DescribeResult(int errors, int warnings, int infos)
	{
		if (errors + warnings + infos == 0)
			return "OK";

		string result = GDC_MMCheckText.Errors(errors);
		if (errors > 0)
			result += GDC_MMCheckText.T(" to fix", " à corriger");

		result += ", " + GDC_MMCheckText.Warnings(warnings);
		if (warnings > 0)
			result += GDC_MMCheckText.T(" to review", " à examiner");

		result += ", " + GDC_MMCheckText.Infos(infos);
		if (infos == 1 && !m_bShowInfo)
			result += GDC_MMCheckText.T(" (hidden)", " (masquée)");
		else if (infos > 1 && !m_bShowInfo)
			result += GDC_MMCheckText.T(" (hidden)", " (masquées)");

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "OK" when there is nothing, else the non-zero counts: "1 error, 2 warnings"
	protected static string DescribeCounts(int errors, int warnings, int infos)
	{
		if (errors + warnings + infos == 0)
			return "OK";

		// only what was found: the section line stays short (the RESULT line gives the three counts)
		array<string> counts = {};
		if (errors > 0)
			counts.Insert(GDC_MMCheckText.Errors(errors));

		if (warnings > 0)
			counts.Insert(GDC_MMCheckText.Warnings(warnings));

		if (infos > 0)
			counts.Insert(GDC_MMCheckText.Infos(infos));

		return string.Join(", ", counts, true);
	}

	//------------------------------------------------------------------------------------------------
	//! \return "vanilla, TILW (GDC not used: its rules are skipped)"
	protected static string DescribeOrigins(GDC_MMCheckContext context)
	{
		string used = GDC_MMCheckOrigin.VANILLA;
		string unused;
		int unusedCount;
		foreach (string origin : GDC_MMCheckOrigin.GetOptional())
		{
			if (context.Uses(origin))
			{
				used += ", " + origin;
			}
			else
			{
				if (!unused.IsEmpty())
					unused += ", ";

				unused += origin;
				unusedCount++;
			}
		}

		if (unused.IsEmpty())
			return used;

		if (unusedCount > 1)
			return used + " (" + unused + GDC_MMCheckText.T(" not used)", " non utilisés)");

		return used + " (" + unused + GDC_MMCheckText.T(" not used)", " non utilisé)");
	}

	//------------------------------------------------------------------------------------------------
	//! \return the blocks in their order, titled from the catalogue; a block not written yet is a plain GDC_MMCheckBlock
	protected static array<ref GDC_MMCheckBlock> GetBlocks()
	{
		array<ref GDC_MMCheckBlock> blocks = {};
		AddBlock(blocks, new GDC_MMCheckBlockStructure(), 1);
		AddBlock(blocks, new GDC_MMCheckBlockResources(), 2);
		AddBlock(blocks, new GDC_MMCheckBlockPlayers(), 3);
		AddBlock(blocks, new GDC_MMCheckBlockSettings(), 4);
		AddBlock(blocks, new GDC_MMCheckBlockLogic(), 5);
		AddBlock(blocks, new GDC_MMCheckBlock(), 6);
		AddBlock(blocks, new GDC_MMCheckBlock(), 7);
		return blocks;
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddBlock(notnull array<ref GDC_MMCheckBlock> blocks, GDC_MMCheckBlock block, int number)
	{
		block.Setup(number, GDC_MMCheckCatalog.GetBlockTitle(number));
		blocks.Insert(block);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsBlockChecked(int number)
	{
		switch (number)
		{
			case 1:
				return m_bMissionStructure;
			case 2:
				return m_bResourcesAndAddons;
			case 3:
				return m_bPlayersAndSlots;
			case 4:
				return m_bSystemSettings;
			case 5:
				return m_bMissionLogic;
			case 6:
				return m_bAI;
			case 7:
				return m_bPrePublication;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Errors first, then warnings, then info (unless hidden in the settings).
	protected void WriteFindings(GDC_MMCheckReport report)
	{
		array<GDC_EMMCheckSeverity> order = { GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckSeverity.INFO };
		foreach (GDC_EMMCheckSeverity severity : order)
		{
			if (severity == GDC_EMMCheckSeverity.INFO && !m_bShowInfo)
				continue;

			foreach (GDC_MMCheckFinding finding : report.m_aFindings)
			{
				if (finding.m_eSeverity == severity)
					finding.Write();
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
//! Buttons of the result dialog.
class GDC_MMMissionCheckResultDialog
{
	static const int CLOSE = 0;
	static const int RERUN = 1;
	static const int BLOCKS = 2;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Check again")]
	protected int ButtonRerun()
	{
		return RERUN;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Blocks...")]
	protected int ButtonBlocks()
	{
		return BLOCKS;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Close", true)]
	protected int ButtonClose()
	{
		return CLOSE;
	}
}
#endif
