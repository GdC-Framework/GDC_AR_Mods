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
	protected ref array<string> m_aDescription;	//!< what is checked, one short sentence (list of the checks in the user guide)
	protected ref array<string> m_aProblem;		//!< what goes wrong in the finding, one sentence
	protected ref array<string> m_aFix;			//!< what to do, one sentence

	//------------------------------------------------------------------------------------------------
	void SetTitle(string english, string french)
	{
		m_aTitle = { english, french };
	}

	//------------------------------------------------------------------------------------------------
	void SetDescription(string english, string french)
	{
		m_aDescription = { english, french };
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
	string GetDescription()
	{
		return Pick(m_aDescription);
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

		// ---- 1. Mission structure (compared with the mission skeleton) -----------------------------------
		rule = Declare("1.1", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Skeleton layers missing", "Calques du squelette absents");
		rule.SetDescription(
			"The root layers and folders of the mission skeleton exist.",
			"Les calques et dossiers racines du squelette de mission existent.");
		rule.SetProblem(
			"the mission lacks these layers or folders of the mission skeleton: %1.",
			"il manque à la mission ces calques ou dossiers du squelette de mission : %1.");
		rule.SetFix(
			"run Create Mission Skeleton to add them, or ignore this if the mission is organised otherwise.",
			"lancez Create Mission Skeleton pour les ajouter, ou ignorez ceci si la mission est organisée autrement.");

		rule = Declare("1.2", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Game mode missing or duplicated", "Mode de jeu absent ou en double");
		rule.SetDescription(
			"The mission has exactly one game mode.",
			"La mission a exactement un mode de jeu.");
		rule.SetProblem(
			"the mission has %1 game modes: it needs exactly one.",
			"la mission a %1 modes de jeu : il en faut exactement un.");
		rule.SetFix(
			"keep a single game mode; without one, run Create Mission Skeleton.",
			"gardez un seul mode de jeu ; s'il n'y en a aucun, lancez Create Mission Skeleton.");

		rule = Declare("1.3", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Skeleton entity missing", "Entité du squelette absente");
		rule.SetDescription(
			"The entities of the mission skeleton (AI world, managers, mission framework) exist.",
			"Les entités du squelette de mission (AI world, managers, framework de mission) existent.");
		rule.SetProblem(
			"the world has no %1 (addon %2), expected by the mission skeleton.",
			"le monde n'a pas de %1 (addon %2), prévu par le squelette de mission.");
		rule.SetFix(
			"run Create Mission Skeleton, which adds it, or ignore this if the mission does without it.",
			"lancez Create Mission Skeleton, qui l'ajoute, ou ignorez ceci si la mission s'en passe.");

		rule = Declare("1.4", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("AI world navmesh without a file", "Navmesh de l'AI World sans fichier");
		rule.SetDescription(
			"Every navmesh of the AI world has a navmesh file.",
			"Chaque navmesh de l'AI world a un fichier.");
		rule.SetProblem(
			"the %1 navmesh has no file: AI cannot move on it.",
			"la navmesh %1 n'a pas de fichier : les IA ne peuvent pas s'y déplacer.");
		rule.SetFix(
			"set its NavmeshFile (NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig).",
			"renseignez son NavmeshFile (NavmeshWorldComponent > NavmeshSettings > NavmeshFilesConfig).");

		rule = Declare("1.5", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Briefing sections missing", "Sections du briefing absentes");
		rule.SetDescription(
			"The briefing has the sections of the mission skeleton.",
			"Le briefing a les sections du squelette de mission.");
		rule.SetProblem(
			"the briefing lacks these sections of the mission skeleton: %1.",
			"il manque au briefing ces sections du squelette de mission : %1.");
		rule.SetFix(
			"add them in the briefing layer, or ignore this if the briefing is organised otherwise.",
			"ajoutez-les dans le calque du briefing, ou ignorez ceci si le briefing est organisé autrement.");

		rule = Declare("1.6", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Briefing section empty", "Section du briefing vide");
		rule.SetDescription(
			"No briefing section is empty.",
			"Aucune section du briefing n'est vide.");
		rule.SetProblem(
			"the \"%1\" section has no text.",
			"la section « %1 » n'a pas de texte.");
		rule.SetFix(
			"write it, or delete the section.",
			"rédigez-la, ou supprimez la section.");

		rule = Declare("1.7", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Briefing section not written", "Section du briefing non rédigée");
		rule.SetDescription(
			"No briefing section still holds the hint of the mission skeleton.",
			"Aucune section du briefing ne contient encore l'indication du squelette.");
		rule.SetProblem(
			"the \"%1\" section still holds the hint of the mission skeleton.",
			"la section « %1 » contient encore l'indication du squelette de mission.");
		rule.SetFix(
			"replace the hint with the mission's text.",
			"remplacez l'indication par le texte de la mission.");

		// ---- 2. Resources and addons ---------------------------------------------------------------------
		rule = Declare("2.1", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Missing resource", "Ressource introuvable");
		rule.SetDescription(
			"Every resource a property of the mission points to exists in the loaded addons.",
			"Chaque ressource visée par une propriété de la mission existe dans les addons chargés.");
		rule.SetProblem(
			"the property points to %1, found in none of the loaded addons.",
			"la propriété pointe vers %1, absente de tous les addons chargés.");
		rule.SetFix(
			"load the addon that holds it, or pick another resource.",
			"chargez l'addon qui la contient, ou choisissez une autre ressource.");

		rule = Declare("2.2", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("MM library proxy used", "Proxy de bibliothèque MM utilisé");
		rule.SetDescription(
			"No entity or property uses a Prefab Library item instead of its real prefab.",
			"Aucune entité ni propriété n'utilise un élément de la Prefab Library au lieu de son vrai prefab.");
		rule.SetProblem(
			"%1 is a library proxy, not the real prefab: the mission would need your GDC Mission Maker Local addon.",
			"%1 est un proxy de bibliothèque, pas le vrai prefab : la mission dépendrait de votre addon GDC Mission Maker Local.");
		rule.SetFix(
			"use the real prefab: drop the item again from the Prefab Library, or pick the real prefab in the property (Resource Browser).",
			"utilisez le vrai prefab : redéposez l'élément depuis la Prefab Library, ou choisissez le vrai prefab dans la propriété (Resource Browser).");
		rule.m_sKB = "gdc-mm-library-proxy";

		rule = Declare("2.3", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Entity without its prefab", "Entité sans son prefab");
		rule.SetDescription(
			"No AI group, character or vehicle is placed without its prefab.",
			"Aucun groupe d'IA, personnage ou véhicule n'est posé sans son prefab.");
		rule.SetProblem(
			"a %1 without its prefab: the addon of the prefab is not loaded, or the prefab was stripped by saving the world without that addon.",
			"un %1 sans son prefab : l'addon du prefab n'est pas chargé, ou le prefab a été retiré en enregistrant le monde sans cet addon.");
		rule.SetFix(
			"load the addon of the prefab and reopen the world, without saving it before; if the entity stays without prefab, restore its layer from git.",
			"chargez l'addon du prefab et rouvrez le monde, sans l'enregistrer avant ; si l'entité reste sans prefab, restaurez son calque depuis git.");

		rule = Declare("2.4", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Excluded addon loaded", "Addon exclu chargé");
		rule.SetDescription(
			"No addon excluded from GDC missions is loaded.",
			"Aucun addon exclu des missions GDC n'est chargé.");
		rule.SetProblem(
			"%1 is loaded: %2",
			"%1 est chargé : %2");
		rule.SetFix(
			"remove %1 from the dependencies of the mission addon, and from the server's mod list.",
			"retirez %1 des dépendances de l'addon de la mission, et de la liste des mods du serveur.");
		rule.m_sKB = "gdc-excluded-addons";

		// ---- 3. Players and slots ------------------------------------------------------------------------
		rule = Declare("3.1", GDC_MMCheckOrigin.RL, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("No playable group", "Aucun groupe jouable");
		rule.SetDescription(
			"With Reforger Lobby, at least one AI group is playable.",
			"Avec Reforger Lobby, au moins un groupe d'IA est jouable.");
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
		rule.SetDescription(
			"Every playable group has a faction.",
			"Chaque groupe jouable a une faction.");
		rule.SetProblem(
			"the group has no faction (m_faction), neither set in the mission nor in its prefab: its slots belong to no side.",
			"le groupe n'a pas de faction (m_faction), ni dans la mission ni dans son prefab : ses slots n'appartiennent à aucun camp.");
		rule.SetFix(
			"set its faction, or use a group prefab that declares one (the RHS group prefabs declare none).",
			"renseignez sa faction, ou utilisez un prefab de groupe qui en déclare une (les prefabs de groupes RHS n'en déclarent pas).");

		rule = Declare("3.3", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC slot frequency list of the wrong length", "Liste de fréquences GDC par slot de mauvaise longueur");
		rule.SetDescription(
			"A slot frequency list, when filled, has one entry per slot.",
			"Une liste de fréquences par slot, si elle est remplie, a une entrée par slot.");
		rule.SetProblem(
			"the list has %1 entries for %2 slots: its frequencies do not match the slots, in order.",
			"la liste a %1 entrées pour %2 slots : ses fréquences ne correspondent pas aux slots, dans l'ordre.");
		rule.SetFix(
			"give one entry per slot, in the order of the unit prefab slots (an empty entry keeps the default).",
			"mettez une entrée par slot, dans l'ordre des slots du groupe (une entrée vide garde la valeur par défaut).");

		rule = Declare("3.4", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Radio that is not a Coalition VON radio", "Radio qui n'est pas une radio Coalition VON");
		rule.SetDescription(
			"With Coalition VON, the radios the playable slots start with are Coalition VON radios.",
			"Avec Coalition VON, les radios de départ des slots jouables sont des radios Coalition VON.");
		rule.SetProblem(
			"the slot %1 carries %2, which is not a Coalition VON radio: it does not work with Coalition VON.",
			"le slot %1 porte %2, qui n'est pas une radio Coalition VON : elle ne fonctionne pas avec Coalition VON.");
		rule.SetFix(
			"replace it in the loadout of the slot prefab with a Coalition VON radio (AN/PRC-152, R-148, R-168...).",
			"remplacez-la dans le loadout du prefab de slot par une radio Coalition VON (AN/PRC-152, R-148, R-168...).");
		rule.m_sKB = "gdc-cvon-radios";

		rule = Declare("3.5", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout over its limits", "Loadout qui déborde");
		rule.SetDescription(
			"The loadout of each playable slot fits in its gear.",
			"Le loadout de chaque slot jouable tient dans son équipement.");
		rule.SetProblem(
			"the slot %1 starts with more than its gear can hold (%2 storage problems: capacity, weight, size or item type): some items will not be spawned.",
			"le slot %1 démarre avec plus que son équipement ne peut contenir (%2 problèmes de rangement : capacité, poids, taille ou type d'objet) : des objets ne seront pas créés.");
		rule.SetFix(
			"open the slot prefab in the Loadout Editor (GDC backpack icon): it shows which storage overflows.",
			"ouvrez le prefab du slot dans l'éditeur de loadouts (icône sac à dos GDC) : il montre quel rangement déborde.");

		rule = Declare("3.6", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Primary weapon without a spare magazine", "Arme principale sans chargeur de rechange");
		rule.SetDescription(
			"The primary weapon of each playable slot has a spare magazine.",
			"L'arme principale de chaque slot jouable a un chargeur de rechange.");
		rule.SetProblem(
			"the slot %1 has no spare magazine for its primary weapon %2.",
			"le slot %1 n'a aucun chargeur de rechange pour son arme principale %2.");
		rule.SetFix(
			"add magazines for it to the loadout (Loadout Editor, Items To Distribute).",
			"ajoutez-lui des chargeurs dans le loadout (éditeur de loadouts, Items To Distribute).");

		rule = Declare("3.7", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout item missing", "Objet du loadout introuvable");
		rule.SetDescription(
			"Every prefab of the loadout of a playable slot is provided by a loaded addon.",
			"Chaque prefab du loadout d'un slot jouable est fourni par un addon chargé.");
		rule.SetProblem(
			"the slot %1 uses prefabs that no loaded addon provides (%2): they will not be spawned.",
			"le slot %1 utilise des prefabs qu'aucun addon chargé ne fournit (%2) : ils ne seront pas créés.");
		rule.SetFix(
			"load the addon that provides them, or replace them in the loadout of the slot prefab.",
			"chargez l'addon qui les fournit, ou remplacez-les dans le loadout du prefab de slot.");

		rule = Declare("3.8", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout heavier than the character can carry", "Loadout plus lourd que ce que le personnage peut porter");
		rule.SetDescription(
			"No loadout of a playable slot is heavier than the character can carry.",
			"Aucun loadout de slot jouable n'est plus lourd que ce que le personnage peut porter.");
		rule.SetProblem(
			"the loadout of slot %1 weighs about %2 kg, above the %3 kg the character can carry: some items may not be spawned.",
			"le loadout du slot %1 pèse environ %2 kg, au-delà des %3 kg que le personnage peut porter : des objets risquent de ne pas être créés.");
		rule.SetFix(
			"lighten the loadout (Loadout Editor).",
			"allégez le loadout (éditeur de loadouts).");

		rule = Declare("3.9", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Loadout items aimed at a missing storage", "Objets du loadout destinés à un rangement absent");
		rule.SetDescription(
			"No item of the loadout of a playable slot aims at a storage missing from its gear.",
			"Aucun objet du loadout d'un slot jouable ne vise un rangement absent de son équipement.");
		rule.SetProblem(
			"the slot %1 puts items in %2 storages that are not on its gear (a pouch of a former vest, for example): they will not be where expected.",
			"le slot %1 range des objets dans %2 rangements absents de son équipement (une pochette d'un ancien gilet, par exemple) : ils ne seront pas là où on les attend.");
		rule.SetFix(
			"open the slot prefab in the Loadout Editor and press Refresh Containers: it moves them to Items To Distribute.",
			"ouvrez le prefab du slot dans l'éditeur de loadouts et cliquez sur Refresh Containers : il les déplace dans Items To Distribute.");

		// ---- 4. System settings --------------------------------------------------------------------------
		rule = Declare("4.1", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW entity deleter without a filter", "Deleter TILW sans filtre");
		rule.SetDescription(
			"Every TILW entity deleter has a prefab filter.",
			"Chaque deleter TILW a un filtre de prefabs.");
		rule.SetProblem(
			"with an empty prefab filter, it deletes every root entity in its sphere, managers and AI included.",
			"avec un filtre de prefabs vide, il supprime toutes les entités racines de sa sphère, managers et IA compris.");
		rule.SetFix(
			"fill its prefab filter list, or check that only what must go is within its radius.",
			"remplissez sa liste de filtres de prefabs, ou vérifiez que seul ce qui doit disparaître est dans son rayon.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-20");
		rule.m_sKB = "tilw-entity-deleter";

		rule = Declare("4.2", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW vehicle crew that never spawns", "Équipage TILW de véhicule qui n'apparaît jamais");
		rule.SetDescription(
			"No vehicle relies on TILW_VehicleCrewComponent for its crew.",
			"Aucun véhicule ne compte sur TILW_VehicleCrewComponent pour son équipage.");
		rule.SetProblem(
			"TILW_VehicleCrewComponent does nothing with Reforger Lobby: TILW's lobby code is compiled out, so the vehicle stays empty.",
			"TILW_VehicleCrewComponent ne fait rien avec Reforger Lobby : le code lobby de TILW n'est pas compilé, le véhicule reste vide.");
		rule.SetFix(
			"spawn the vehicle and its crew with a TILW_PrefabSpawner (crew config, pregame spawn ticked).",
			"faites apparaître le véhicule et son équipage avec un TILW_PrefabSpawner (config d'équipage, apparition en pregame cochée).");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-10");
		rule.m_sKB = "tilw-vehicle-crew";

		rule = Declare("4.3", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("Settings different from the skeleton", "Réglages différents du squelette");
		rule.SetDescription(
			"The values the mission skeleton sets on its entities (game mode, AI world) are kept.",
			"Les valeurs que le squelette de mission pose sur ses entités (mode de jeu, AI world) sont conservées.");
		rule.SetProblem(
			"its settings differ from the mission skeleton: %1.",
			"ses réglages diffèrent du squelette de mission : %1.");
		rule.SetFix(
			"set the expected values back, or ignore this if the difference is wanted.",
			"remettez les valeurs attendues, ou ignorez ceci si l'écart est voulu.");

		rule = Declare("4.4", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Unknown faction key", "Clé de faction inconnue");
		rule.SetDescription(
			"Every faction key typed in the mission exists among the factions of the world.",
			"Chaque clé de faction saisie dans la mission existe parmi les factions du monde.");
		rule.SetProblem(
			"the faction %1 does not exist in the world.",
			"la faction %1 n'existe pas dans le monde.");
		rule.SetFix(
			"use the key of an existing faction (US, USSR, FIA...), as the faction manager declares it.",
			"utilisez la clé d'une faction existante (US, USSR, FIA…), telle que le gestionnaire de factions la déclare.");

		rule = Declare("4.5", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC teleport without a destination", "Téléportation GDC sans destination");
		rule.SetDescription(
			"Every teleport module has a destination that exists.",
			"Chaque module de téléportation a une destination qui existe.");
		rule.SetProblem(
			"the module cannot teleport: %1.",
			"le module ne peut pas téléporter : %1.");
		rule.SetFix(
			"set m_sDestinationName to the name of the destination entity.",
			"renseignez m_sDestinationName avec le nom de l'entité de destination.");

		rule = Declare("4.6", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC teleport module nobody can use", "Module de téléportation GDC inutilisable");
		rule.SetDescription(
			"A teleport module in ALL_PLAYERS mode asks for a rank a playable slot has.",
			"Un module de téléportation en mode ALL_PLAYERS exige un grade qu'un slot jouable possède.");
		rule.SetProblem(
			"in ALL_PLAYERS mode it requires the rank %1, and no playable slot reaches it (highest: %2).",
			"en mode ALL_PLAYERS il exige le grade %1, qu'aucun slot jouable n'atteint (le plus haut : %2).");
		rule.SetFix(
			"lower its minimum rank (m_eMinRankRequired, INVALID for none), or give that rank to the slot meant to use it.",
			"baissez son grade minimum (m_eMinRankRequired, INVALID pour aucun), ou donnez ce grade au slot qui doit l'utiliser.");

		rule = Declare("4.7", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC fire support nobody can call", "Appui feu GDC que personne ne peut demander");
		rule.SetDescription(
			"A support module can be called by some playable slot (rank and required items).",
			"Un module d'appui peut être demandé par un slot jouable (grade et objets requis).");
		rule.SetProblem(
			"no playable slot can call it: %1.",
			"aucun slot jouable ne peut le demander : %1.");
		rule.SetFix(
			"lower its minimum rank or change its required items, or give them to the slot meant to call it.",
			"baissez son grade minimum ou changez ses objets requis, ou donnez-les au slot qui doit le demander.");

		rule = Declare("4.8", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC fire support areas not found", "Zones d'appui feu GDC introuvables");
		rule.SetDescription(
			"The areas of a support module exist.",
			"Les zones d'un module d'appui existent.");
		rule.SetProblem(
			"its areas cannot apply: %1.",
			"ses zones ne peuvent pas s'appliquer : %1.");
		rule.SetFix(
			"give the names of closed polylines placed in the mission, or set the area mode to NONE.",
			"indiquez les noms de polylignes fermées posées dans la mission, ou mettez le mode de zone à NONE.");

		rule = Declare("4.9", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC fire support ammunition badly set", "Munitions d'appui feu GDC mal réglées");
		rule.SetDescription(
			"A fire support module has ammunition and a consistent dispersion.",
			"Un module d'appui feu a des munitions et une dispersion cohérente.");
		rule.SetProblem(
			"the fire support cannot fire as set: %1.",
			"l'appui feu ne peut pas tirer tel qu'il est réglé : %1.");
		rule.SetFix(
			"add ammunition (m_aAmmo) and keep the minimum dispersion below the maximum.",
			"ajoutez des munitions (m_aAmmo) et gardez la dispersion minimale sous la maximale.");

		rule = Declare("4.10", GDC_MMCheckOrigin.GDC, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.GDC);
		rule.SetTitle("GDC AI artillery badly set", "Artillerie IA GDC mal réglée");
		rule.SetDescription(
			"An AI artillery component has a mortar that exists and consistent fire orders.",
			"Un composant d'artillerie IA a un mortier qui existe et des ordres de tir cohérents.");
		rule.SetProblem(
			"the artillery cannot fire as set: %1.",
			"l'artillerie ne peut pas tirer telle qu'elle est réglée : %1.");
		rule.SetFix(
			"name the mortar (or its group) as in its Name field, add fire orders, and keep each inner radius below the outer one.",
			"nommez le mortier (ou son groupe) comme dans son champ Name, ajoutez des ordres de tir, et gardez chaque rayon intérieur sous l'extérieur.");

		// ---- 5. Mission logic ----------------------------------------------------------------------------
		rule = Declare("5.1", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW interaction placed as a child", "Interaction TILW placée en enfant");
		rule.SetDescription(
			"No TILW interaction is placed as a child of another entity.",
			"Aucune interaction TILW n'est placée en enfant d'une autre entité.");
		rule.SetProblem(
			"a TILW interaction under another entity may lose its action label on a dedicated server.",
			"une interaction TILW sous une autre entité peut perdre le libellé de son action sur un serveur dédié.");
		rule.SetFix(
			"place it as a root entity at the object's position, with \"Display UI At Reference Point\" ticked on its UserActionContext.",
			"placez-la en entité racine à la position de l'objet, avec « Display UI At Reference Point » coché sur son UserActionContext.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-20");
		rule.m_sKB = "tilw-interaction-child";

		rule = Declare("5.2", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW trigger without a flag", "Trigger TILW sans flag");
		rule.SetDescription(
			"Every TILW trigger sets a flag.",
			"Chaque trigger TILW lève un flag.");
		rule.SetProblem(
			"the trigger sets no flag (m_flagName empty): the mission logic cannot react to it.",
			"le trigger ne lève aucun flag (m_flagName vide) : la logique de mission ne peut pas y réagir.");
		rule.SetFix(
			"give it a flag name, or delete it.",
			"donnez-lui un nom de flag, ou supprimez-le.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-triggers";

		rule = Declare("5.3", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW faction trigger without an owner faction", "Trigger de faction TILW sans faction propriétaire");
		rule.SetDescription(
			"Every TILW faction trigger has an owner faction.",
			"Chaque trigger de faction TILW a une faction propriétaire.");
		rule.SetProblem(
			"no owner faction (m_ownerFactionKey): it can never be captured, and its status messages fail.",
			"aucune faction propriétaire (m_ownerFactionKey) : il ne peut jamais être capturé, et ses messages d'état échouent.");
		rule.SetFix(
			"set the key of the faction that captures it.",
			"renseignez la clé de la faction qui le capture.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-triggers";

		rule = Declare("5.4", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW trigger status messages badly filled", "Messages d'état de trigger TILW mal remplis");
		rule.SetDescription(
			"A TILW trigger that sends status messages has exactly 6 of them.",
			"Un trigger TILW qui envoie des messages d'état en a exactement 6.");
		rule.SetProblem(
			"status messages are on, but the list has %1 entries instead of 6: players see an error message instead.",
			"les messages d'état sont activés, mais la liste a %1 entrées au lieu de 6 : les joueurs voient un message d'erreur à la place.");
		rule.SetFix(
			"fill exactly 6 messages (an empty one is not sent), or untick m_sendStatusMessages.",
			"remplissez exactement 6 messages (un message vide n'est pas envoyé), ou décochez m_sendStatusMessages.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-triggers";

		rule = Declare("5.5", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW presence trigger without a valid target", "Trigger de présence TILW sans cible valable");
		rule.SetDescription(
			"Every TILW presence trigger looks for entities that exist.",
			"Chaque trigger de présence TILW cherche des entités qui existent.");
		rule.SetProblem(
			"the trigger cannot work: %1.",
			"le trigger ne peut pas fonctionner : %1.");
		rule.SetFix(
			"give the names of the entities to look for, as set in their Name field or by a spawner.",
			"indiquez les noms des entités à chercher, tels que saisis dans leur champ Name ou donnés par un spawner.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-triggers";

		rule = Declare("5.6", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW event or meta flag without a condition", "Événement ou meta flag TILW sans condition");
		rule.SetDescription(
			"Every TILW mission event has a condition, every meta flag a definition.",
			"Chaque événement de mission TILW a une condition, chaque meta flag une définition.");
		rule.SetProblem(
			"%1 has no condition: it never becomes true, and TILW logs a script error each time it evaluates it.",
			"%1 n'a pas de condition : il ne devient jamais vrai, et TILW écrit une erreur de script à chaque évaluation.");
		rule.SetFix(
			"add a condition (for an event only run by name, a literal on a flag nothing sets is enough), or delete it.",
			"ajoutez une condition (pour un événement lancé seulement par son nom, un littéral sur un flag que rien ne lève suffit), ou supprimez-le.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.7", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW condition operator without operand", "Opérateur de condition TILW sans opérande");
		rule.SetDescription(
			"Every operator of a TILW condition (Conjunction, Disjunction...) has operands.",
			"Chaque opérateur d'une condition TILW (Conjunction, Disjunction...) a des opérandes.");
		rule.SetProblem(
			"an operator of %1 (%2) has no operand: its result never changes (an empty Conjunction is always true, the event happens as soon as the game starts).",
			"un opérateur de %1 (%2) n'a pas d'opérande : son résultat ne change jamais (une Conjunction vide est toujours vraie, l'événement se produit dès le début de la partie).");
		rule.SetFix(
			"add its operands (literals or other operators), or remove the operator.",
			"ajoutez ses opérandes (littéraux ou autres opérateurs), ou retirez l'opérateur.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.8", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW flag read but never set", "Flag TILW lu que rien ne lève");
		rule.SetDescription(
			"Every flag a TILW condition waits for is set by something in the mission.",
			"Chaque flag qu'attend une condition TILW est levé par un élément de la mission.");
		rule.SetProblem(
			"%1 waits for the flag \"%2\", which nothing in the mission sets: it will never happen.",
			"%1 attend le flag « %2 », que rien dans la mission ne lève : cela ne se produira jamais.");
		rule.SetFix(
			"check the spelling against the trigger, interaction or instruction meant to set it. If a mission script sets it, ignore this warning.",
			"vérifiez l'orthographe par rapport au trigger, à l'interaction ou à l'instruction censés le lever. Si un script de la mission le lève, ignorez cet avertissement.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.9", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW instruction aimed at a missing event", "Instruction TILW visant un événement inexistant");
		rule.SetDescription(
			"Every event named by a Run, Reactivate or Deactivate Events instruction exists.",
			"Chaque événement nommé par une instruction Run, Reactivate ou Deactivate Events existe.");
		rule.SetProblem(
			"%1 names the event \"%2\", which no mission event holds: the instruction does nothing.",
			"%1 nomme l'événement « %2 », qu'aucun événement de mission ne porte : l'instruction ne fait rien.");
		rule.SetFix(
			"type the name exactly as in the event's m_name field.",
			"saisissez le nom exactement comme dans le champ m_name de l'événement.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.10", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW event name held by several events", "Nom d'événement TILW porté par plusieurs événements");
		rule.SetDescription(
			"An event name aimed at by an instruction belongs to one event only.",
			"Un nom d'événement visé par une instruction n'appartient qu'à un seul événement.");
		rule.SetProblem(
			"%2 events are named \"%1\": an instruction aimed at this name acts on all of them.",
			"%2 événements s'appellent « %1 » : une instruction qui vise ce nom agit sur tous.");
		rule.SetFix(
			"give each event its own name.",
			"donnez à chaque événement un nom qui lui est propre.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.11", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW instruction aimed at a missing entity", "Instruction TILW visant une entité inexistante");
		rule.SetDescription(
			"Every entity name a TILW instruction, spawner or vehicle crew looks for (place, group, waypoint, map item...) is held by an entity.",
			"Chaque nom d'entité que cherche une instruction, un spawner ou un équipage TILW (lieu, groupe, waypoint, élément de carte...) est porté par une entité.");
		rule.SetProblem(
			"%1 looks for names nobody holds: %2. Nothing happens for them.",
			"%1 cherche des noms que rien ne porte : %2. Rien ne se passe pour eux.");
		rule.SetFix(
			"give these names to the entities (Name field), or correct them. A name given when an entity spawns (m_setEntityName(s)) counts.",
			"donnez ces noms aux entités (champ Name), ou corrigez-les. Un nom donné à l'apparition d'une entité (m_setEntityName(s)) compte.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.12", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("Several TILW Mission Framework entities", "Plusieurs entités TILW Mission Framework");
		rule.SetDescription(
			"The mission holds a single TILW Mission Framework entity.",
			"La mission contient une seule entité TILW Mission Framework.");
		rule.SetProblem(
			"the mission has %1 TILW Mission Framework entities: TILW keeps only one, the events of the others are only evaluated when the game starts.",
			"la mission a %1 entités TILW Mission Framework : TILW n'en garde qu'une, les événements des autres ne sont évalués qu'au début de la partie.");
		rule.SetFix(
			"move their events and flags into one entity and delete the others.",
			"regroupez leurs événements et leurs flags dans une seule entité et supprimez les autres.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.13", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW pregame event reading non-random flags", "Événement TILW d'avant briefing lisant des flags non aléatoires");
		rule.SetDescription(
			"A TILW event allowed before the end of the briefing only reads random flags.",
			"Un événement TILW autorisé avant la fin du briefing ne lit que des flags aléatoires.");
		rule.SetProblem(
			"%1 can happen before the briefing has ended, but reads flags that are not random: %2. Most flags are not set yet at that time.",
			"%1 peut se produire avant la fin du briefing, mais lit des flags non aléatoires : %2. La plupart des flags ne sont pas encore levés à ce moment.");
		rule.SetFix(
			"untick m_pregameEvent, or base its condition on random flags (m_randomFlags).",
			"décochez m_pregameEvent, ou fondez sa condition sur des flags aléatoires (m_randomFlags).");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		rule = Declare("5.14", GDC_MMCheckOrigin.TILW, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.MODSET);
		rule.SetTitle("TILW game end suppressed (debug)", "Fin de partie TILW désactivée (debug)");
		rule.SetDescription(
			"The debug setting that suppresses the TILW game end is off when an event ends the game.",
			"Le réglage de debug qui désactive la fin de partie TILW est décoché quand un événement termine la partie.");
		rule.SetProblem(
			"the debug setting m_suppressGameEnd is ticked: the End Game instructions show a message instead of ending the game.",
			"le réglage de debug m_suppressGameEnd est coché : les instructions End Game affichent un message au lieu de terminer la partie.");
		rule.SetFix(
			"untick it before publishing.",
			"décochez-le avant la publication.");
		rule.SetVerified("TilWMissionFramework", "1.0.128", "2026-11-21");
		rule.m_sKB = "tilw-mission-events";

		// ---- 6. AI -----------------------------------------------------------------------------------------
		rule = Declare("6.1", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("AI group without a faction", "Groupe IA sans faction");
		rule.SetDescription(
			"Every AI group, placed or spawned during the mission, has a faction.",
			"Chaque groupe IA, posé ou apparu en cours de mission, a une faction.");
		rule.SetProblem(
			"%1 has no faction (m_faction): a group gives its faction to its members, so they belong to no side.",
			"%1 n'a pas de faction (m_faction) : un groupe donne sa faction à ses membres, qui n'appartiennent donc à aucun camp.");
		rule.SetFix(
			"set the group's faction key, or use a child prefab that sets it (the RHS group prefabs declare none).",
			"renseignez la clé de faction du groupe, ou utilisez un prefab enfant qui la fixe (les prefabs de groupe RHS n'en déclarent pas).");

		rule = Declare("6.2", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("AI waypoint not found", "Waypoint IA introuvable");
		rule.SetDescription(
			"Every waypoint or target named by a group, a cycle waypoint or an entity waypoint exists.",
			"Chaque waypoint ou cible nommé par un groupe, un waypoint Cycle ou un waypoint d'entité existe.");
		rule.SetProblem(
			"%1 names %2: no entity holds these names, or it is not a waypoint. The AI ignores them.",
			"%1 nomme %2 : aucune entité ne porte ces noms, ou ce n'est pas un waypoint. L'IA les ignore.");
		rule.SetFix(
			"type the names exactly as in the Name field of the waypoints (or of the target entity).",
			"saisissez les noms exactement comme dans le champ Name des waypoints (ou de l'entité visée).");

		rule = Declare("6.3", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.ERROR, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Group vehicle the AI cannot use", "Véhicule de groupe inutilisable par l'IA");
		rule.SetDescription(
			"Every vehicle a group lists (m_aStaticVehicles) exists and can be used by the AI.",
			"Chaque véhicule listé par un groupe (m_aStaticVehicles) existe et peut être utilisé par l'IA.");
		rule.SetProblem(
			"the group lists vehicles the AI cannot use: %1. No entity holds the name, or it has no SCR_AIVehicleUsageComponent.",
			"le groupe liste des véhicules que l'IA ne peut pas utiliser : %1. Aucune entité ne porte ce nom, ou elle n'a pas de SCR_AIVehicleUsageComponent.");
		rule.SetFix(
			"type the vehicle's name as in its Name field, and choose a vehicle or static weapon the AI can use.",
			"saisissez le nom du véhicule comme dans son champ Name, et choisissez un véhicule ou une arme fixe utilisable par l'IA.");

		rule = Declare("6.4", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("Waypoints nothing uses", "Waypoints que rien n'utilise");
		rule.SetDescription(
			"Every placed waypoint is named by a group, a cycle waypoint or another setting of the mission.",
			"Chaque waypoint posé est nommé par un groupe, un waypoint Cycle ou un autre réglage de la mission.");
		rule.SetProblem(
			"%1 waypoints are named by no group, cycle waypoint, instruction or setting of the mission: %2.",
			"%1 waypoints ne sont nommés par aucun groupe, waypoint Cycle, instruction ou réglage de la mission : %2.");
		rule.SetFix(
			"check the names typed where they should be used (a typo leaves a group standing still), or delete them.",
			"vérifiez les noms saisis là où ils devraient servir (une faute de frappe laisse un groupe immobile), ou supprimez-les.");

		rule = Declare("6.5", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("AI or waypoint above the void", "IA ou waypoint au-dessus du vide");
		rule.SetDescription(
			"Every AI group, AI character and waypoint stands on the ground or on an object.",
			"Chaque groupe IA, personnage IA et waypoint repose sur le sol ou sur un objet.");
		rule.SetProblem(
			"nothing (ground or object) lies within %1 m below it: placed too high, beside a floor, or under the terrain.",
			"rien (sol ou objet) ne se trouve à moins de %1 m sous lui : posé trop haut, à côté d'un plancher, ou sous le terrain.");
		rule.SetFix(
			"lower it onto the floor or the ground.",
			"descendez-le sur le plancher ou sur le sol.");
		rule.m_sKB = "engine-ai-placement";

		rule = Declare("6.6", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.INFO, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("AI budget", "Budget d'IA");
		rule.SetDescription(
			"The number of AI the mission places and spawns, against the active AI limit.",
			"Le nombre d'IA que la mission pose et fait apparaître, comparé à la limite d'IA actives.");
		rule.SetProblem(
			"%1 AI at start, up to %2 more spawned during the mission, for an active AI limit of %3 (vehicle crews not counted).",
			"%1 IA au départ, jusqu'à %2 de plus apparues en cours de mission, pour une limite d'IA actives de %3 (équipages de véhicules non comptés).");
		rule.SetFix(
			"check that the AI alive at the busiest moment stay below the limit.",
			"vérifiez que les IA en vie au moment le plus chargé restent sous la limite.");

		rule = Declare("6.7", GDC_MMCheckOrigin.VANILLA, GDC_EMMCheckSeverity.WARNING, GDC_EMMCheckFamily.ENGINE);
		rule.SetTitle("More AI at start than the active AI limit", "Plus d'IA au départ que la limite d'IA actives");
		rule.SetDescription(
			"The AI placed at start do not exceed the active AI limit of the AI world.",
			"Les IA posées au départ ne dépassent pas la limite d'IA actives du monde IA.");
		rule.SetProblem(
			"%1 AI at start for an active AI limit of %2: the AI beyond the limit may stay inactive.",
			"%1 IA au départ pour une limite d'IA actives de %2 : les IA au-delà de la limite peuvent rester inactives.");
		rule.SetFix(
			"raise the limit (SCR_AIWorld, Active AI limit) or place fewer AI at start.",
			"relevez la limite (SCR_AIWorld, Active AI limit) ou posez moins d'IA au départ.");

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
