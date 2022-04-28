//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Worker
//////////////////////////////////////////////////////////////////////////////////

#include "GLWorker.h"
#include "CGLRenderLib.h"

#include "core/DebugInterface.h"
#include "core/platform/assert.h"

extern CGLRenderLib g_library;

GLWorkerThread g_glWorker;

#define WORK_PENDING_MARKER 0x1d1d0001

GLWorkerThread::work_t::work_t(const char* _name, FUNC_TYPE f, uint id, bool block) : func(f)
{
	name = _name;
	result = WORK_PENDING_MARKER;
	workId = id;
	blocking = block;
}

int GLWorkerThread::WaitForResult(work_t* work)
{
	ASSERT(work);

	DevMsg(DEVMSG_SHADERAPI, "WaitForResult for %s (workId %d)\n", work->name, work->workId);

	// wait
	do
	{
		const int result = work->result;
		if (result != WORK_PENDING_MARKER)
		{
			delete work;
			return result;
		}

		// HACK: weird hack to make this unfreeze itself
		if (IsWorkDone() && !m_pendingWork)
		{
			m_pendingWork = work;
			SignalWork();
		}

		Platform_Sleep(1);
	} while (true);

	return 0;
}

int GLWorkerThread::AddWork(const char* name, FUNC_TYPE f, bool blocking)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (blocking && g_library.IsMainThread(thisThreadId)) // not required for main thread
		return f();

	work_t* work = new work_t(name, f, m_workCounter++, blocking);

	// chain link
	work->next = m_pendingWork.load();
	m_pendingWork = work;

	SignalWork();

	if (blocking)
		return WaitForResult(work);

	return 0;
}

int GLWorkerThread::Run()
{
	g_library.BeginAsyncOperation(GetThreadID());

	// find some work
	do
	{
		work_t* currentWork = m_pendingWork.load();

		if (currentWork)
			m_pendingWork = currentWork->next.load();
		else
			break;

		FUNC_TYPE func = currentWork->func;

		// run work
		//DevMsg(DEVMSG_SHADERAPI, "BeginAsyncOperation for %s (workId %d)\n", currentWork->name, currentWork->workId);
		if (currentWork->blocking)
		{
			currentWork->result = func();
		}
		else
		{
			func();
			delete currentWork;
		}
		//DevMsg(DEVMSG_SHADERAPI, "EndAsyncOperation for %s (workId %d)\n", currentWork->name, currentWork->workId);

		if (IsTerminating())
			break;
	} while (true);

	g_library.EndAsyncOperation();

	return 0;
}
