//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Render worker thread
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/platform/eqjobmanager.h"

using REND_FUNC_TYPE = EqFunction<int()>;
class CRenderJob;

class RenderWorkerHandler
{
public:
	virtual const char* GetAsyncThreadName() const { return "RenderAsyncWorker"; }
	virtual void		BeginAsyncOperation(uintptr_t threadId) = 0;
	virtual void		EndAsyncOperation() = 0;
	virtual bool		IsMainThread(uintptr_t threadId) const = 0;
};

class CRenderWorker : public Threading::CEqThread
{
public:
	void		Init(RenderWorkerHandler* workHandler, REND_FUNC_TYPE loopFunc, int workPoolSize = 32);
	void		Shutdown();

	// syncronous execution
	int			WaitForExecute(const char* name, REND_FUNC_TYPE f);

	// asyncronous execution
	void		Execute(const char* name, REND_FUNC_TYPE f);
protected:

	int			Run() override;

	struct Work 
	{
		REND_FUNC_TYPE	func;
		volatile int	result = -10000;
	};
	List<Work>				m_asyncJobList{ PP_SL };
	RenderWorkerHandler*	m_workHandler{ nullptr };

	REND_FUNC_TYPE			m_loopFunc;
};

extern CRenderWorker g_renderWorker;
