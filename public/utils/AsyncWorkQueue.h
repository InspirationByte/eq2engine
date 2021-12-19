//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Async work queue
//				designed to perform multi-threaded calls in single-threaded,
//				protected manner
//////////////////////////////////////////////////////////////////////////////////

#ifndef ASYNCWORKQUEUE_H
#define ASYNCWORKQUEUE_H

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "utils/function.h"

class CAsyncWorkQueue
{
public:
	CAsyncWorkQueue()
	{
	}

	int		Push(EqFunction<int()> f);
	int		RunAll();
	void	RemoveAll();

protected:

	int WaitForResult(uint32_t workId);

	uint32_t m_workCounter;
	DkList<struct work_t*> m_pendingWork;
	Threading::CEqMutex m_mutex;
};

#endif // ASYNCWORKQUEUE_H

