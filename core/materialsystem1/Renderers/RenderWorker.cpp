//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Render worker thread
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IEqParallelJobs.h"
#include "RenderWorker.h"

using namespace Threading;

static CEqMutex s_renderWorkerMutex;
CRenderWorker g_renderWorker;

//------------------------------

void CRenderWorker::Init(RenderWorkerHandler* workHandler, REND_FUNC_TYPE loopFunc, int workPoolSize)
{
	m_workHandler = workHandler;
	m_loopFunc = std::move(loopFunc);

	StartWorkerThread("RenderWorkerThread", TP_ABOVE_NORMAL);
}

void CRenderWorker::Shutdown()
{
	StopThread();
}

int CRenderWorker::WaitForExecute(const char* name, REND_FUNC_TYPE f)
{
	const uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (m_workHandler->IsMainThread(thisThreadId)) // not required for main thread
	{
		const int res = f();
		if (m_loopFunc)
			m_loopFunc();
		return res;
	}

	WaitForThread();

	List<Work>::Iterator workIt;
	{
		CScopedMutex m(s_renderWorkerMutex);
		workIt = m_asyncJobList.append(Work{ std::move(f), WORK_WAIT_FOR_COMPLETE });
	}

	ASSERT(!workIt.atEnd());
	
	do
	{
		SignalWork();
		Platform_Sleep(0);
	} while ((*workIt).result == WORK_WAIT_FOR_COMPLETE);

	const int result = (*workIt).result;
	ASSERT(result != WORK_WAIT_FOR_COMPLETE);

	return result;
}

void CRenderWorker::Execute(const char* name, REND_FUNC_TYPE f)
{
	{
		CScopedMutex m(s_renderWorkerMutex);
		m_asyncJobList.append(Work{ std::move(f) });
	}

	SignalWork();
}

int CRenderWorker::Run()
{
	List<Work>::Iterator workIt;
	{
		CScopedMutex m(s_renderWorkerMutex);
		while (m_asyncJobList.getCount())
		{
			workIt = m_asyncJobList.first();
			if ((*workIt).result > WORK_WAIT_FOR_COMPLETE)
			{
				m_asyncJobList.remove(workIt);
				workIt = {};
			}
			else
				break;
		}
	}

	if (workIt.atEnd())
	{
		if (m_loopFunc)
			m_loopFunc();
		return 0;
	}

	const int result = (*workIt).func();
	if (m_loopFunc)
		m_loopFunc();

	if ((*workIt).result == WORK_PENDING)
	{
		CScopedMutex m(s_renderWorkerMutex);
		m_asyncJobList.remove(workIt);
	}
	else
	{
		Atomic::Exchange((*workIt).result, result);
	}

	// more work available
	if (m_asyncJobList.getCount())
		SignalWork();

	return 0;
}
