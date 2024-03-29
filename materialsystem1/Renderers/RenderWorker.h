//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Render worker thread
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr const int WORK_NOT_STARTED = -20000;
static constexpr const int WORK_TAKEN_SLOT = -10000;
static constexpr const int WORK_PENDING		= 10000;
static constexpr const int WORK_EXECUTING	= 20000;

class RenderWorkerHandler
{
public:
	virtual const char* GetAsyncThreadName() const { return "RenderAsyncWorker"; }
	virtual void		BeginAsyncOperation(uintptr_t threadId) = 0;
	virtual void		EndAsyncOperation() = 0;
	virtual bool		IsMainThread(uintptr_t threadId) const = 0;
};

class CRenderWorkThread : public Threading::CEqThread
{
public:
	using FUNC_TYPE = EqFunction<int()>;

	void		Init(RenderWorkerHandler* workHandler, int workPoolSize = 32);
	void		InitLoop(RenderWorkerHandler* workHandler, FUNC_TYPE loopFunc, int workPoolSize = 32);
	void		Shutdown();

	const char* GetLastWorkName() const { return m_lastWorkName; }

	bool		HasPendingWork() const;

	// syncronous execution
	int			WaitForExecute(const char* name, FUNC_TYPE f);

	// asyncronous execution
	void		Execute(const char* name, FUNC_TYPE f);

protected:
	int			Run() override;

	struct Work
	{
		FUNC_TYPE	func;
		EqString	name;
		int			result{ WORK_NOT_STARTED };
		uint		workIdx{ 0 };
		bool		sync{ false };
	};

	using SignalPool = FixedArray<Threading::CEqSignal, 96>;
	using WorkPool = FixedArray<Work, 96>;

	EqString				m_lastWorkName;

	FUNC_TYPE				m_loopFunc;
	WorkPool				m_workRingPool;
	SignalPool				m_completionSignal;
	RenderWorkerHandler*	m_workHandler{ nullptr };
	uint					m_workIdx{ 0 };
	uint					m_lastWorkIdx{ 0 };
};

extern CRenderWorkThread g_renderWorker;
