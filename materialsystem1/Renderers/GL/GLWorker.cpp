//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Worker
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapigl_def.h"
#include "GLWorker.h"

using namespace Threading;

#ifdef USE_SDL2
#include "GLRenderLibrary_SDL.h"
extern CGLRenderLib_SDL g_library;
#elif defined(USE_GLES2)
#include "GLRenderLibrary_ES.h"
extern CGLRenderLib_ES g_library;
#else
#include "GLRenderLibrary.h"
extern CGLRenderLib g_library;
#endif

GLWorkerThread g_glWorker;

void GLWorkerThread::Init()
{
	StartWorkerThread("GLWorker");

	m_workRingPool.setNum(m_workRingPool.numAllocated());

	// by default every work in pool is free (see .Wait call after getting one from ring pool)
	for (int i = 0; i < m_completionSignal.numAllocated(); ++i)
		m_completionSignal.append(CEqSignal(true));

	for (int i = 0; i < m_completionSignal.numElem(); ++i)
		m_completionSignal[i].Raise();
}

void GLWorkerThread::Shutdown()
{
	SignalWork();
	StopThread();

	m_workRingPool.clear(true);
}

int GLWorkerThread::WaitForExecute(const char* name, FUNC_TYPE f)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (g_library.IsMainThread(thisThreadId)) // not required for main thread
	{
		return f();
	}

	// add to work list
	const int slot = Atomic::Increment(m_workCounter) % m_workRingPool.numAllocated();

	Work& work = m_workRingPool[slot];
	CEqSignal& completionSignal = m_completionSignal[slot];

	// wait if we got into busy slot
	completionSignal.Wait();

	work.func = f;
	work.sync = true;
	Atomic::Exchange(work.result, WORK_PENDING);
	completionSignal.Clear();

	SignalWork();
	completionSignal.Wait();

	const int workResult = Atomic::Exchange(work.result, WORK_NOT_STARTED);

	ASSERT_MSG(workResult != WORK_PENDING, "Failed to wait for GL worker task - WORK_PENDING");
	ASSERT_MSG(workResult != WORK_EXECUTING, "Failed to wait for GL worker task - WORK_EXECUTING");
	ASSERT_MSG(workResult != WORK_NOT_STARTED, "Empty slot was working on GL task - WORK_NOT_STARTED");

	return workResult;
}

void GLWorkerThread::Execute(const char* name, FUNC_TYPE f)
{
	const int slot = Atomic::Increment(m_workCounter) % m_workRingPool.numAllocated();

	Work &work = m_workRingPool[slot];
	CEqSignal& completionSignal = m_completionSignal[slot];

	if (work.result != WORK_NOT_STARTED)
		completionSignal.Wait();
	completionSignal.Clear();

	work.func = f;
	work.sync = false;

	Atomic::Exchange(work.result, WORK_PENDING);
	SignalWork();
}

int GLWorkerThread::Run()
{
	bool begun = false;

	for (int i = 0; i < m_workRingPool.numElem(); ++i)
	{
		Work& work = m_workRingPool[i];
		if (Atomic::CompareExchange(work.result, WORK_PENDING, WORK_EXECUTING) == WORK_PENDING)
		{
			if(!begun)
				g_library.BeginAsyncOperation(GetThreadID());
			begun = true;

			const int result = work.func();
			Atomic::Exchange(work.result, work.sync ? result : WORK_NOT_STARTED);
			m_completionSignal[i].Raise();
		}		
	}

	if(begun)
		g_library.EndAsyncOperation();

	return 0;
}
