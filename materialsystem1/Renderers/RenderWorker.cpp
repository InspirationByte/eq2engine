//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Render worker thread
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IEqParallelJobs.h"
#include "RenderWorker.h"

using namespace Threading;

static CEqJobManager* s_renderJobMng = nullptr;
CRenderWorker g_renderWorker;

class CRenderJob : public IParallelJob
{
public:
    template<typename F>
    CRenderJob(const char* jobName, F func)
        : IParallelJob(jobName)
        , m_jobFunction(std::move(func))
    {
    }

    void Execute() override;

    REND_FUNC_TYPE	m_jobFunction;
    int 			m_result = -1;
};

class CAsyncStarterJob : public IParallelJob
{
public:
    CAsyncStarterJob(const char* jobName, CRenderJob* job, IParallelJob* waitJob)
        : IParallelJob(jobName)
        , m_incidentJob(job)
        , m_waitJob(waitJob)
    {
    }

    void Execute() override;

protected:
    CRenderJob* m_incidentJob;
    IParallelJob* m_waitJob;
};

void CRenderJob::Execute()
{
    ASSERT_MSG(g_renderWorker.GetThreadID() == Threading::GetCurrentThreadID(), "Invalid thread for %s!", m_jobName.ToCString());

    //Msg("started %s\n", m_jobName.ToCString());
    m_result = m_jobFunction();
}

void CAsyncStarterJob::Execute()
{
    //Msg("- prep async %s\n", m_incidentJob->GetName());

    m_waitJob->GetSignal()->Wait();
    m_waitJob->InitJob();
    m_waitJob->AddWait(m_incidentJob);
    s_renderJobMng->StartJob(m_waitJob);
    DeleteOnFinish();
}

//------------------------------

void CRenderWorker::Init(RenderWorkerHandler* workHandler, REND_FUNC_TYPE loopFunc, int workPoolSize)
{
    if (loopFunc)
        m_loopJob = PPNew CRenderJob("EndWorkLoopJob", loopFunc);
    else
        m_loopJob = PPNew SyncJob("EndWorkSyncJob");

    m_loopJob->InitSignal(false);
    m_workHandler = workHandler;

    s_renderJobMng = PPNew CEqJobManager("RenderWorkerJobMng", 1, workPoolSize);

    FunctionJob funcJob("GetThreadIdJob", [&](void*, int i) {
        m_jobThreadId = Threading::GetCurrentThreadID();
        });
    funcJob.InitSignal();
    s_renderJobMng->InitStartJob(&funcJob);
    funcJob.GetSignal()->Wait();

    ASSERT_MSG(m_jobThreadId != (uintptr_t)-1, "RenderWorker thread ID was not initialized");
    ASSERT_MSG(Threading::GetCurrentThreadID() != m_jobThreadId, "Please make sure RenderWorker is not at main thread!");
}

void CRenderWorker::Shutdown()
{
    if(m_loopJob && m_loopJob->GetSignal())
        m_loopJob->GetSignal()->Wait();
    if(s_renderJobMng)
        s_renderJobMng->Wait();

    SAFE_DELETE(m_loopJob);
    SAFE_DELETE(s_renderJobMng);
}

int CRenderWorker::WaitForExecute(const char* name, REND_FUNC_TYPE f)
{
    const uintptr_t thisThreadId = Threading::GetCurrentThreadID();

    if (m_workHandler->IsMainThread(thisThreadId)) // not required for main thread
    {
        const int res = f();
        m_loopJob->Execute();
        return res;
    }

    CRenderJob* job = PPNew CRenderJob(name, std::move(f));
    job->InitJob();

    // wait for previous device spin to complete
    m_loopJob->GetSignal()->Wait();

    m_loopJob->InitJob();
    m_loopJob->AddWait(job);

    m_lastWorkName = name;

    s_renderJobMng->StartJob(job);
    s_renderJobMng->StartJob(m_loopJob);

    // wait for job and device spin
    m_loopJob->GetSignal()->Wait();
    m_loopJob->GetSignal()->Raise();

    const int res = job->m_result;
    delete job;

    return res;
}

void CRenderWorker::Execute(const char* name, REND_FUNC_TYPE f)
{
    // start async job
    CRenderJob* job = PPNew CRenderJob(name, std::move(f));
    job->DeleteOnFinish();

    CAsyncStarterJob* starterJob = PPNew CAsyncStarterJob(name, job, m_loopJob);
    starterJob->InitJob();
    job->AddWait(starterJob);
    s_renderJobMng->StartJob(job);

    g_parallelJobs->GetJobMng()->InitStartJob(starterJob);
}

void CRenderWorker::WaitForThread() const
{
    SubmitJobs();
    m_loopJob->GetSignal()->Wait();
    m_loopJob->GetSignal()->Raise();
}

void CRenderWorker::RunLoop()
{
    SubmitJobs();

    // wait for previous device loop to complete
    if (m_loopJob->GetSignal()->Wait(0))
    {
        m_loopJob->InitJob();
        s_renderJobMng->StartJob(m_loopJob);
    }
}

void CRenderWorker::SubmitJobs() const
{
    if (!s_renderJobMng->AllJobsCompleted())
        s_renderJobMng->Submit(1);
}