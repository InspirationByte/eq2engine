//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Worker
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr const int WORK_NOT_STARTED = -10000;
static constexpr const int WORK_PENDING		= 10000;
static constexpr const int WORK_EXECUTING	= 20000;

class GLWorkerThread : public Threading::CEqThread
{
	friend class ShaderAPIGL;

public:
	using FUNC_TYPE = EqFunction<int()>;

	GLWorkerThread() = default;

	void	Init();
	void	Shutdown();

	// syncronous execution
	int		WaitForExecute(const char* name, FUNC_TYPE f);

	// asyncronous execution
	void	Execute(const char* name, FUNC_TYPE f);

protected:
	struct Work
	{
		FUNC_TYPE				func;
		int						result{ WORK_NOT_STARTED };
		bool					sync{ false };
	};

	int		Run() override;

	FixedArray<Work, 32>					m_workRingPool;
	FixedArray<Threading::CEqSignal, 32>	m_completionSignal;
	int						m_workCounter{ 0 };
};

extern GLWorkerThread g_glWorker;
