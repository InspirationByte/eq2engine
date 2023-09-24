//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Async work queue
//				designed to perform multi-threaded calls in single-threaded,
//				protected manner
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "AsyncWorkQueue.h"

using namespace Threading;

struct SchedWork
{
	SchedWork(const CAsyncWorkQueue::FUNC_TYPE& fn) 
		: func(fn)
	{
	}

	const CAsyncWorkQueue::FUNC_TYPE func;
};

void CAsyncWorkQueue::RemoveAll()
{
	CScopedMutex m(m_mutex);

	for (SchedWork* work : m_pendingWork)
		delete work;
	m_pendingWork.clear(true);
}

void CAsyncWorkQueue::Push(const FUNC_TYPE& f)
{
	CScopedMutex m(m_mutex);
	m_pendingWork.append(PPNew SchedWork(f));
}

void CAsyncWorkQueue::RunAll()
{
	SchedWork* currentWork = nullptr;

	// find some work
	do
	{
		// search for work
		{
			CScopedMutex m(m_mutex);
			if(m_pendingWork.numElem())
			{
				currentWork = m_pendingWork[0];
				m_pendingWork.fastRemoveIndex(0);
			}
		}

		if (!currentWork)
			break;

		currentWork->func();
		delete currentWork;
		currentWork = nullptr;
	}while (true);
}
