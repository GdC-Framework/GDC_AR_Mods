#ifdef WORKBENCH
//! Unit tests of Check Mission and of the loadout analysis it shares with the Loadout Editor, run from the command line
//! by Tools/MissionCheckTests/run_tests.py (-wbModule=ResourceManager -plugin=GDC_MMCheckUnitTestsPlugin -run).
//! Each test writes "[GDC_CHECK_TEST] PASS|FAIL <name> <detail>", the last line "[GDC_CHECK_TEST] END <failures>".
[WorkbenchPluginAttribute(name: "GDC Check Mission Unit Tests", wbModules: { "ResourceManager" })]
class GDC_MMCheckUnitTestsPlugin : WorkbenchPlugin
{
	protected static const string PREFIX = "[GDC_CHECK_TEST]";
	protected static const string CHARACTERS = "Prefabs/GDC_CheckTests/Characters/";
	protected static const ResourceName RIFLEMAN = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	protected static const ResourceName UNARMED = "{2F912ED6E399FF47}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Unarmed.et";

	protected int m_iFailures;

	//------------------------------------------------------------------------------------------------
	override void RunCommandline()
	{
		TestCatalogue();
		TestTexts();
		TestResourceStates();
		TestExcludedAddons();
		TestLoadouts();
		Print(string.Format("%1 END %2", PREFIX, m_iFailures), LogLevel.NORMAL);
		Workbench.Exit(0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Check(string name, bool ok, string detail = string.Empty)
	{
		string result = "PASS";
		LogLevel level = LogLevel.NORMAL;
		if (!ok)
		{
			result = "FAIL";
			level = LogLevel.ERROR;
			m_iFailures++;
		}

		Print(string.Format("%1 %2 %3 %4", PREFIX, result, name, detail), level);
	}

	//------------------------------------------------------------------------------------------------
	//! Every rule complete in both languages, same placeholders, unique ids and titles, MODSET rules verified and sourced.
	protected void TestCatalogue()
	{
		array<GDC_EMMCheckLanguage> languages = { GDC_EMMCheckLanguage.ENGLISH, GDC_EMMCheckLanguage.FRENCH };
		set<string> ids = new set<string>();
		foreach (GDC_MMCheckRule rule : GDC_MMCheckCatalog.GetRules())
		{
			string id = rule.m_sId;
			Check("catalogue.unique_id " + id, !ids.Contains(id));
			ids.Insert(id);
			Check("catalogue.block " + id, rule.m_iBlock >= 1 && rule.m_iBlock <= GDC_MMCheckCatalog.GetBlockTitles().Count());

			array<string> problems = {};
			array<string> fixes = {};
			foreach (GDC_EMMCheckLanguage language : languages)
			{
				GDC_MMCheckText.s_eLanguage = language;
				string languageName = typename.EnumToString(GDC_EMMCheckLanguage, language);
				Check("catalogue.title " + id + " " + languageName, !rule.GetTitle().IsEmpty());
				Check("catalogue.problem " + id + " " + languageName, !rule.GetProblem().IsEmpty());
				Check("catalogue.description " + id + " " + languageName, !rule.GetDescription().IsEmpty());
				// a feature may exist in several frameworks (triggers, teleport): the title names the framework
				if (rule.m_sOrigin == GDC_MMCheckOrigin.TILW)
					Check("catalogue.title_names_framework " + id + " " + languageName, rule.GetTitle().Contains("TILW"), rule.GetTitle());
				problems.Insert(rule.GetProblem());
				fixes.Insert(rule.GetFix());
			}

			GDC_MMCheckText.s_eLanguage = GDC_EMMCheckLanguage.ENGLISH;
			Check("catalogue.placeholders " + id, Placeholders(problems[0]) == Placeholders(problems[1]) && Placeholders(fixes[0]) == Placeholders(fixes[1]),
				Placeholders(problems[0]) + " / " + Placeholders(problems[1]));
			Check("catalogue.fix_both_or_none " + id, fixes[0].IsEmpty() == fixes[1].IsEmpty());

			if (rule.m_eFamily == GDC_EMMCheckFamily.MODSET)
			{
				Check("catalogue.modset_verified " + id, !rule.m_sVerifiedAddon.IsEmpty() && !rule.m_sVerifiedVersion.IsEmpty() && !rule.m_sVerifiedOn.IsEmpty());
				Check("catalogue.modset_sheet " + id, !rule.m_sKB.IsEmpty());
			}
		}

		foreach (GDC_EMMCheckLanguage language : languages)
		{
			GDC_MMCheckText.s_eLanguage = language;
			set<string> titles = new set<string>();
			foreach (GDC_MMCheckRule titled : GDC_MMCheckCatalog.GetRules())
			{
				Check("catalogue.unique_title " + titled.m_sId, !titles.Contains(titled.GetTitle()), titled.GetTitle());
				titles.Insert(titled.GetTitle());
			}
		}

		GDC_MMCheckText.s_eLanguage = GDC_EMMCheckLanguage.ENGLISH;
		Check("catalogue.excluded_addons", !GDC_MMCheckCatalog.GetExcludedAddons().IsEmpty());
		Check("catalogue.sheet_url", GDC_MMCheckCatalog.GetSheetUrl("x").StartsWith("https://github.com/") && GDC_MMCheckCatalog.GetSheetUrl("").IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	//! \return "%1%2" for a text holding %2 and %1: the placeholders it uses, in order
	protected static string Placeholders(string text)
	{
		string found;
		for (int n = 1; n <= 9; n++)
		{
			if (text.Contains("%" + n.ToString()))
				found += "%" + n.ToString();
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected void TestTexts()
	{
		Check("text.char_count", GDC_MMCheckLog.CharCount("Où") == 2 && GDC_MMCheckLog.CharCount("Problème") == 8);
		Check("text.pad", GDC_MMCheckLog.Pad("Où", 5) == "Où   ");
		Check("text.upper", GDC_MMCheckLog.Upper("réglages des systèmes") == "RÉGLAGES DES SYSTÈMES", GDC_MMCheckLog.Upper("réglages des systèmes"));

		GDC_MMCheckText.s_eLanguage = GDC_EMMCheckLanguage.FRENCH;
		Check("text.counts_fr", GDC_MMCheckText.Errors(0) == "aucune erreur" && GDC_MMCheckText.Warnings(1) == "1 avertissement" && GDC_MMCheckText.Infos(2) == "2 infos");
		Check("text.pick_fr", GDC_MMCheckText.T("a", "b") == "b" && GDC_MMCheckText.T("a", "") == "a");
		GDC_MMCheckText.s_eLanguage = GDC_EMMCheckLanguage.ENGLISH;
		Check("text.counts_en", GDC_MMCheckText.Errors(0) == "no error" && GDC_MMCheckText.Warnings(1) == "1 warning" && GDC_MMCheckText.Infos(2) == "2 info");
	}

	//------------------------------------------------------------------------------------------------
	//! Rule 2.1: how a reference is resolved, the cases the engine handles in a surprising way.
	protected void TestResourceStates()
	{
		CheckState("resource.ok", "{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et", GDC_MMCheckContext.RESOURCE_OK);
		CheckState("resource.renamed_path", "{E0A05C76552E7F58}Prefabs/AI/Renamed_AIWorld.et", GDC_MMCheckContext.RESOURCE_OK);
		CheckState("resource.path_only", "Prefabs/AI/SCR_AIWorld.et", GDC_MMCheckContext.RESOURCE_OK);
		CheckState("resource.missing", "{6C0A11E0C4EC59F2}Prefabs/GDC_CheckTests/MissingPrefab.et", GDC_MMCheckContext.RESOURCE_MISSING);
		CheckState("resource.missing_path_only", "Prefabs/GDC_CheckTests/MissingPrefab.et", GDC_MMCheckContext.RESOURCE_MISSING);
		// unknown GUID on an existing path: repaired from the path by the Workbench when it opens the world
		CheckState("resource.wrong_guid", "{6C0A11E0C4EC59F4}Prefabs/AI/SCR_AIWorld.et", GDC_MMCheckContext.RESOURCE_OK);
	}

	//------------------------------------------------------------------------------------------------
	//! Rule 2.4 with made-up lists of loaded addons: loading a missing addon opens a modal dialog, so the real one is not
	//! loaded here. Anarchy Map Markers is excluded only from Reforger Lobby missions (PS_GameModeCoop).
	protected void TestExcludedAddons()
	{
		array<string> withAnarchy = { "58D0FB3206B6F859", "69A510CE600D1126" };
		array<string> withoutAnarchy = { "58D0FB3206B6F859" };
		set<string> lobbyMission = new set<string>();
		lobbyMission.Insert("PS_GameModeCoop");
		lobbyMission.Insert("SCR_AIWorld");
		set<string> otherMission = new set<string>();
		otherMission.Insert("SCR_AIWorld");

		array<GDC_MMCheckExcludedAddon> found = GDC_MMCheckBlockResources.FindExcludedAddons(withAnarchy, lobbyMission);
		Check("excluded.anarchy_with_lobby", found.Count() == 1 && found[0].m_sGUID == "69A510CE600D1126", found.Count().ToString());
		Check("excluded.anarchy_without_lobby", GDC_MMCheckBlockResources.FindExcludedAddons(withAnarchy, otherMission).IsEmpty());
		Check("excluded.not_loaded", GDC_MMCheckBlockResources.FindExcludedAddons(withoutAnarchy, lobbyMission).IsEmpty());
		Check("excluded.reason_both_languages", found.Count() == 1 && !found[0].GetReason().IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckState(string name, ResourceName resource, int expected)
	{
		int state = GDC_MMCheckContext.ResolveResourceState(resource);
		Check(name, state == expected, string.Format("state %1, expected %2", state, expected));
	}

	//------------------------------------------------------------------------------------------------
	//! GDC_LoadoutAnalysis, shared by the Loadout Editor and rules 3.6 to 3.10.
	protected void TestLoadouts()
	{
		GDC_LoadoutAnalysis rifleman = GDC_LoadoutAnalysis.ReadPrefab(RIFLEMAN);
		Check("loadout.read", rifleman != null);
		if (rifleman)
		{
			Check("loadout.primary", rifleman.GetPrimaryWeapon().GetPath() == "Prefabs/Weapons/Rifles/M16/Rifle_M16A2.et", rifleman.GetPrimaryWeapon());
			Check("loadout.spare_magazines", rifleman.GetPrimarySpareMagazines() == 6, rifleman.GetPrimarySpareMagazines().ToString());
			Check("loadout.clean", rifleman.m_aOverLimit.IsEmpty() && rifleman.m_aNotOnGear.IsEmpty() && rifleman.m_aMissing.IsEmpty());
			float weight = rifleman.GetTotalWeight();
			Check("loadout.weight", weight > 15 && weight < 40 && rifleman.m_fMaxWeight == 80, string.Format("%1 / %2", weight, rifleman.m_fMaxWeight));
			Check("loadout.weapon_label", WeaponLabel(rifleman).EndsWith("6 spare magazines"), WeaponLabel(rifleman));
		}

		GDC_LoadoutAnalysis unarmed = GDC_LoadoutAnalysis.ReadPrefab(UNARMED);
		Check("loadout.overflow", unarmed && unarmed.m_aOverLimit.Count() == 2, LoadoutDetail(unarmed));
		Check("loadout.no_primary", unarmed && unarmed.GetPrimarySpareMagazines() == -1);

		GDC_LoadoutAnalysis noSpare = ReadTest("T_NoSpare");
		Check("loadout.no_spare", noSpare && noSpare.GetPrimarySpareMagazines() == 0, LoadoutDetail(noSpare));
		Check("loadout.no_spare_label", noSpare && WeaponLabel(noSpare).Contains(GDC_LoadoutAnalysis.NO_SPARE_MAGAZINE), WeaponLabel(noSpare));

		GDC_LoadoutAnalysis missing = ReadTest("T_Missing");
		Check("loadout.missing_prefab", missing && missing.m_aMissing.Count() == 1, LoadoutDetail(missing));

		GDC_LoadoutAnalysis heavy = ReadTest("T_Heavy");
		Check("loadout.heavy", heavy && heavy.GetTotalWeight() > heavy.m_fMaxWeight, LoadoutDetail(heavy));

		GDC_LoadoutAnalysis notOnGear = ReadTest("T_NotOnGear");
		Check("loadout.not_on_gear", notOnGear && notOnGear.m_aNotOnGear.Count() == 1 && notOnGear.m_aOverLimit.IsEmpty(), LoadoutDetail(notOnGear));

		Check("loadout.not_a_character", GDC_LoadoutAnalysis.ReadPrefab("{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et") == null);
	}

	//------------------------------------------------------------------------------------------------
	protected static GDC_LoadoutAnalysis ReadTest(string name)
	{
		return GDC_LoadoutAnalysis.ReadPrefab(Workbench.GetResourceName(CHARACTERS + name + ".et"));
	}

	//------------------------------------------------------------------------------------------------
	protected static string WeaponLabel(GDC_LoadoutAnalysis loadout)
	{
		if (!loadout)
			return string.Empty;

		foreach (GDC_LoadoutWeaponSlot weapon : loadout.m_aWeapons)
		{
			if (weapon && weapon.m_sKey.EndsWith(":0"))
				return weapon.m_sLabel;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string LoadoutDetail(GDC_LoadoutAnalysis loadout)
	{
		if (!loadout)
			return "not readable";

		return string.Format("spare %1, over limit %2, not on gear %3, missing %4, weight %5 / %6", loadout.GetPrimarySpareMagazines(),
			loadout.m_aOverLimit.Count(), loadout.m_aNotOnGear.Count(), loadout.m_aMissing.Count(), loadout.GetTotalWeight(), loadout.m_fMaxWeight);
	}
}
#endif
