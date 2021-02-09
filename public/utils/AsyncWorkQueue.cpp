//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Async work queue
//				designed to perform multi-threaded calls in single-threaded,
//				protected manner
//////////////////////////////////////////////////////////////////////////////////

#include "AsyncWorkQueue.h"

#define WORK_PENDING_MARKER 0xbeefea1e

struct work_t
{
	work_t(std::function<int()> f, uint32_t id)
	{
		func = f;
		workId = id;
	}

	std::function<int()> func;
	uint32_t workId;
};

void CAsyncWorkQueue::RemoveAll()
{
	Threading::CScopedMutex m(m_mutex);

	for (int i = 0; i < m_pendingWork.numElem(); i++)
		delete m_pendingWork[i];

	m_pendingWork.clear();
}

int CAsyncWorkQueue::Push(std::function<int()> f)
{
	// set the new worker and signal to start...
	Threading::CScopedMutex m(m_mutex);

	work_t* work = new work_t(f, m_workCounter++);
	m_pendingWork.append(work);

	return 0;
}

int CAsyncWorkQueue::RunAll()
{
	work_t* currentWork = nullptr;

	// find some work
	while (!currentWork)
	{
		// search for work
		{
			Threading::CScopedMutex m(m_mutex);

			for (int i = 0; i < m_pendingWork.numElem(); i++)
			{
				currentWork = m_pendingWork[i];
				break;
			}

			// no work and haven't picked one?
			if (m_pendingWork.numElem() == 0 && !currentWork)
				break;
		}

		if (currentWork)
		{
			work_t* cur = currentWork;
			{
				Threading::CScopedMutex m(m_mutex);
				m_pendingWork.fastRemove(currentWork);
			}

			// get and quickly dispose
			currentWork = nullptr;

			// run work
			cur->func();
			delete cur;
		}
	}

	return 0;
}
