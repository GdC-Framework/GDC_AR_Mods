// GDC_FireMission.c
// Path in mod: Scripts/Game/GDC/Support/GDC_FireMission.c

//! Server-side virtual fire mission: no tube is involved, shells are spawned above the target and launched at it.
//! The ammo prefab must have a ProjectileMoveComponent, and a Runtime RplComponent so that clients see it
//! (vanilla *_EffectModule shells are made for this).
class GDC_FireMission : Managed
{
	//! Time left after the last launch for the shell to land before the mission is reported as finished.
	protected const int IMPACT_MARGIN_MS = 5000;

	protected ResourceName m_sAmmoPrefab;
	protected vector m_vCenter;
	protected int m_iRoundsLeft;
	protected float m_fDispersion;
	protected float m_fInterval;
	protected float m_fSpawnHeight;

	protected bool m_bFirstRoundFired;

	protected ref ScriptInvokerVoid m_OnFirstRound = new ScriptInvokerVoid();
	protected ref ScriptInvokerVoid m_OnFinished = new ScriptInvokerVoid();

	//------------------------------------------------------------------------------------------------
	//! \param ammoPrefab Shell prefab to launch
	//! \param center Target position, only X and Z are used
	//! \param rounds Number of shells to fire
	//! \param dispersion Radius of the impact disc in metres
	//! \param interval Delay between two shells in seconds
	//! \param spawnHeight Height above the ground at which shells are spawned, in metres
	void GDC_FireMission(ResourceName ammoPrefab, vector center, int rounds, float dispersion, float interval, float spawnHeight)
	{
		m_sAmmoPrefab = ammoPrefab;
		m_vCenter = center;
		m_iRoundsLeft = rounds;
		m_fDispersion = dispersion;
		m_fInterval = interval;
		m_fSpawnHeight = spawnHeight;
	}

	//------------------------------------------------------------------------------------------------
	//! Invoked once, when the first shell is launched.
	ScriptInvokerVoid GetOnFirstRound()
	{
		return m_OnFirstRound;
	}

	//------------------------------------------------------------------------------------------------
	//! Invoked once, after the last shell has had time to land.
	ScriptInvokerVoid GetOnFinished()
	{
		return m_OnFinished;
	}

	//------------------------------------------------------------------------------------------------
	//! Starts the mission. To be called by the authority of the support module only.
	//! \param delay Delay before the first shell is launched, in seconds
	void Start(float delay)
	{
		GetGame().GetCallqueue().CallLater(FireNextRound, delay * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Stops the mission: remaining shells are not fired and OnFinished is not invoked.
	void Cancel()
	{
		GetGame().GetCallqueue().Remove(FireNextRound);
		GetGame().GetCallqueue().Remove(Finish);
	}

	//------------------------------------------------------------------------------------------------
	protected void FireNextRound()
	{
		if (m_iRoundsLeft <= 0)
			return;

		vector target = GetRandomTarget();
		bool launched = LaunchShell(target);
		m_iRoundsLeft--;

		#ifdef DEBUG
		Print(string.Format("[GDC_FireMission] Round launched: %1, target %2, distance to center %3 m, dispersion %4 m, rounds left %5", launched, target, vector.DistanceXZ(target, m_vCenter), m_fDispersion, m_iRoundsLeft));
		#endif

		if (!m_bFirstRoundFired)
		{
			m_bFirstRoundFired = true;
			m_OnFirstRound.Invoke();
		}

		if (m_iRoundsLeft > 0)
			GetGame().GetCallqueue().CallLater(FireNextRound, m_fInterval * 1000, false);
		else
			GetGame().GetCallqueue().CallLater(Finish, IMPACT_MARGIN_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void Finish()
	{
		m_OnFinished.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! Returns a uniformly distributed ground position within the dispersion disc.
	protected vector GetRandomTarget()
	{
		float angle = Math.RandomFloat(0, Math.PI2);
		float radius = Math.Sqrt(Math.RandomFloat(0, m_fDispersion * m_fDispersion));

		vector target = m_vCenter + Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
		target[1] = GetGame().GetWorld().GetSurfaceY(target[0], target[2]);
		return target;
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns one shell above the mission center and launches it at the given position.
	//! \return false if the shell could not be spawned or launched
	protected bool LaunchShell(vector target)
	{
		Resource resource = Resource.Load(m_sAmmoPrefab);
		if (!resource.IsValid())
		{
			Print(string.Format("[GDC_FireMission] Invalid ammo prefab '%1'.", m_sAmmoPrefab), LogLevel.WARNING);
			return false;
		}

		BaseWorld world = GetGame().GetWorld();

		vector spawnPos = m_vCenter;
		spawnPos[1] = world.GetSurfaceY(spawnPos[0], spawnPos[2]) + m_fSpawnHeight;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = spawnPos;

		IEntity shell = GetGame().SpawnEntityPrefab(resource, world, params);
		if (!shell)
		{
			Print(string.Format("[GDC_FireMission] Failed to spawn '%1'.", m_sAmmoPrefab), LogLevel.WARNING);
			return false;
		}

		ProjectileMoveComponent moveComponent = ProjectileMoveComponent.Cast(shell.FindComponent(ProjectileMoveComponent));
		if (!moveComponent)
		{
			Print(string.Format("[GDC_FireMission] '%1' has no ProjectileMoveComponent.", m_sAmmoPrefab), LogLevel.WARNING);
			delete shell;
			return false;
		}

		vector transform[4];
		shell.GetTransform(transform);
		SCR_Math3D.LookAt(transform[3], target, vector.Up, transform);
		shell.SetTransform(transform);

		vector direction = vector.Direction(transform[3], target);
		direction.Normalize();

		moveComponent.Launch(direction, vector.Zero, 1, shell, null, null, null, null);
		return true;
	}
}
