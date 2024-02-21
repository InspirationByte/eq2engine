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

CEqJobThread::CEqJobThread(CEqParallelJobManager* owner) 
	: m_owner(owner), 
	m_curJob(nullptr)
{

}

int CEqJobThread::Run()
{
	// thread will find job by himself
	if( !m_owner->TryPopNewJob( this ) )
		return 0;

	ParallelJob* pjob = const_cast<ParallelJob*>(m_curJob);
	m_curJob = nullptr;
	
	pjob->flags |= JOB_FLAG_CURRENT;

	// execute
	pjob->job->Run();

	pjob->flags |= JOB_FLAG_EXECUTED;
	pjob->flags &= ~JOB_FLAG_CURRENT;

	/*if (pjob->onComplete)
	{
		m_owner->AddCompleted(pjob);
	}
	else */

	if (pjob->flags & JOB_FLAG_DELETE)
		delete pjob->job;
	delete pjob;

	SignalWork();

	return 0;
}

bool CEqJobThread::TryAssignJob( ParallelJob* pjob )
{
	if( m_curJob )
		return false;

	if(pjob->threadId != 0)
		return false;

	pjob->threadId = GetThreadID();
	m_curJob = pjob;

	return true;
}

const ParallelJob* CEqJobThread::GetCurrentJob() const
{
	return const_cast<ParallelJob*>(m_curJob);
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

	m_mainThreadId = Threading::GetCurrentThreadID();

	return true;
}

void CEqParallelJobManager::Shutdown()
{
	Wait();

	for (CEqJobThread* jobThread : m_jobThreads)
		delete jobThread;

	m_jobThreads.clear(true);
	m_workQueue.clear(true);
}

// adds the job to the queue
void CEqParallelJobManager::AddJob(IParallelJob* job)
{
	CScopedMutex m(m_mutex);

	// check if already queued
	for (ParallelJob* pjob : m_workQueue)
	{
		if (pjob->job == job)
			return;
	}

	ParallelJob* pjob = PPNew ParallelJob(job);
	m_workQueue.append(pjob);

	job->OnAddedToQueue();
}

// adds the job
void CEqParallelJobManager::AddJob(EQ_JOB_FUNC func, void* args, int count /*= 1*/)
{
	ASSERT(count > 0);

	FunctionParallelJob* funcJob = PPNew FunctionParallelJob("PJob", func, args, count);

	ParallelJob* job = PPNew ParallelJob(funcJob);
	job->flags = JOB_FLAG_DELETE;

	{
		CScopedMutex m(m_mutex);
		m_workQueue.append(job);
	}
}

// this submits jobs to the CEqJobThreads
void CEqParallelJobManager::Submit()
{
	if (!m_workQueue.numElem())
		return;

	for (CEqJobThread* jobThread : m_jobThreads)
		jobThread->SignalWork();
}

bool CEqParallelJobManager::AllJobsCompleted() const
{
	return m_workQueue.numElem() == 0;
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
	CScopedMutex m(m_mutex);

	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		ParallelJob* pjob = m_workQueue[i];
		if (!pjob->job->WaitForJobGroup(0))
			continue;

		if (requestBy->TryAssignJob(pjob))
		{
			m_workQueue.fastRemoveIndex(i);
			return true;
		}
	}

	return false;
}

int	CEqParallelJobManager::GetActiveJobThreadsCount()
{
	int cnt = 0;
	for (CEqJobThread* jobThread : m_jobThreads)
		cnt += (jobThread->IsWorkDone() == false) ? 1 : 0;

	return cnt;
}

int	CEqParallelJobManager::GetJobThreadsCount()
{
	return m_jobThreads.numElem();
}

int	CEqParallelJobManager::GetActiveJobsCount()
{
	CScopedMutex m(m_mutex);

	int cnt = 0;
	for (const ParallelJob* job : m_workQueue)
	{
		if (job->threadId != 0)
			cnt++;
	}

	return cnt;
}

int	CEqParallelJobManager::GetPendingJobCount()
{
	CScopedMutex m(m_mutex);

	int cnt = 0;
	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		const ParallelJob* job = m_workQueue[i];
		if (job->threadId == 0)
			cnt++;
	}

	return cnt;
}
