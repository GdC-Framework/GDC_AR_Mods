#ifdef WORKBENCH
//! Does the navmesh answer in the World Editor (edit mode, no game running)? Rules 6.5 to 6.7 rely on it. Run from the
//! command line by Tools/MissionCheckTests/run_tests.py (-wbModule=WorldEditor -plugin=GDC_MMCheckNavmeshProbePlugin
//! -run -probeWorld=<world .ent>). Writes "[GDC_CHECK_PROBE] <marker>" lines.
[WorkbenchPluginAttribute(name: "GDC Check Mission Navmesh Probe", wbModules: { "WorldEditor" })]
class GDC_MMCheckNavmeshProbePlugin : WorkbenchPlugin
{
	protected static const string PREFIX = "[GDC_CHECK_PROBE] ";
	protected static const ref array<string> NAVMESH_NAMES = { "Soldiers", "BTRlike", "LowRes", "Vehicles", "Navmesh", "default" };

	//------------------------------------------------------------------------------------------------
	override void RunCommandline()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		string world;
		if (worldEditor)
			worldEditor.GetCmdLine("-probeWorld", world);

		if (!worldEditor || world.IsEmpty() || !worldEditor.SetOpenedResource(world))
		{
			Print(PREFIX + "OPEN_FAILED " + world, LogLevel.ERROR);
			Workbench.Exit(2);
			return;
		}

		WorldEditorAPI api = worldEditor.GetApi();
		Print(PREFIX + "START", LogLevel.NORMAL);
		AIWorld aiWorld = FindAIWorld(api);
		Print(PREFIX + "AIWORLD " + (aiWorld != null).ToString(), LogLevel.NORMAL);
		if (aiWorld)
			ProbeNavmesh(api, aiWorld);
		if (aiWorld)
			ProbeGroups(api, aiWorld);

		ProbeTraces(api);
		Print(PREFIX + "END", LogLevel.NORMAL);
		Workbench.Exit(0); // after the world is loaded: asked before, the exit crashes the world loading
	}

	//------------------------------------------------------------------------------------------------
	protected AIWorld FindAIWorld(WorldEditorAPI api)
	{
		for (int s = 0, subScenes = api.GetNumSubScenes(); s < subScenes; s++)
		{
			for (int i = 0, count = api.GetEntityCount(s); i < count; i++)
			{
				IEntitySource source = api.GetEntity(s, i);
				if (source && source.GetClassName().ToType() && source.GetClassName().ToType().IsInherited(AIWorld))
					return AIWorld.Cast(api.SourceToEntity(source));
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void ProbeNavmesh(WorldEditorAPI api, AIWorld aiWorld)
	{
		BaseWorld world = api.GetWorld();
		float ground = world.GetSurfaceY(2048, 2048);
		float sea = world.GetSurfaceY(150, 150);
		Print(PREFIX + string.Format("GROUND_Y %1 SEA_Y %2 OCEAN %3", ground, sea, world.GetOceanBaseHeight()), LogLevel.NORMAL);
		array<string> labels = { "ground", "air30", "under10", "sea" };
		array<vector> points = { Vector(2048, ground, 2048), Vector(2048, ground + 30, 2048), Vector(2048, ground - 10, 2048), Vector(150, sea, 150) };
		foreach (string name : NAVMESH_NAMES)
		{
			NavmeshWorldComponent navmesh = aiWorld.GetNavmeshWorldComponent(name);
			Print(PREFIX + "NAVMESH " + name + " " + (navmesh != null).ToString(), LogLevel.NORMAL);
			if (!navmesh)
				continue;

			foreach (int i, vector point : points)
			{
				vector reachable;
				bool near1 = navmesh.GetReachablePoint(point, 1, reachable);
				vector reachable5;
				bool near5 = navmesh.GetReachablePoint(point, 5, reachable5);
				Print(PREFIX + string.Format("POINT %1 %2 valid=%3 loaded=%4 reach1=%5 %6 reach5=%7 %8", name, labels[i], navmesh.IsTileValid(point),
					navmesh.IsTileLoaded(point), near1, reachable, near5, reachable5), LogLevel.NORMAL);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The navmesh under the first AI groups of the mission sub-scene
	protected void ProbeGroups(WorldEditorAPI api, AIWorld aiWorld)
	{
		NavmeshWorldComponent soldiers = aiWorld.GetNavmeshWorldComponent("Soldiers");
		NavmeshWorldComponent vehicles = aiWorld.GetNavmeshWorldComponent("BTRlike");
		int found;
		int subScene = api.GetCurrentSubScene();
		for (int i = 0, count = api.GetEntityCount(subScene); i < count && found < 5; i++)
		{
			IEntitySource source = api.GetEntity(subScene, i);
			if (!source || !source.GetClassName().ToType() || !source.GetClassName().ToType().IsInherited(SCR_AIGroup))
				continue;

			found++;
			IEntity group = api.SourceToEntity(source);
			vector origin = group.GetOrigin();
			vector reachable;
			string text = string.Format("GROUP %1 at %2:", source.GetName(), origin);
			if (soldiers)
				text += string.Format(" soldiers valid=%1 loaded=%2 requested=%3 reach5=%4 reach50=%5", soldiers.IsTileValid(origin), soldiers.IsTileLoaded(origin), soldiers.IsTileRequested(origin), soldiers.GetReachablePoint(origin, 5, reachable), soldiers.GetReachablePoint(origin, 50, reachable));
			if (vehicles)
				text += string.Format(" vehicles valid=%1 loaded=%2 requested=%3 reach5=%4 reach50=%5", vehicles.IsTileValid(origin), vehicles.IsTileLoaded(origin), vehicles.IsTileRequested(origin), vehicles.GetReachablePoint(origin, 5, reachable), vehicles.GetReachablePoint(origin, 50, reachable));
			Print(PREFIX + text, LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A trace down from just above the terrain, from 30 m above it, and from just above the roof of a terrain building
	protected void ProbeTraces(WorldEditorAPI api)
	{
		BaseWorld world = api.GetWorld();
		float ground = world.GetSurfaceY(2048, 2048);
		Print(PREFIX + "TRACE ground " + TraceDown(world, Vector(2048, ground + 0.5, 2048), 2).ToString(), LogLevel.NORMAL);
		Print(PREFIX + "TRACE air30 " + TraceDown(world, Vector(2048, ground + 30, 2048), 2).ToString(), LogLevel.NORMAL);

		for (int s = 0, subScenes = api.GetNumSubScenes(); s < subScenes; s++)
		{
			for (int i = 0, count = api.GetEntityCount(s); i < count; i++)
			{
				IEntitySource source = api.GetEntity(s, i);
				BaseContainer ancestor = source.GetAncestor();
				if (!ancestor || !ancestor.GetResourceName().Contains("/Houses/"))
					continue;

				IEntity house = api.SourceToEntity(source);
				if (!house)
					continue;

				vector mins, maxs;
				house.GetWorldBounds(mins, maxs);
				vector center = (mins + maxs) * 0.5;
				vector top = Vector(center[0], maxs[1] + 0.5, center[2]);
				float roofSurface = world.GetSurfaceY(center[0], center[2]);
				Print(PREFIX + string.Format("HOUSE %1 top=%2 terrain=%3 trace=%4 traceToGround=%5", ancestor.GetResourceName(), maxs[1], roofSurface,
					TraceDown(world, top, 3), TraceDown(world, top, maxs[1] - roofSurface + 1)), LogLevel.NORMAL);
				return;
			}
		}

		Print(PREFIX + "HOUSE none", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! \return fraction of the distance travelled before a hit (1 = nothing below within the distance)
	protected float TraceDown(BaseWorld world, vector start, float distance)
	{
		TraceParam trace = new TraceParam();
		trace.Start = start;
		trace.End = start - Vector(0, distance, 0);
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		return world.TraceMove(trace, null);
	}
}
#endif
