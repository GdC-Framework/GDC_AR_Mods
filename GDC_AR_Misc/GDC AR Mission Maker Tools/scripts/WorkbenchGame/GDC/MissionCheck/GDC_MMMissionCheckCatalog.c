#ifdef WORKBENCH
//! Catalogue of the Check Mission blocks and rules: the rules layer, data only. It is the single source of the texts
//! shown in the settings dialog and the report, in English and French side by side.
//! Adding a rule: declare it here (texts, family, verification), then report it from its block by its number with
//! GDC_MMCheckReport.Add. The reasons behind a rule (sources, history) go to its knowledge base sheet (m_sKB).

//------------------------------------------------------------------------------------------------
class GDC_MMCheckRule
{
	string m_sId;						//!< "2.1": block number, rule number
	int m_iBlock;
	string m_sFramework;				//!< GDC_MMCheckFramework value
	GDC_EMMCheckSeverity m_eSeverity;
	GDC_EMMCheckFamily m_eFamily;
	string m_sKB;						//!< id of its knowledge base sheet, empty if none

	// MODSET rules: the addon whose behaviour the rule rests on, and when it was last verified
	string m_sVerifiedAddon;			//!< addon ID, as in addon.gproj
	string m_sVerifiedVersion;
	string m_sVerifiedOn;				//!< YYYY-MM-DD

	// texts, English then French; problem and fix may hold %1 and %2, given by the block reporting the finding
	protected ref array<string> m_aTitle;		//!< short, also the title of its findings
	protected ref array<string> m_aProblem;		//!< what goes wrong in the finding, one sentence
	protected ref array<string> m_aFix;			//!< what to do, one sentence

	//------------------------------------------------------------------------------------------------
	void SetTitle(string english, string french)
	{
		m_aTitle = { english, french };
	}

	//------------------------------------------------------------------------------------------------
	void SetProblem(string english, string french)
	{
		m_aProblem = { english, french };
	}

	//------------------------------------------------------------------------------------------------
	void SetFix(string english, string french)
	{
		m_aFix = { english, french };
	}

	//------------------------------------------------------------------------------------------------
	void SetVerified(string addon, string version, string date)
	{
		m_sVerifiedAddon = addon;
		m_sVerifiedVersion = version;
		m_sVerifiedOn = date;
	}

	//------------------------------------------------------------------------------------------------
	string GetTitle()
	{
		return Pick(m_aTitle);
	}

	//------------------------------------------------------------------------------------------------
	string GetProblem()
	{
		return Pick(m_aProblem);
	}

	//------------------------------------------------------------------------------------------------
	string GetFix()
	{
		return Pick(m_aFix);
	}

	//------------------------------------------------------------------------------------------------
	protected static string Pick(array<string> texts)
	{
		if (!texts)
			return string.Empty;

		return GDC_MMCheckText.T(texts[0], texts[1]);
	}
}

//------------------------------------------------------------------------------------------------
class GDC_MMCheckCatalog
{
	protected static ref array<ref GDC_MMCheckRule> s_aRules;

	//! Where the knowledge base sheets are read: the public GitHub repository, their only home (the GDC wiki links to them)
	protected static const string SHEET_URL = "https://github.com/GdC-Framework/GDC_AR_Mods/blob/main/GDC_AR_Misc/GDC%20AR%20Mission%20Maker%20Tools/GDC_AR_MissionCheck_KB/";

	//------------------------------------------------------------------------------------------------
	//! \return address of a knowledge base sheet in the current language, empty without a sheet
	static string GetSheetUrl(string id)
	{
		if (id.IsEmpty())
			return string.Empty;

		return SHEET_URL + id + GDC_MMCheckText.T(".md", ".fr.md");
	}

	//------------------------------------------------------------------------------------------------
	//! \return block titles in the current language, index = block number - 1
	static array<string> GetBlockTitles()
	{
		if (GDC_MMCheckText.s_eLanguage == GDC_EMMCheckLanguage.FRENCH)
			return { "Structure", "Ressources et addons", "Joueurs et slots", "Réglages des systèmes", "Logique de mission", "IA", "Pré-publication" };

		return { "Structure", "Resources and addons", "Players and slots", "System settings", "Mission logic", "AI", "Pre-publication" };
	}

	//------------------------------------------------------------------------------------------------
	static string GetBlockTitle(int block)
	{
		array<string> titles = GetBlockTitles();
		if (block < 1 || block > titles.Count())
			return string.Empty;

		return titles[block - 1];
	}

	//------------------------------------------------------------------------------------------------
	static array<ref GDC_MMCheckRule> GetRules()
	{
		if (s_aRules)
			return s_aRules;

		s_aRules = {};
		GDC_MMCheckRule rule;

		// ---- 2. Resources and addons --------------------------------------------------------------
		rule = Declare("2.1", GDC_MMCheckFramework.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Missing resource", "Ressource introuvable");
		rule.SetProblem(
			"the property points to %1, found in none of the loaded addons.",
			"la propriété pointe vers %1, absente de tous les addons chargés.");
		rule.SetFix(
			"load the addon that holds it, or pick another resource.",
			"chargez l'addon qui la contient, ou choisissez une autre ressource.");

		rule = Declare("2.2", GDC_MMCheckFramework.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("MM library proxy used", "Proxy de bibliothèque MM utilisé");
		rule.SetProblem(
			"%1 is a library proxy, not the real prefab: the mission would need your GDC Mission Maker Local addon.",
			"%1 est un proxy de bibliothèque, pas le vrai prefab : la mission dépendrait de votre addon GDC Mission Maker Local.");
		rule.SetFix(
			"use the real prefab: drop the item again from the Prefab Library, or pick the real prefab in the property (Resource Browser).",
			"utilisez le vrai prefab : redéposez l'élément depuis la Prefab Library, ou choisissez le vrai prefab dans la propriété (Resource Browser).");
		rule.m_sKB = "gdc-mm-library-proxy";

		rule = Declare("2.3", GDC_MMCheckFramework.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Entity without its prefab", "Entité sans son prefab");
		rule.SetProblem(
			"a bare %1 with no prefab: its prefab was probably stripped by saving the world while the prefab's addon was not loaded.",
			"un %1 nu, sans prefab : il a sans doute été retiré en enregistrant le monde alors que l'addon du prefab n'était pas chargé.");
		rule.SetFix(
			"close the world without saving, restore the layer from git, load the missing addon, then reopen the world.",
			"fermez le monde sans enregistrer, restaurez le calque depuis git, chargez l'addon manquant, puis rouvrez le monde.");

		rule = Declare("2.4", GDC_MMCheckFramework.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Unknown resource GUID", "GUID de ressource inconnu");
		rule.SetProblem(
			"the property points to %1: this GUID is unknown, but a file with another GUID exists at this path.",
			"la propriété pointe vers %1 : ce GUID est inconnu, mais un fichier d'un autre GUID existe à ce chemin.");
		rule.SetFix(
			"pick the resource again in this property.",
			"choisissez à nouveau la ressource dans cette propriété.");

		// ---- 4. System settings -------------------------------------------------------------------
		rule = Declare("4.1", GDC_MMCheckFramework.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("Entity deleter without a filter", "Deleter sans filtre");
		rule.SetProblem(
			"with an empty prefab filter, it deletes every root entity in its sphere, managers and AI included.",
			"avec un filtre de prefabs vide, il supprime toutes les entités racines de sa sphère, managers et IA compris.");
		rule.SetFix(
			"fill its prefab filter list, or check that only what must go is within its radius.",
			"remplissez sa liste de filtres de prefabs, ou vérifiez que seul ce qui doit disparaître est dans son rayon.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-20");
		rule.m_sKB = "tilw-entity-deleter";

		// ---- 5. Mission logic ---------------------------------------------------------------------
		rule = Declare("5.1", GDC_MMCheckFramework.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("Interaction placed as a child", "Interaction placée en enfant");
		rule.SetProblem(
			"a TILW interaction under another entity may lose its action label on a dedicated server.",
			"une interaction TILW sous une autre entité peut perdre le libellé de son action sur un serveur dédié.");
		rule.SetFix(
			"place it as a root entity at the object's position, with \"Display UI At Reference Point\" ticked on its UserActionContext.",
			"placez-la en entité racine à la position de l'objet, avec « Display UI At Reference Point » coché sur son UserActionContext.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-20");
		rule.m_sKB = "tilw-interaction-child";

		rule = Declare("5.2", GDC_MMCheckFramework.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("Vehicle crew that never spawns", "Équipage de véhicule qui n'apparaît jamais");
		rule.SetProblem(
			"TILW_VehicleCrewComponent does nothing with Reforger Lobby: TILW's lobby code is compiled out, so the vehicle stays empty.",
			"TILW_VehicleCrewComponent ne fait rien avec Reforger Lobby : le code lobby de TILW n'est pas compilé, le véhicule reste vide.");
		rule.SetFix(
			"spawn the vehicle and its crew with a TILW_PrefabSpawner (crew config, pregame spawn ticked).",
			"faites apparaître le véhicule et son équipage avec un TILW_PrefabSpawner (config d'équipage, apparition en pregame cochée).");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-10");
		rule.m_sKB = "tilw-vehicle-crew";

		return s_aRules;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the rule, null if the id is not declared
	static GDC_MMCheckRule Get(string id)
	{
		foreach (GDC_MMCheckRule rule : GetRules())
		{
			if (rule.m_sId == id)
				return rule;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static array<GDC_MMCheckRule> GetBlockRules(int block)
	{
		array<GDC_MMCheckRule> rules = {};
		foreach (GDC_MMCheckRule rule : GetRules())
		{
			if (rule.m_iBlock == block)
				rules.Insert(rule);
		}

		return rules;
	}

	//------------------------------------------------------------------------------------------------
	//! One line per block, for the settings dialog: "2. Resources and addons: missing resource, MM library proxy used, ..."
	static string DescribeBlocks()
	{
		string text;
		string colon = GDC_MMCheckText.T(": ", " : ");
		foreach (int i, string title : GetBlockTitles())
		{
			int block = i + 1;
			string line = block.ToString() + ". " + title + colon;
			array<GDC_MMCheckRule> rules = GetBlockRules(block);
			if (rules.IsEmpty())
			{
				line += GDC_MMCheckText.T("coming soon", "à venir");
			}
			else
			{
				foreach (int r, GDC_MMCheckRule rule : rules)
				{
					if (r > 0)
						line += ", ";

					line += Uncapitalize(rule.GetTitle());
				}
			}

			if (!text.IsEmpty())
				text += "\n";

			text += line;
		}

		return text;
	}

	//------------------------------------------------------------------------------------------------
	protected static GDC_MMCheckRule Declare(string id, string framework, GDC_EMMCheckSeverity severity, GDC_EMMCheckFamily family)
	{
		GDC_MMCheckRule rule = new GDC_MMCheckRule();
		rule.m_sId = id;
		rule.m_iBlock = id.Substring(0, id.IndexOf(".")).ToInt();
		rule.m_sFramework = framework;
		rule.m_eSeverity = severity;
		rule.m_eFamily = family;
		s_aRules.Insert(rule);
		return rule;
	}

	//------------------------------------------------------------------------------------------------
	//! Lowercases the first letter of a title, unless it starts an acronym ("MM library" stays as is).
	protected static string Uncapitalize(string text)
	{
		if (text.Length() < 2)
			return text;

		// ToLower only handles ASCII; the only accented capital starting a title
		if (text.StartsWith("É"))
			return "é" + text.Substring(2, text.Length() - 2);

		string second = text.Substring(1, 1);
		string secondLower = second;
		secondLower.ToLower();
		if (second != secondLower)
			return text;

		string first = text.Substring(0, 1);
		first.ToLower();
		return first + text.Substring(1, text.Length() - 1);
	}
}
#endif
