//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Worker
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLWORKER_H
#define GLWORKER_H

#include "utils/eqthread.h"
#include "ds/function.h"

#include <atomic>

class GLWorkerThread : Threading::CEqThread
{
	friend class ShaderAPIGL;

public:
	using FUNC_TYPE = EqFunction<int()>;

	GLWorkerThread()
	{
	}

	void Init()
	{
		StartWorkerThread("GLWorker");
	}

	void Shutdown()
	{
		SignalWork();
		StopThread();

		// delete work
		work_t* work = m_pendingWork;
		m_pendingWork = nullptr;
		while (work)
		{
			work_t* nextWork = work->next;
			delete work;
			work = nextWork;
		}
	}

	// syncronous execution
	int WaitForExecute(const char* name, FUNC_TYPE f)
	{
		return AddWork(name, f, true);
	}

	// asyncronous execution
	void Execute(const char* name, FUNC_TYPE f)
	{
		AddWork(name, f, false);
	}

protected:
	int Run();

	int AddWork(const char* name, FUNC_TYPE f, bool blocking);

	struct work_t
	{
		work_t(const char* _name, FUNC_TYPE f, uint id, bool block);

		std::atomic<work_t*>	next{ nullptr };
		FUNC_TYPE				func;

		const char* name;

		volatile int			result;
		uint					workId;
		bool					blocking;
	};

	int WaitForResult(work_t* work);

	std::atomic<work_t*>	m_pendingWork;

	uint					m_workCounter;
};

extern GLWorkerThread g_glWorker;

#endif // GLWORKER_H