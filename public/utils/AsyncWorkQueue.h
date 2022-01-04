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
#include "ds/DkList.h"
#include "ds/function.h"

class CAsyncWorkQueue
{
public:
	using FUNC_TYPE = EqFunction<int()>;

	CAsyncWorkQueue() = default;

	int		Push(const FUNC_TYPE& f);
	int		RunAll();
	void	RemoveAll();

protected:

	int WaitForResult(uint workId);

	uint					m_workCounter{ 0 };
	DkList<struct work_t*>	m_pendingWork;
	Threading::CEqMutex		m_mutex;
};

#endif // ASYNCWORKQUEUE_H

