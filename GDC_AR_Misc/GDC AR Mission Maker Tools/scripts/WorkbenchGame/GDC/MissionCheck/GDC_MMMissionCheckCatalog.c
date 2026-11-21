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
	string m_sOrigin;				//!< GDC_MMCheckOrigin value
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
//! An addon GDC missions must not load (rule 2.4), with the reason; its knowledge base sheet: gdc-excluded-addons.
class GDC_MMCheckExcludedAddon
{
	string m_sGUID;
	string m_sName;
	string m_sOnlyWith;					//!< excluded only when the mission places an entity of this class; empty: always
	protected ref array<string> m_aReason;	//!< English, French; one sentence ending with a full stop

	//------------------------------------------------------------------------------------------------
	void GDC_MMCheckExcludedAddon(string guid, string name, string onlyWith, string englishReason, string frenchReason)
	{
		m_sGUID = guid;
		m_sName = name;
		m_sOnlyWith = onlyWith;
		m_aReason = { englishReason, frenchReason };
	}

	//------------------------------------------------------------------------------------------------
	string GetReason()
	{
		return GDC_MMCheckText.T(m_aReason[0], m_aReason[1]);
	}
}

//------------------------------------------------------------------------------------------------
class GDC_MMCheckCatalog
{
	protected static ref array<ref GDC_MMCheckExcludedAddon> s_aExcludedAddons;

	//------------------------------------------------------------------------------------------------
	//! \return addons GDC missions must not load (rule 2.4); keep the sheet gdc-excluded-addons in step
	static array<ref GDC_MMCheckExcludedAddon> GetExcludedAddons()
	{
		if (s_aExcludedAddons)
			return s_aExcludedAddons;

		s_aExcludedAddons = {};
		s_aExcludedAddons.Insert(new GDC_MMCheckExcludedAddon("69A510CE600D1126", "Anarchy Map Markers & Drawings", "PS_GameModeCoop",
			"it conflicts with Reforger Lobby (input of the quick marker menu, OnInputQuickMarkerMenu).",
			"il entre en conflit avec Reforger Lobby (entrée du menu rapide des marqueurs, OnInputQuickMarkerMenu)."));
		return s_aExcludedAddons;
	}

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
			return { "Structure de la mission", "Ressources et addons", "Joueurs et slots", "Réglages des systèmes", "Logique de mission", "IA", "Pré-publication" };

		return { "Mission structure", "Resources and addons", "Players and slots", "System settings", "Mission logic", "AI", "Pre-publication" };
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

		// ---- 1. Mission structure (compared with the mission skeleton) -----------------------------
		rule = Declare("1.1", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Skeleton layers missing", "Calques du squelette absents");
		rule.SetProblem(
			"the mission lacks these layers or folders of the mission skeleton: %1.",
			"il manque à la mission ces calques ou dossiers du squelette de mission : %1.");
		rule.SetFix(
			"run Create Mission Skeleton to add them, or ignore this if the mission is organised otherwise.",
			"lancez Create Mission Skeleton pour les ajouter, ou ignorez ceci si la mission est organisée autrement.");

		rule = Declare("1.2", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Game mode missing or duplicated", "Mode de jeu absent ou en double");
		rule.SetProblem(
			"the mission has %1 game modes: it needs exactly one.",
			"la mission a %1 modes de jeu : il en faut exactement un.");
		rule.SetFix(
			"keep a single game mode; without one, run Create Mission Skeleton.",
			"gardez un seul mode de jeu ; s'il n'y en a aucun, lancez Create Mission Skeleton.");

		rule = Declare("1.3", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Skeleton entity missing", "Entité du squelette absente");
		rule.SetProblem(
			"the world has no %1 (addon %2), expected by the mission skeleton.",
			"le monde n'a pas de %1 (addon %2), prévu par le squelette de mission.");
		rule.SetFix(
			"run Create Mission Skeleton, which adds it, or ignore this if the mission does without it.",
			"lancez Create Mission Skeleton, qui l'ajoute, ou ignorez ceci si la mission s'en passe.");

		rule = Declare("1.4", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("AI world navmesh without a file", "Navmesh de l'AI World sans fichier");
		rule.SetProblem(
			"the %1 navmesh has no file: AI cannot move on it.",
			"la navmesh %1 n'a pas de fichier : les IA ne peuvent pas s'y déplacer.");
		rule.SetFix(
			"set its NavmeshFile (NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig).",
			"renseignez son NavmeshFile (NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig).");

		rule = Declare("1.5", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Briefing sections missing", "Sections du briefing absentes");
		rule.SetProblem(
			"the briefing lacks these sections of the mission skeleton: %1.",
			"il manque au briefing ces sections du squelette de mission : %1.");
		rule.SetFix(
			"add them in the briefing layer, or ignore this if the briefing is organised otherwise.",
			"ajoutez-les dans le calque du briefing, ou ignorez ceci si le briefing est organisé autrement.");

		rule = Declare("1.6", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Briefing section empty", "Section du briefing vide");
		rule.SetProblem(
			"the \"%1\" section has no text.",
			"la section « %1 » n'a pas de texte.");
		rule.SetFix(
			"write it, or delete the section.",
			"rédigez-la, ou supprimez la section.");

		rule = Declare("1.7", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Briefing section not written", "Section du briefing non rédigée");
		rule.SetProblem(
			"the \"%1\" section still holds the hint of the mission skeleton.",
			"la section « %1 » contient encore l'indication du squelette de mission.");
		rule.SetFix(
			"replace the hint with the mission's text.",
			"remplacez l'indication par le texte de la mission.");

		// ---- 2. Resources and addons --------------------------------------------------------------
		rule = Declare("2.1", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Missing resource", "Ressource introuvable");
		rule.SetProblem(
			"the property points to %1, found in none of the loaded addons.",
			"la propriété pointe vers %1, absente de tous les addons chargés.");
		rule.SetFix(
			"load the addon that holds it, or pick another resource.",
			"chargez l'addon qui la contient, ou choisissez une autre ressource.");

		rule = Declare("2.2", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("MM library proxy used", "Proxy de bibliothèque MM utilisé");
		rule.SetProblem(
			"%1 is a library proxy, not the real prefab: the mission would need your GDC Mission Maker Local addon.",
			"%1 est un proxy de bibliothèque, pas le vrai prefab : la mission dépendrait de votre addon GDC Mission Maker Local.");
		rule.SetFix(
			"use the real prefab: drop the item again from the Prefab Library, or pick the real prefab in the property (Resource Browser).",
			"utilisez le vrai prefab : redéposez l'élément depuis la Prefab Library, ou choisissez le vrai prefab dans la propriété (Resource Browser).");
		rule.m_sKB = "gdc-mm-library-proxy";

		rule = Declare("2.3", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Entity without its prefab", "Entité sans son prefab");
		rule.SetProblem(
			"a %1 without its prefab: the addon of the prefab is not loaded, or the prefab was stripped by saving the world without that addon.",
			"un %1 sans son prefab : l'addon du prefab n'est pas chargé, ou le prefab a été retiré en enregistrant le monde sans cet addon.");
		rule.SetFix(
			"load the addon of the prefab and reopen the world, without saving it before; if the entity stays without prefab, restore its layer from git.",
			"chargez l'addon du prefab et rouvrez le monde, sans l'enregistrer avant ; si l'entité reste sans prefab, restaurez son calque depuis git.");

		rule = Declare("2.4", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Excluded addon loaded", "Addon exclu chargé");
		rule.SetProblem(
			"%1 is loaded: %2",
			"%1 est chargé : %2");
		rule.SetFix(
			"remove %1 from the dependencies of the mission addon, and from the server's mod list.",
			"retirez %1 des dépendances de l'addon de la mission, et de la liste des mods du serveur.");
		rule.m_sKB = "gdc-excluded-addons";

		// ---- 3. Players and slots -----------------------------------------------------------------
		rule = Declare("3.1", GDC_MMCheckOrigin.RL, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("No playable group", "Aucun groupe jouable");
		rule.SetProblem(
			"no AI group has Set Playable ticked: Reforger Lobby has no slot to offer.",
			"aucun groupe d'IA n'a Set Playable coché : Reforger Lobby n'a aucun slot à proposer.");
		rule.SetFix(
			"tick Set Playable (m_bSetPlayable) on the groups of the players.",
			"cochez Set Playable (m_bSetPlayable) sur les groupes des joueurs.");
		rule.SetVerified("ReforgerLobbyCommunity", "1.8.23", "2026-11-21");
		rule.m_sKB = "rl-playable-groups";

		rule = Declare("3.2", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Playable group without a faction", "Groupe jouable sans faction");
		rule.SetProblem(
			"the group has no faction (m_faction), neither set in the mission nor in its prefab: its slots belong to no side.",
			"le groupe n'a pas de faction (m_faction), ni dans la mission ni dans son prefab : ses slots n'appartiennent à aucun camp.");
		rule.SetFix(
			"set its faction, or use a group prefab that declares one (the RHS group prefabs declare none).",
			"renseignez sa faction, ou utilisez un prefab de groupe qui en déclare une (les prefabs de groupes RHS n'en déclarent pas).");

		rule = Declare("3.3", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Slot frequency list of the wrong length", "Liste de fréquences de mauvaise longueur");
		rule.SetProblem(
			"the list has %1 entries for %2 slots: its frequencies do not match the slots, in order.",
			"la liste a %1 entrées pour %2 slots : ses fréquences ne correspondent pas aux slots, dans l'ordre.");
		rule.SetFix(
			"give one entry per slot, in the order of the unit prefab slots (an empty entry keeps the default).",
			"mettez une entrée par slot, dans l'ordre des slots du groupe (une entrée vide garde la valeur par défaut).");

		rule = Declare("3.4", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Teleport module nobody can use", "Module de téléportation inutilisable");
		rule.SetProblem(
			"in ALL_PLAYERS mode it requires the rank %1, and no playable slot reaches it (highest: %2).",
			"en mode ALL_PLAYERS il exige le grade %1, qu'aucun slot jouable n'atteint (le plus haut : %2).");
		rule.SetFix(
			"lower its minimum rank (m_eMinRankRequired, INVALID for none), or give that rank to the slot meant to use it.",
			"baissez son grade minimum (m_eMinRankRequired, INVALID pour aucun), ou donnez ce grade au slot qui doit l'utiliser.");

		rule = Declare("3.5", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Radio that is not a Coalition VON radio", "Radio qui n'est pas une radio Coalition VON");
		rule.SetProblem(
			"the slot %1 carries %2, which is not a Coalition VON radio: it does not work with Coalition VON.",
			"le slot %1 porte %2, qui n'est pas une radio Coalition VON : elle ne fonctionne pas avec Coalition VON.");
		rule.SetFix(
			"replace it in the loadout of the slot prefab with a Coalition VON radio (AN/PRC-152, R-148, R-168...).",
			"remplacez-la dans le loadout du prefab de slot par une radio Coalition VON (AN/PRC-152, R-148, R-168...).");
		rule.m_sKB = "gdc-cvon-radios";

		rule = Declare("3.6", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout over its limits", "Loadout qui déborde");
		rule.SetProblem(
			"the slot %1 starts with more than its gear can hold (%2 storage problems: capacity, weight, size or item type): some items will not be spawned.",
			"le slot %1 démarre avec plus que son équipement ne peut contenir (%2 problèmes de rangement : capacité, poids, taille ou type d'objet) : des objets ne seront pas créés.");
		rule.SetFix(
			"open the slot prefab in the Loadout Editor (GDC backpack icon): it shows which storage overflows.",
			"ouvrez le prefab du slot dans l'éditeur de loadouts (icône sac à dos GDC) : il montre quel rangement déborde.");

		rule = Declare("3.7", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Primary weapon without a spare magazine", "Arme principale sans chargeur de rechange");
		rule.SetProblem(
			"the slot %1 has no spare magazine for its primary weapon %2.",
			"le slot %1 n'a aucun chargeur de rechange pour son arme principale %2.");
		rule.SetFix(
			"add magazines for it to the loadout (Loadout Editor, Items To Distribute).",
			"ajoutez-lui des chargeurs dans le loadout (éditeur de loadouts, Items To Distribute).");

		rule = Declare("3.8", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout item missing", "Objet du loadout introuvable");
		rule.SetProblem(
			"the slot %1 uses prefabs that no loaded addon provides (%2): they will not be spawned.",
			"le slot %1 utilise des prefabs qu'aucun addon chargé ne fournit (%2) : ils ne seront pas créés.");
		rule.SetFix(
			"load the addon that provides them, or replace them in the loadout of the slot prefab.",
			"chargez l'addon qui les fournit, ou remplacez-les dans le loadout du prefab de slot.");

		rule = Declare("3.9", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout heavier than the character can carry", "Loadout plus lourd que ce que le personnage peut porter");
		rule.SetProblem(
			"the loadout of slot %1 weighs about %2 kg, above the %3 kg the character can carry: some items may not be spawned.",
			"le loadout du slot %1 pèse environ %2 kg, au-delà des %3 kg que le personnage peut porter : des objets risquent de ne pas être créés.");
		rule.SetFix(
			"lighten the loadout (Loadout Editor).",
			"allégez le loadout (éditeur de loadouts).");

		rule = Declare("3.10", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout items aimed at a missing storage", "Objets du loadout destinés à un rangement absent");
		rule.SetProblem(
			"the slot %1 puts items in %2 storages that are not on its gear (a pouch of a former vest, for example): they will not be where expected.",
			"le slot %1 range des objets dans %2 rangements absents de son équipement (une pochette d'un ancien gilet, par exemple) : ils ne seront pas là où on les attend.");
		rule.SetFix(
			"open the slot prefab in the Loadout Editor and press Refresh Containers: it moves them to Items To Distribute.",
			"ouvrez le prefab du slot dans l'éditeur de loadouts et cliquez sur Refresh Containers : il les déplace dans Items To Distribute.");

		// ---- 4. System settings -------------------------------------------------------------------
		rule = Declare("4.1", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
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
		rule = Declare("5.1", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("Interaction placed as a child", "Interaction placée en enfant");
		rule.SetProblem(
			"a TILW interaction under another entity may lose its action label on a dedicated server.",
			"une interaction TILW sous une autre entité peut perdre le libellé de son action sur un serveur dédié.");
		rule.SetFix(
			"place it as a root entity at the object's position, with \"Display UI At Reference Point\" ticked on its UserActionContext.",
			"placez-la en entité racine à la position de l'objet, avec « Display UI At Reference Point » coché sur son UserActionContext.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-20");
		rule.m_sKB = "tilw-interaction-child";

		rule = Declare("5.2", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
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
	protected static GDC_MMCheckRule Declare(string id, string origin, GDC_EMMCheckSeverity severity, GDC_EMMCheckFamily family)
	{
		GDC_MMCheckRule rule = new GDC_MMCheckRule();
		rule.m_sId = id;
		rule.m_iBlock = id.Substring(0, id.IndexOf(".")).ToInt();
		rule.m_sOrigin = origin;
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
