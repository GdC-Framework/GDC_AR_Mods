#ifdef WORKBENCH
//! Shared types of the Check Mission plugin: log, origins, findings, report, context given to each block, base block.

//------------------------------------------------------------------------------------------------
//! Language of every text the plugin builds (dialogs, report, rules): set from the plugin settings before each run.
//! The Workbench's own interface stays English.
enum GDC_EMMCheckLanguage
{
	ENGLISH,
	FRENCH
}

//------------------------------------------------------------------------------------------------
//! Both languages are written side by side where the text is used (catalogue, report): T() picks the current one.
class GDC_MMCheckText
{
	static GDC_EMMCheckLanguage s_eLanguage = GDC_EMMCheckLanguage.ENGLISH;

	//------------------------------------------------------------------------------------------------
	static string T(string english, string french)
	{
		if (s_eLanguage == GDC_EMMCheckLanguage.FRENCH && !french.IsEmpty())
			return french;

		return english;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "no error", "1 error", "2 errors" (French: "aucune erreur"...)
	static string Errors(int count)
	{
		return CountOf(count, T("no error", "aucune erreur"), T("error", "erreur"), T("errors", "erreurs"));
	}

	//------------------------------------------------------------------------------------------------
	static string Warnings(int count)
	{
		return CountOf(count, T("no warning", "aucun avertissement"), T("warning", "avertissement"), T("warnings", "avertissements"));
	}

	//------------------------------------------------------------------------------------------------
	static string Infos(int count)
	{
		return CountOf(count, T("no info", "aucune info"), "info", T("info", "infos"));
	}

	//------------------------------------------------------------------------------------------------
	protected static string CountOf(int count, string none, string singular, string plural)
	{
		if (count == 0)
			return none;

		if (count == 1)
			return "1 " + singular;

		return count.ToString() + " " + plural;
	}
}

//------------------------------------------------------------------------------------------------
//! Every line of the plugin starts with [GDC_CHECK]: filtering the Log Console with it keeps the whole report, laid out
//! for the mission maker (header, one section per block, each finding with Where / Problem / Fix, result).
class GDC_MMCheckLog
{
	static const string PREFIX = "[GDC_CHECK]";
	static const int WIDTH = 78; //!< width of the separator lines

	//------------------------------------------------------------------------------------------------
	//! Always prints an expression: Print() of a lone variable writes its declaration ("string x = '...'"), not its value.
	static void Write(string text = string.Empty, LogLevel level = LogLevel.NORMAL)
	{
		Print(PREFIX + " " + text, level);
	}

	//------------------------------------------------------------------------------------------------
	static void Separator()
	{
		Write(Repeat("=", WIDTH));
	}

	//------------------------------------------------------------------------------------------------
	//! "---- 2. Resources and addons ------------------------------ 1 error"
	static void Section(string title, string result)
	{
		string line = "---- " + title + " ";
		int fill = WIDTH - CharCount(line) - CharCount(result) - 1;
		if (fill < 4)
			fill = 4;

		Write(line + Repeat("-", fill) + " " + result);
	}

	//------------------------------------------------------------------------------------------------
	static string Repeat(string text, int count)
	{
		string repeated;
		for (int i = 0; i < count; i++)
		{
			repeated += text;
		}

		return repeated;
	}

	//------------------------------------------------------------------------------------------------
	static string Capitalize(string text)
	{
		if (text.IsEmpty())
			return text;

		string first = text.Substring(0, 1);
		first.ToUpper();
		return first + text.Substring(1, text.Length() - 1);
	}

	//------------------------------------------------------------------------------------------------
	//! string.ToUpper() only handles ASCII: the French accented letters are added.
	static string Upper(string text)
	{
		string upper = text;
		upper.ToUpper(); // in place
		upper.Replace("é", "É");
		upper.Replace("è", "È");
		upper.Replace("ê", "Ê");
		upper.Replace("à", "À");
		upper.Replace("ç", "Ç");
		return upper;
	}

	//------------------------------------------------------------------------------------------------
	//! \return the text followed by spaces up to the given length
	static string Pad(string text, int length)
	{
		return text + Repeat(" ", length - CharCount(text));
	}

	//------------------------------------------------------------------------------------------------
	//! string.Length() counts UTF-8 bytes ("é" = 2): this counts characters, for alignment.
	static int CharCount(string text)
	{
		int count;
		for (int i = 0, length = text.Length(); i < length; i++)
		{
			int code = text.ToAscii(i);
			if (code < 0)
				code += 256;

			if (code < 0x80 || code >= 0xC0) // not a UTF-8 continuation byte
				count++;
		}

		return count;
	}
}

//------------------------------------------------------------------------------------------------
//! Origin of a rule: vanilla, GDC or a third-party framework (TILW). Rules of an optional origin only run when the
//! mission uses it (one of its entities or components is placed): an addon merely loaded does not count.
class GDC_MMCheckOrigin
{
	static const string VANILLA = "vanilla";
	static const string GDC = "GDC";
	static const string TILW = "TILW";
	static const string RL = "RL"; //!< Reforger Lobby (and its PSCore)

	//------------------------------------------------------------------------------------------------
	//! \return optional origins, recognised from class names, in their display order (vanilla is always used)
	static array<string> GetOptional()
	{
		return { GDC, RL, TILW };
	}

	//------------------------------------------------------------------------------------------------
	//! \return class name prefixes of an origin (PK_ classes ship with TILW, PS_ ones with Reforger Lobby and PSCore)
	static array<string> GetClassPrefixes(string origin)
	{
		if (origin == GDC)
			return { "GDC_" };

		if (origin == TILW)
			return { "TILW_", "PK_" };

		if (origin == RL)
			return { "PS_" };

		return {};
	}
}

//------------------------------------------------------------------------------------------------
enum GDC_EMMCheckSeverity
{
	ERROR,		//!< breaks the game: fix it before playing
	WARNING,	//!< may be wanted, the mission maker decides
	INFO		//!< for the record
}

//------------------------------------------------------------------------------------------------
//! Where the knowledge behind a rule comes from, hence how long it holds.
enum GDC_EMMCheckFamily
{
	ENGINE,		//!< engine or Workbench fact: holds until the engine changes
	GDC,		//!< GDC convention: holds until GDC decides otherwise
	MODSET		//!< behaviour (or bug) of a third-party addon: to verify again when that addon is updated
}

//------------------------------------------------------------------------------------------------
//! One finding, written for the mission maker: what (title), where, what goes wrong (problem), what to do (fix).
class GDC_MMCheckFinding
{
	GDC_EMMCheckSeverity m_eSeverity;
	string m_sRule;			//!< number of the rule, e.g. "5.1"
	string m_sOrigin;	//!< GDC_MMCheckOrigin value
	string m_sTitle;		//!< short name of the rule, e.g. "Entity deleter without a filter"
	string m_sWhere;		//!< layer / entity, property
	string m_sLink;			//!< @"ENTITY:<id>": the Log Console selects the entity when it is clicked (may be empty)
	string m_sProblem;		//!< what goes wrong, one sentence
	string m_sFix;			//!< what to do, one sentence (may be empty)
	string m_sSheet;		//!< address of the rule's knowledge base sheet (may be empty)

	//------------------------------------------------------------------------------------------------
	//! Writes the finding as a block of lines, all at the log level of its severity (trailing blank line included): the
	//! Log Console level filters then keep whole findings.
	void Write()
	{
		LogLevel level = GDC_MMCheckReport.SeverityLogLevel(m_eSeverity);
		GDC_MMCheckLog.Write(string.Format("  %1 %2 [%3] %4", GDC_MMCheckReport.SeverityLabel(m_eSeverity), m_sRule, m_sOrigin, m_sTitle), level);

		string whereLabel = GDC_MMCheckText.T("Where", "Où");
		string problemLabel = GDC_MMCheckText.T("Problem", "Problème");
		string fixLabel = GDC_MMCheckText.T("Fix", "Correction");
		string sheetLabel = GDC_MMCheckText.T("Sheet", "Fiche");
		int width = Math.Max(Math.Max(GDC_MMCheckLog.CharCount(whereLabel), GDC_MMCheckLog.CharCount(problemLabel)),
			Math.Max(GDC_MMCheckLog.CharCount(fixLabel), GDC_MMCheckLog.CharCount(sheetLabel)));
		string colon = " : "; // log column layout, both languages

		// readable place and clickable link on one line: the link alone is an opaque id, stale once the world is reloaded
		string where = m_sWhere;
		if (!m_sLink.IsEmpty())
			where += " " + m_sLink;

		where.TrimInPlace();
		if (!where.IsEmpty())
			GDC_MMCheckLog.Write("      " + GDC_MMCheckLog.Pad(whereLabel, width) + colon + where, level);

		GDC_MMCheckLog.Write("      " + GDC_MMCheckLog.Pad(problemLabel, width) + colon + GDC_MMCheckLog.Capitalize(m_sProblem), level);
		if (!m_sFix.IsEmpty())
			GDC_MMCheckLog.Write("      " + GDC_MMCheckLog.Pad(fixLabel, width) + colon + GDC_MMCheckLog.Capitalize(m_sFix), level);

		// plain text, to copy: the Log Console opens @"..." as a Workbench resource, a web address gives an error
		if (!m_sSheet.IsEmpty())
			GDC_MMCheckLog.Write("      " + GDC_MMCheckLog.Pad(sheetLabel, width) + colon + m_sSheet, level);

		GDC_MMCheckLog.Write(string.Empty, level);
	}
}

//------------------------------------------------------------------------------------------------
class GDC_MMCheckReport
{
	ref array<ref GDC_MMCheckFinding> m_aFindings = {};
	protected GDC_MMCheckContext m_Context;

	//------------------------------------------------------------------------------------------------
	void GDC_MMCheckReport(GDC_MMCheckContext context)
	{
		m_Context = context;
	}

	//------------------------------------------------------------------------------------------------
	//! Adds a finding of a catalogue rule (GDC_MMCheckCatalog), which gives its severity, origin, title, problem and
	//! fix, in the current language.
	//! \param ruleId e.g. "2.1"
	//! \param entity the entity concerned, null for a finding about the whole mission
	//! \param property the property concerned, appended to the entity's description (may be empty)
	//! \param arg1 replaces %1 in the rule's problem and fix (e.g. the resource), arg2 replaces %2, arg3 %3
	void Add(string ruleId, IEntitySource entity, string property = string.Empty, string arg1 = string.Empty, string arg2 = string.Empty, string arg3 = string.Empty)
	{
		GDC_MMCheckRule rule = GDC_MMCheckCatalog.Get(ruleId);
		if (!rule)
		{
			GDC_MMCheckLog.Write("Internal error: rule " + ruleId + " is not in the catalogue.", LogLevel.ERROR);
			return;
		}

		GDC_MMCheckFinding finding = new GDC_MMCheckFinding();
		finding.m_eSeverity = rule.m_eSeverity;
		finding.m_sRule = rule.m_sId;
		finding.m_sOrigin = rule.m_sOrigin;
		finding.m_sTitle = rule.GetTitle();
		finding.m_sProblem = string.Format(rule.GetProblem(), arg1, arg2, arg3);
		finding.m_sFix = string.Format(rule.GetFix(), arg1, arg2, arg3);
		finding.m_sSheet = GDC_MMCheckCatalog.GetSheetUrl(rule.m_sKB);
		if (entity)
		{
			finding.m_sWhere = m_Context.Describe(entity);
			finding.m_sLink = m_Context.GetEntityLink(entity);
		}

		if (!property.IsEmpty())
		{
			string detail = GDC_MMCheckText.T("property ", "propriété ") + property;
			if (finding.m_sWhere.IsEmpty())
				finding.m_sWhere = detail;
			else
				finding.m_sWhere += ", " + detail;
		}

		m_aFindings.Insert(finding);
	}

	//------------------------------------------------------------------------------------------------
	int Count(GDC_EMMCheckSeverity severity)
	{
		int count;
		foreach (GDC_MMCheckFinding finding : m_aFindings)
		{
			if (finding.m_eSeverity == severity)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! \return ERROR -> LogLevel.ERROR, WARNING -> LogLevel.WARNING, INFO -> LogLevel.NORMAL
	static LogLevel SeverityLogLevel(GDC_EMMCheckSeverity severity)
	{
		switch (severity)
		{
			case GDC_EMMCheckSeverity.ERROR:
				return LogLevel.ERROR;
			case GDC_EMMCheckSeverity.WARNING:
				return LogLevel.WARNING;
		}

		return LogLevel.NORMAL;
	}

	//------------------------------------------------------------------------------------------------
	static string SeverityLabel(GDC_EMMCheckSeverity severity)
	{
		switch (severity)
		{
			case GDC_EMMCheckSeverity.ERROR:
				return GDC_MMCheckText.T("ERROR", "ERREUR");
			case GDC_EMMCheckSeverity.WARNING:
				return GDC_MMCheckText.T("WARNING", "AVERTISSEMENT");
		}

		return "INFO";
	}
}

//------------------------------------------------------------------------------------------------
//! What the blocks work on: the entities of the mission sub-scene (the terrain's are left out), read once per run.
class GDC_MMCheckContext
{
	WorldEditorAPI m_Api;
	int m_iSubScene;
	string m_sWorldPath;
	ResourceName m_SkeletonConfig;	//!< mission skeleton block 1 compares the mission with (plugin settings)
	ref array<IEntitySource> m_aEntities = {};
	ref set<string> m_UsedOrigins = new set<string>(); //!< GDC_MMCheckOrigin values used by the mission

	protected ref map<string, int> m_mResourceStates = new map<string, int>();

	static const int RESOURCE_OK = 0;
	static const int RESOURCE_MISSING = 1;

	//------------------------------------------------------------------------------------------------
	void GDC_MMCheckContext(WorldEditorAPI api)
	{
		m_Api = api;
		m_iSubScene = api.GetCurrentSubScene();
		api.GetWorldPath(m_sWorldPath);

		for (int i = 0, count = api.GetEntityCount(m_iSubScene); i < count; i++)
		{
			IEntitySource source = api.GetEntity(m_iSubScene, i);
			if (source)
				m_aEntities.Insert(source);
		}

		FindUsedOrigins();
	}

	//------------------------------------------------------------------------------------------------
	bool Uses(string origin)
	{
		return m_UsedOrigins.Contains(origin);
	}

	//------------------------------------------------------------------------------------------------
	//! An origin is used when an entity or a component of the mission has one of its class prefixes.
	protected void FindUsedOrigins()
	{
		m_UsedOrigins.Insert(GDC_MMCheckOrigin.VANILLA);
		foreach (string origin : GDC_MMCheckOrigin.GetOptional())
		{
			array<string> prefixes = GDC_MMCheckOrigin.GetClassPrefixes(origin);
			foreach (IEntitySource entity : m_aEntities)
			{
				if (HasPrefix(entity.GetClassName(), prefixes))
				{
					m_UsedOrigins.Insert(origin);
					break;
				}

				bool found;
				for (int i = 0, count = entity.GetComponentCount(); i < count; i++)
				{
					if (HasPrefix(entity.GetComponent(i).GetClassName(), prefixes))
					{
						found = true;
						break;
					}
				}

				if (found)
				{
					m_UsedOrigins.Insert(origin);
					break;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasPrefix(string className, notnull array<string> prefixes)
	{
		foreach (string prefix : prefixes)
		{
			if (className.StartsWith(prefix))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "layer/entity", the entity being its name, else its prefab (or class) and "(x, z)"; a child entity
	//! (position relative to its parent) is followed by ", child of <parent>"
	string Describe(IEntitySource entity)
	{
		string name = DescribeEntity(entity);
		BaseContainer parent = entity.GetParent();
		if (parent && parent.ToEntitySource())
			name += GDC_MMCheckText.T(", child of ", ", enfant de ") + DescribeEntity(parent.ToEntitySource());

		string layer = m_Api.GetEntitySubsceneLayer(m_iSubScene, entity);
		if (layer.IsEmpty())
			return name;

		return layer + "/" + name;
	}

	//------------------------------------------------------------------------------------------------
	//! The Log Console turns @"ENTITY:<id>" into a link selecting the entity, as in the engine's own messages. The id is
	//! taken from the standard text of the editor's entity: "Class<0x...> @"ENTITY:<id>" ('class','model') at <...>".
	//! \return @"ENTITY:<id>", empty when the entity has no instance in the editor
	string GetEntityLink(IEntitySource source)
	{
		IEntity entity = m_Api.SourceToEntity(source);
		if (!entity)
			return string.Empty;

		string text = string.Format("%1", entity);
		int start = text.IndexOf("@\"ENTITY:");
		if (start < 0)
			return string.Empty;

		int end = text.IndexOfFrom(start + 2, "\"");
		if (end < 0)
			return string.Empty;

		return text.Substring(start, end - start + 1);
	}

	//------------------------------------------------------------------------------------------------
	//! \return the name shown in the World Editor hierarchy; an entity without a name of its own is followed by "(x, z)"
	protected string DescribeEntity(IEntitySource entity)
	{
		string name = entity.GetName();
		if (!name.IsEmpty())
			return name;

		name = m_Api.GetEntityNiceName(entity);
		if (name.IsEmpty())
			name = entity.GetClassName();

		if (entity.GetParent())
			return name;

		vector coords;
		entity.Get("coords", coords);
		return name + string.Format(" (%1, %2)", Math.Round(coords[0]), Math.Round(coords[2]));
	}

	//------------------------------------------------------------------------------------------------
	//! Whether a referenced resource exists in the loaded addons, by its GUID (its stored path may be outdated).
	//! \return RESOURCE_OK or RESOURCE_MISSING
	int GetResourceState(ResourceName resource)
	{
		int state;
		if (m_mResourceStates.Find(resource, state))
			return state;

		state = ResolveResourceState(resource);
		m_mResourceStates.Insert(resource, state);
		return state;
	}

	//------------------------------------------------------------------------------------------------
	//! GetResourceState without its cache.
	static int ResolveResourceState(ResourceName resource)
	{
		int state;
		// resource database lookup (packed files included, nothing loaded). GetPath() resolves a known GUID to the file's
		// current path (a renamed file stays found); GetResourceName() of a path without any file returns the path
		// itself, without GUID. Resource.Load is no test: it falls back on the path when the GUID is unknown
		// (verified by Tools/MissionCheckTests).
		string registeredGuid = GetGUID(Workbench.GetResourceName(resource.GetPath()));
		// an unknown GUID whose path exists is repaired from the path by the Workbench when it opens the world
		if (registeredGuid.IsEmpty())
			state = RESOURCE_MISSING;
		else
			state = RESOURCE_OK;

		return state;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "{GUID}" part of a resource name, empty if it has none
	static string GetGUID(string resource)
	{
		if (!resource.StartsWith("{"))
			return string.Empty;

		int end = resource.IndexOf("}");
		if (end < 0)
			return string.Empty;

		return resource.Substring(0, end + 1);
	}

	//------------------------------------------------------------------------------------------------
	//! \return whether the class named className inherits the class named baseName; false when either is not loaded
	static bool Inherits(string className, string baseName)
	{
		typename base = baseName.ToType();
		if (!base)
			return false;

		typename type = className.ToType();
		return type && type.IsInherited(base);
	}
}

//------------------------------------------------------------------------------------------------
//! One block of checks, run or skipped as a whole from the plugin settings. A block not written yet only shows its title.
class GDC_MMCheckBlock
{
	protected int m_iNumber;
	protected string m_sTitle;

	//------------------------------------------------------------------------------------------------
	void Setup(int number, string title)
	{
		m_iNumber = number;
		m_sTitle = title;
	}

	//------------------------------------------------------------------------------------------------
	int GetNumber()
	{
		return m_iNumber;
	}

	//------------------------------------------------------------------------------------------------
	string GetTitle()
	{
		return m_sTitle;
	}

	//------------------------------------------------------------------------------------------------
	bool IsImplemented()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void Run(GDC_MMCheckContext context, GDC_MMCheckReport report);
}
#endif
