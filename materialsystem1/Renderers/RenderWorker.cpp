//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Render worker thread
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RenderWorker.h"

using namespace Threading;

CRenderWorkThread g_renderWorker;

void CRenderWorkThread::Init(RenderWorkerHandler* workHandler, int workPoolSize)
{
	m_workHandler = workHandler;
	StartWorkerThread(m_workHandler->GetAsyncThreadName(), TP_ABOVE_NORMAL, 3072 * 1024);

	const int poolSize = min(workPoolSize, m_workRingPool.numAllocated());
	m_workRingPool.setNum(poolSize);

	// by default every work in pool is free (see .Wait call after getting one from ring pool)
	for (int i = 0; i < m_workRingPool.numElem(); ++i)
	{
		m_completionSignal.appendEmplace(true);
		m_completionSignal[i].Raise();
	}
}

void CRenderWorkThread::InitLoop(RenderWorkerHandler* workHandler, FUNC_TYPE loopFunc, int workPoolSize)
{
	m_loopFunc = loopFunc;
	m_workHandler = workHandler;
	StartWorkerThread(m_workHandler->GetAsyncThreadName(), TP_ABOVE_NORMAL, 3072 * 1024);

	const int poolSize = min(workPoolSize, m_workRingPool.numAllocated());
	m_workRingPool.setNum(poolSize);

	// by default every work in pool is free (see .Wait call after getting one from ring pool)
	for (int i = 0; i < m_workRingPool.numElem(); ++i)
	{
		m_completionSignal.appendEmplace(true);
		m_completionSignal[i].Raise();
	}
}

void CRenderWorkThread::Shutdown()
{
	SignalWork();
	StopThread();

	m_workRingPool.clear(true);
}

int CRenderWorkThread::WaitForExecute(const char* name, FUNC_TYPE f)
{
	const uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (m_workHandler->IsMainThread(thisThreadId)) // not required for main thread
	{
		const int res = f();
		if (m_loopFunc)
			m_loopFunc();
		return res;
	}

	// chose free slot
	Work* work = nullptr;
	CEqSignal* completionSignal = nullptr;
	do {
		for(int slot = 0; slot < m_workRingPool.numElem(); ++slot)
		{
			if(m_completionSignal[slot].Wait(0) && Atomic::CompareExchange(m_workRingPool[slot].result, WORK_NOT_STARTED, WORK_TAKEN_SLOT) == WORK_NOT_STARTED)
			{
				work = &m_workRingPool[slot];
				completionSignal = &m_completionSignal[slot];
				completionSignal->Clear();
				break;
			}
		}
		Threading::YieldCurrentThread();
	} while(!work);

	work->workIdx = Atomic::Increment(m_workIdx);
	work->name = name;
	work->func = std::move(f);
	work->sync = true;
	Atomic::Exchange(work->result, WORK_PENDING);

	SignalWork();

	const bool isSignalled = completionSignal->Wait();
	const int workResult = Atomic::Exchange(work->result, WORK_NOT_STARTED);

	ASSERT_MSG(isSignalled && workResult != WORK_PENDING, "Failed to wait for render worker task - WORK_PENDING");
	ASSERT_MSG(isSignalled && workResult != WORK_EXECUTING, "Failed to wait for render worker task - WORK_EXECUTING");
	ASSERT_MSG(isSignalled && workResult != WORK_NOT_STARTED, "Empty slot was working on render task - WORK_NOT_STARTED");

	return workResult;
}

void CRenderWorkThread::Execute(const char* name, FUNC_TYPE f)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (m_workHandler->IsMainThread(thisThreadId)) // not required for main thread
	{
		f();
		if (m_loopFunc)
			m_loopFunc();
		return;
	}

	// chose free slot
	Work* work = nullptr;
	CEqSignal* completionSignal = nullptr;
	do
	{
		for(int slot = 0; slot < m_workRingPool.numElem(); ++slot)
		{
			if(m_completionSignal[slot].Wait(0) && Atomic::CompareExchange(m_workRingPool[slot].result, WORK_NOT_STARTED, WORK_TAKEN_SLOT) == WORK_NOT_STARTED)
			{
				work = &m_workRingPool[slot];
				completionSignal = &m_completionSignal[slot];
				completionSignal->Clear();
				break;
			}
		}
	}while(!work);

	work->workIdx = Atomic::Increment(m_workIdx);
	work->name = name;
	work->func = std::move(f);
	work->sync = false;
	Atomic::Exchange(work->result, WORK_PENDING);

	SignalWork();
}

bool CRenderWorkThread::HasPendingWork() const
{
	for (int i = 0; i < m_workRingPool.numElem(); ++i)
	{
		const Work& work = m_workRingPool[i];
		if (Atomic::Load(work.result) != WORK_NOT_STARTED)
		{
			return true;
		}
	}
	return false;
}

int CRenderWorkThread::Run()
{
	bool begun = false;

	for (int i = 0; i < m_workRingPool.numElem(); ++i)
	{
		Work& work = m_workRingPool[i];
		if (Atomic::CompareExchange(work.result, WORK_PENDING, WORK_EXECUTING) == WORK_PENDING)
		{
			if (!begun)
				m_workHandler->BeginAsyncOperation(GetThreadID());
			begun = true;

			PROF_EVENT(work.name);
			const int result = work.func();
			work.func = nullptr;
			Atomic::Exchange(work.result, work.sync ? result : WORK_NOT_STARTED);

			m_lastWorkName = work.name;
			m_lastWorkIdx = work.workIdx;
			m_completionSignal[i].Raise();

			if (m_loopFunc)
				m_loopFunc();
		}
	}

	if (begun)
		m_workHandler->EndAsyncOperation();

	if (m_loopFunc)
		m_loopFunc();

	YieldCurrentThread();

	return 0;
}
