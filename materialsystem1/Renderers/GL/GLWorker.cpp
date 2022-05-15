//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Worker
//////////////////////////////////////////////////////////////////////////////////

#include "GLWorker.h"

#include "core/DebugInterface.h"
#include "core/platform/assert.h"

#ifdef USE_SDL2
#include "CGLRenderLib_SDL.h"
extern CGLRenderLib_SDL g_library;
#elif defined(USE_GLES2)
#include "CGLRenderLib_ES.h"
extern CGLRenderLib_ES g_library;
#else
#include "CGLRenderLib.h"
extern CGLRenderLib g_library;
#endif

GLWorkerThread g_glWorker;

#define WORK_PENDING_MARKER 0x1d1d0001

GLWorkerThread::work_t::work_t(const char* _name, FUNC_TYPE f, uint id, bool block) : func(f)
{
	name = _name;
	result = WORK_PENDING_MARKER;
	workId = id;
	blocking = block;
}

void GLWorkerThread::Init()
{
	StartWorkerThread("GLWorker");
}

void GLWorkerThread::Shutdown()
{
	SignalWork();
	StopThread();

	// delete work
	delete m_pendingWork;
	m_pendingWork = nullptr;
}

int GLWorkerThread::WaitForExecute(const char* name, FUNC_TYPE f)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (g_library.IsMainThread(thisThreadId)) // not required for main thread
		return f();

	return AddWork(name, f, true);
}

int GLWorkerThread::WaitForResult(work_t* work)
{
	ASSERT(work);
	DevMsg(DEVMSG_SHADERAPI, "WaitForResult for %s (workId %d)\n", work->name, work->workId);

	WaitForThread();

	ASSERT(m_pendingWork == work);
	m_pendingWork = nullptr;

	const int result = work->result;

	delete work;

	return result;
}

int GLWorkerThread::AddWork(const char* name, FUNC_TYPE f, bool blocking)
{
	CScopedMutex m(m_mutex);

	// wait before worker gets done
	WaitForThread();

	m_pendingWork = PPNew work_t(name, f, m_workCounter++, blocking);

	SignalWork();

	if (blocking)
		return WaitForResult(m_pendingWork);

	return 0;
}

int GLWorkerThread::Run()
{
	ASSERT_MSG(m_pendingWork, "worker triggered but no work");

	g_library.BeginAsyncOperation(GetThreadID());

	FUNC_TYPE func = m_pendingWork->func;

	// run work
	//DevMsg(DEVMSG_SHADERAPI, "BeginAsyncOperation for %s (workId %d)\n", currentWork->name, currentWork->workId);
	if (m_pendingWork->blocking)
	{
		m_pendingWork->result = func();
	}
	else
	{
		func();
		delete m_pendingWork;
		m_pendingWork = nullptr;
	}
	//DevMsg(DEVMSG_SHADERAPI, "EndAsyncOperation for %s (workId %d)\n", currentWork->name, currentWork->workId);

	g_library.EndAsyncOperation();

	return 0;
}
