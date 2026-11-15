// GDC_MissionTestRunnerEntity.c
// Path in mod: Scripts/Game/GDC/MissionTests/GDC_MissionTestRunnerEntity.c

[EntityEditorProps(category: "GDC/Tests", description: "Spawned by the automated mission tests on the dedicated server. Do not place it.")]
class GDC_MissionTestRunnerEntityClass : GenericEntityClass
{
}

//! Holds the running scenario and measures the frame rate of the dedicated server, in windows of 10 real seconds.
class GDC_MissionTestRunnerEntity : GenericEntity
{
	protected const int FPS_WINDOW_MS = 10000;

	protected ref GDC_MissionTest m_Test;

	protected int m_iFrames;
	protected int m_iWindowStart;
	protected float m_fLastFps;
	protected float m_fMinFps = 1000;
	protected float m_fFpsSum;
	protected int m_iFpsWindows;

	//------------------------------------------------------------------------------------------------
	void GDC_MissionTestRunnerEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.FRAME);
		m_iWindowStart = System.GetTickCount();
	}

	//------------------------------------------------------------------------------------------------
	void Run(notnull GDC_MissionTest test)
	{
		m_Test = test;
		test.Start(this);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		m_iFrames++;
		int elapsed = System.GetTickCount() - m_iWindowStart;
		if (elapsed < FPS_WINDOW_MS)
			return;

		m_fLastFps = m_iFrames * 1000.0 / elapsed;
		m_fMinFps = Math.Min(m_fMinFps, m_fLastFps);
		m_fFpsSum += m_fLastFps;
		m_iFpsWindows++;
		m_iFrames = 0;
		m_iWindowStart = System.GetTickCount();
	}

	//------------------------------------------------------------------------------------------------
	float GetLastFps()
	{
		return m_fLastFps;
	}

	//------------------------------------------------------------------------------------------------
	//! Minimum since the last reset, so a phase of the scenario can be measured alone.
	float GetMinFps()
	{
		return m_fMinFps;
	}

	//------------------------------------------------------------------------------------------------
	float GetAverageFps()
	{
		if (m_iFpsWindows == 0)
			return 0;

		return m_fFpsSum / m_iFpsWindows;
	}

	//------------------------------------------------------------------------------------------------
	void ResetFpsStats()
	{
		m_fMinFps = 1000;
		m_fFpsSum = 0;
		m_iFpsWindows = 0;
	}

	//------------------------------------------------------------------------------------------------
	string GetFpsReport()
	{
		return string.Format("Server FPS: average %1, minimum %2 (windows of %3 s)", Math.Round(GetAverageFps()), Math.Round(m_fMinFps), FPS_WINDOW_MS / 1000);
	}
}
