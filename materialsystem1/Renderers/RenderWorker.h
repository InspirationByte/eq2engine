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

	void		Init(RenderWorkerHandler* workHandler);
	void		Shutdown();

	// syncronous execution
	int			WaitForExecute(const char* name, FUNC_TYPE f);

	// asyncronous execution
	void		Execute(const char* name, FUNC_TYPE f);

protected:
	int			Run() override;

	struct Work
	{
		FUNC_TYPE	func;
		int			result{ WORK_NOT_STARTED };
		bool		sync{ false };
	};

	FixedArray<Work, 32>					m_workRingPool;
	FixedArray<Threading::CEqSignal, 32>	m_completionSignal;
	RenderWorkerHandler*					m_workHandler{ nullptr };
};

extern CRenderWorkThread g_renderWorker;