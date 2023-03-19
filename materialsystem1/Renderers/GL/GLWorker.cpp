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
	{
		m_completionSignal.appendEmplace(true);
		m_completionSignal[i].Raise();
	}

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

	work->func = f;
	work->sync = true;
	Atomic::Exchange(work->result, WORK_PENDING);
	
	SignalWork();
	const bool isSignalled = completionSignal->Wait();
	const int workResult = Atomic::Exchange(work->result, WORK_NOT_STARTED);

	ASSERT_MSG(isSignalled && workResult != WORK_PENDING, "Failed to wait for GL worker task - WORK_PENDING");
	ASSERT_MSG(isSignalled && workResult != WORK_EXECUTING, "Failed to wait for GL worker task - WORK_EXECUTING");
	ASSERT_MSG(isSignalled && workResult != WORK_NOT_STARTED, "Empty slot was working on GL task - WORK_NOT_STARTED");

	return workResult;
}

void GLWorkerThread::Execute(const char* name, FUNC_TYPE f)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (g_library.IsMainThread(thisThreadId)) // not required for main thread
	{
		f();
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

	work->func = f;
	work->sync = false;
	Atomic::Exchange(work->result, WORK_PENDING);

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
