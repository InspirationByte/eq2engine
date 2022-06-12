//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Worker
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <atomic>

class GLWorkerThread : Threading::CEqThread
{
	friend class ShaderAPIGL;

public:
	using FUNC_TYPE = EqFunction<int()>;

	GLWorkerThread()
	{
	}

	void Init();

	void Shutdown();

	// syncronous execution
	int WaitForExecute(const char* name, FUNC_TYPE f);

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
		FUNC_TYPE				func;

		const char* name;

		volatile int			result;
		uint					workId;
		bool					blocking;
	};

	int WaitForResult(work_t* work);

	Threading::CEqMutex		m_mutex;
	work_t*					m_pendingWork;

	uint					m_workCounter;
};

extern GLWorkerThread g_glWorker;
