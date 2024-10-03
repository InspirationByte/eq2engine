//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Render worker thread
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
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

void CRenderJob::Execute()
{
	m_result = m_jobFunction();
}

//------------------------------

void CRenderWorker::Init(RenderWorkerHandler* workHandler, REND_FUNC_TYPE loopFunc, int workPoolSize)
{
	if(loopFunc)
		m_loopJob = PPNew CRenderJob("RenderLoopJob", loopFunc);
	else
		m_loopJob = PPNew SyncJob("RenderSyncJob");

	m_loopJob->InitSignal(false);
	m_workHandler = workHandler;
	
	s_renderJobMng = PPNew CEqJobManager("RenderWorkerJobMng", 1, workPoolSize);

	FunctionJob funcJob("GetThreadIdJob", [&](void*, int i){
		m_jobThreadId = Threading::GetCurrentThreadID();
		return 0;
	});
	funcJob.InitSignal();
	s_renderJobMng->InitStartJob(&funcJob);
	funcJob.GetSignal()->Wait();

	ASSERT_MSG(Threading::GetCurrentThreadID() != m_jobThreadId, "Please make sure RenderWorker is not at main thread!");
}

void CRenderWorker::Shutdown()
{
	m_loopJob->GetSignal()->Wait();
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

	SyncJob waitForLoop("WaitLoop");
	waitForLoop.InitSignal();
	waitForLoop.InitJob();
	
	// wait for previous device spin to complete
	m_loopJob->GetSignal()->Wait();

	m_loopJob->InitJob();
	m_loopJob->AddWait(job);

	waitForLoop.AddWait(job);
	waitForLoop.AddWait(m_loopJob);

	m_lastWorkName = name;

	s_renderJobMng->StartJob(job);
	s_renderJobMng->StartJob(m_loopJob);
	s_renderJobMng->StartJob(&waitForLoop);

	// wait for job and device spin
	waitForLoop.GetSignal()->Wait();

	const int res = job->m_result;
	delete job;

	return res;
}

void CRenderWorker::Execute(const char* name, REND_FUNC_TYPE f)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (m_workHandler->IsMainThread(thisThreadId)) // not required for main thread
	{
		f();
		m_loopJob->Execute();
		return;
	}

	// start async job
	CRenderJob* job = PPNew CRenderJob(name, std::move(f));
	job->InitJob();
	job->DeleteOnFinish();

	// wait for previous device spin to complete
	m_loopJob->GetSignal()->Wait();

	m_loopJob->InitJob();
	m_loopJob->AddWait(job);

	s_renderJobMng->StartJob(job);
	s_renderJobMng->StartJob(m_loopJob);
}

void CRenderWorker::WaitForThread() const
{
	m_loopJob->GetSignal()->Wait();
	m_loopJob->GetSignal()->Raise();
}

void CRenderWorker::RunLoop()
{
	// wait for previous device loop to complete
	m_loopJob->GetSignal()->Wait();
	m_loopJob->InitJob();
	s_renderJobMng->StartJob(m_loopJob);
}