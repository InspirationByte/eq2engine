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

class CRenderWorker
{
public:
    void		Init(RenderWorkerHandler* workHandler, REND_FUNC_TYPE loopFunc, int workPoolSize = 32);
    void		Shutdown();

    const char* GetLastWorkName() const { return m_lastWorkName; }

    // syncronous execution
    int			WaitForExecute(const char* name, REND_FUNC_TYPE f);

    // asyncronous execution
    void		Execute(const char* name, REND_FUNC_TYPE f);

    void		RunLoop();
    void		SubmitJobs() const;
    void		WaitForThread() const;

    uintptr_t	             GetThreadID() const { return m_jobThreadId; }

protected:
    EqString					 m_lastWorkName;
    uintptr_t				    m_jobThreadId{ 0 };
    IParallelJob*           m_loopJob{ nullptr };
    RenderWorkerHandler*    m_workHandler{ nullptr };
};

extern CRenderWorker g_renderWorker;
