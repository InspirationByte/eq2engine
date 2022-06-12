//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Async work queue
//				designed to perform multi-threaded calls in single-threaded,
//				protected manner
//////////////////////////////////////////////////////////////////////////////////

#pragma once

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
	Array<struct work_t*>	m_pendingWork{ PP_SL };
	Threading::CEqMutex		m_mutex;
};
