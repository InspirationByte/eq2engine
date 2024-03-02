//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqParallelJobs.h"
#include "core/IDkCore.h"
#include "core/IEqCPUServices.h"

using namespace Threading;

EXPORTED_INTERFACE(IEqParallelJobManager, CEqParallelJobManager);

//
// The job execution thread
//
class CEqJobThread : public Threading::CEqThread
{
	friend class CEqParallelJobManager;
public:
	CEqJobThread(CEqParallelJobManager* owner);

	int						Run();
	bool					TryAssignJob(IParallelJob* job);

	const IParallelJob*		GetCurrentJob() const;

protected:

	volatile IParallelJob*	m_curJob;
	CEqParallelJobManager*	m_owner;
};

CEqJobThread::CEqJobThread(CEqParallelJobManager* owner) 
	: m_owner(owner), 
	m_curJob(nullptr)
{
}

int CEqJobThread::Run()
{
	// thread will find job by himself
	if( !m_owner->TryPopNewJob( this ) )
	{
		//SignalWork();
		return 0;
	}

	IParallelJob* job = const_cast<IParallelJob*>(m_curJob);
	m_curJob = nullptr;

	// execute
	job->Run();
	job->m_phase = IParallelJob::JOB_DONE;

	if(job->m_deleteJob)
		delete job;

	SignalWork();

	return 0;
}

bool CEqJobThread::TryAssignJob( IParallelJob* pjob )
{
	if( m_curJob )
		return false;

	m_curJob = pjob;

	return true;
}

const IParallelJob* CEqJobThread::GetCurrentJob() const
{
	return const_cast<IParallelJob*>(m_curJob);
}

//-------------------------------------------------------------------------------------------

CEqParallelJobManager::CEqParallelJobManager()
{
	// required by mobile port
	g_eqCore->RegisterInterface(this);
}

CEqParallelJobManager::~CEqParallelJobManager()
{
	g_eqCore->UnregisterInterface<CEqParallelJobManager>();
}

// creates new job thread
bool CEqParallelJobManager::Init()
{
	const int numThreadsToSpawn = max(4, g_cpuCaps->GetCPUCount());
	for (int i = 0; i < numThreadsToSpawn; ++i)
	{
		CEqJobThread* jobThread = PPNew CEqJobThread(this);
		m_jobThreads.append(jobThread);

		jobThread->StartWorkerThread(EqString::Format("eqWorker_%d", i));
	}

	MsgInfo("*Parallel jobs threads: %d\n", numThreadsToSpawn);

	return true;
}

void CEqParallelJobManager::Shutdown()
{
	Wait();

	for (CEqJobThread* jobThread : m_jobThreads)
		delete jobThread;

	m_jobThreads.clear(true);
}

// adds the job to the queue
void CEqParallelJobManager::AddJob(IParallelJob* job)
{
	if(job->m_phase == IParallelJob::JOB_STARTED)
		return;

	job->m_phase = IParallelJob::JOB_STARTED;
	m_jobs.enqueue(job);
	
	job->OnAddedToQueue();

	Submit();
}

// adds the job
void CEqParallelJobManager::AddJob(EQ_JOB_FUNC func, void* args, int count /*= 1*/)
{
	ASSERT(count > 0);

	FunctionParallelJob* job = PPNew FunctionParallelJob("PJob", func, args, count);
	job->m_phase = IParallelJob::JOB_STARTED;
	job->m_deleteJob = true;
	
	m_jobs.enqueue(job);
	job->OnAddedToQueue();

	Submit();
}

// this submits jobs to the CEqJobThreads
void CEqParallelJobManager::Submit()
{
	for (CEqJobThread* jobThread : m_jobThreads)
		jobThread->SignalWork();
}

bool CEqParallelJobManager::AllJobsCompleted() const
{
	for (CEqJobThread* jobThread : m_jobThreads)
	{
		if(!jobThread->WaitForThread(0))
			return false;
	}
	return true;
}

// wait for completion
void CEqParallelJobManager::Wait(int waitTimeout)
{
	for (CEqJobThread* jobThread : m_jobThreads)
		jobThread->WaitForThread(waitTimeout);
}

// called by job thread
bool CEqParallelJobManager::TryPopNewJob( CEqJobThread* requestBy )
{
	IParallelJob* job = nullptr;
	if(!m_jobs.dequeue(job))
		return false;
	
	if(job->WaitForJobGroup(0) && requestBy->TryAssignJob(job))
	{
		return true;
	}
	else
		m_jobs.enqueue(job);

	return false;
}

int	CEqParallelJobManager::GetJobThreadsCount()
{
	return m_jobThreads.numElem();
}
