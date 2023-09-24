//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	using FUNC_TYPE = EqFunction<void()>;

	CAsyncWorkQueue() = default;

	void	Push(const FUNC_TYPE& f);
	void	RunAll();
	void	RemoveAll();

protected:

	Array<struct SchedWork*>	m_pendingWork{ PP_SL };
	Threading::CEqMutex			m_mutex;
};
