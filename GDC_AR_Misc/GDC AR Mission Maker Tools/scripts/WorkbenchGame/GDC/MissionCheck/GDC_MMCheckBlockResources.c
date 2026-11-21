#ifdef WORKBENCH
//! Block 2, resources and addons (rules 2.1 to 2.4, described in GDC_MMCheckCatalog):
//! - 2.1 / 2.4 every resource the mission references exists in the loaded addons, looked up by GUID (2.4: GUID unknown
//!   but a file exists at its path);
//! - 2.2 no MM library proxy (PrefabLibrary/MM ...) used as a prefab or in a property;
//! - 2.3 no AI group, character or vehicle without a prefab.
//! Only the references the mission adds are checked: a reference its entity's prefab already holds belongs to the
//! prefab's addon.
class GDC_MMCheckBlockResources : GDC_MMCheckBlock
{
	protected static const string MM_LIBRARY_PREFIX = "prefablibrary/mm ";
	protected static const ref array<string> PREFAB_ONLY_CLASSES = { "SCR_AIGroup", "SCR_ChimeraCharacter", "Vehicle" };

	protected GDC_MMCheckContext m_Context;
	protected GDC_MMCheckReport m_Report;
	protected ref map<string, ref set<string>> m_mPrefabResources; //!< prefab -> every resource it references

	//------------------------------------------------------------------------------------------------
	override bool IsImplemented()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void Run(GDC_MMCheckContext context, GDC_MMCheckReport report)
	{
		m_Context = context;
		m_Report = report;
		m_mPrefabResources = new map<string, ref set<string>>();

		foreach (IEntitySource entity : context.m_aEntities)
		{
			ResourceName prefab = SCR_BaseContainerTools.GetPrefabResourceName(entity);
			if (prefab.IsEmpty())
				CheckPrefabOnlyClass(entity);
			else if (IsLibraryProxy(prefab))
				report.Add("2.2", entity, string.Empty, prefab.GetPath());

			set<string> inherited = GetPrefabResources(entity.GetAncestor());
			map<string, string> references = new map<string, string>(); // resource -> first property holding it
			CollectEntity(entity, references);
			foreach (string resource, string property : references)
			{
				if (!inherited.Contains(resource))
					CheckResource(resource, entity, property);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckPrefabOnlyClass(IEntitySource entity)
	{
		foreach (string className : PREFAB_ONLY_CLASSES)
		{
			if (GDC_MMCheckContext.Inherits(entity.GetClassName(), className))
			{
				m_Report.Add("2.3", entity, string.Empty, entity.GetClassName());
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return every resource the prefab references (its ancestors' included), read once per run; empty without a prefab
	protected set<string> GetPrefabResources(BaseContainer prefab)
	{
		if (!prefab)
			return new set<string>();

		string key = prefab.GetResourceName();
		set<string> resources = m_mPrefabResources.Get(key);
		if (resources)
			return resources;

		map<string, string> references = new map<string, string>();
		CollectEntity(prefab, references);
		resources = new set<string>();
		foreach (string resource, string property : references)
		{
			resources.Insert(resource);
		}

		m_mPrefabResources.Insert(key, resources);
		return resources;
	}

	//------------------------------------------------------------------------------------------------
	//! Every resource referenced by the entity (inherited values included) and its components.
	protected static void CollectEntity(BaseContainer entity, notnull map<string, string> references)
	{
		Collect(entity, string.Empty, true, references);

		IEntitySource source = entity.ToEntitySource();
		if (!source)
			return;

		for (int i = 0, count = source.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource component = source.GetComponent(i);
			Collect(component, component.GetClassName(), false, references);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Walks the values of a container and of the objects it holds.
	//! \param isEntity the entity itself: its "components" array is skipped, CollectEntity walks the components
	protected static void Collect(BaseContainer container, string path, bool isEntity, notnull map<string, string> references)
	{
		for (int i = 0, count = container.GetNumVars(); i < count; i++)
		{
			string name = container.GetVarName(i);
			string property = name;
			if (!path.IsEmpty())
				property = path + "." + name;

			DataVarType type = container.GetDataVarType(i);
			if (type == DataVarType.RESOURCE_NAME)
			{
				ResourceName single;
				container.Get(name, single);
				AddReference(references, single, property);
			}
			else if (type == DataVarType.RESOURCE_NAME_ARRAY)
			{
				array<ResourceName> resources = {};
				container.Get(name, resources);
				foreach (int r, ResourceName listed : resources)
				{
					AddReference(references, listed, property + "[" + r.ToString() + "]");
				}
			}
			else if (type == DataVarType.OBJECT)
			{
				BaseContainer child = container.GetObject(name);
				if (child)
					Collect(child, property, false, references);
			}
			else if (type == DataVarType.OBJECT_ARRAY && !(isEntity && name == "components"))
			{
				BaseContainerList children = container.GetObjectArray(name);
				if (children)
				{
					for (int c = 0, childCount = children.Count(); c < childCount; c++)
					{
						BaseContainer element = children.Get(c);
						if (element)
							Collect(element, property + "[" + c.ToString() + "]", false, references);
					}
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddReference(notnull map<string, string> references, ResourceName resource, string property)
	{
		if (!resource.IsEmpty() && !references.Contains(resource))
			references.Insert(resource, property);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckResource(ResourceName resource, IEntitySource entity, string property)
	{
		if (IsLibraryProxy(resource))
		{
			m_Report.Add("2.2", entity, property, resource.GetPath());
			return;
		}

		switch (m_Context.GetResourceState(resource))
		{
			case GDC_MMCheckContext.RESOURCE_MISSING:
				m_Report.Add("2.1", entity, property, resource);
				break;

			case GDC_MMCheckContext.RESOURCE_PATH_ONLY:
				m_Report.Add("2.4", entity, property, resource);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsLibraryProxy(ResourceName resource)
	{
		string path = resource.GetPath();
		path.Replace("\\", "/");
		path.ToLower();
		return path.StartsWith(MM_LIBRARY_PREFIX);
	}
}
#endif
