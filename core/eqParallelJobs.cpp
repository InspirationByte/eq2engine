//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqParallelJobs.h"
#include "core/IDkCore.h"

using namespace Threading;

EXPORTED_INTERFACE(IEqParallelJobThreads, CEqParallelJobThreads);

CEqJobThread::CEqJobThread(CEqParallelJobThreads* owner, int jobTypeId) 
	: m_owner(owner), 
	m_curJob(nullptr), 
	m_threadJobTypeId(jobTypeId)
{

}

int CEqJobThread::Run()
{
	// thread will find job by himself
	while( m_owner->AssignFreeJob( this ) )
	{
		eqParallelJob_t* job = const_cast<eqParallelJob_t*>(m_curJob);
		
		job->flags |= JOB_FLAG_CURRENT;

		// execute
		int iter = 0;
		while(job->numIter-- > 0)
		{
			job->func(job->arguments, iter );
			iter++;
		}

		job->flags |= JOB_FLAG_EXECUTED;
		job->flags &= ~JOB_FLAG_CURRENT;

		if (job->onComplete)
		{
			m_owner->AddCompleted(job);
		}
		else if (job->flags & JOB_FLAG_DELETE)
		{
			delete job;
		}

		m_curJob = nullptr;
	}

	return 0;
}

bool CEqJobThread::AssignJob( eqParallelJob_t* job )
{
	if( m_curJob )
		return false;

	if(job->threadId != 0)
		return false;

	// job only for specific thread?
	if (job->typeId != JOB_TYPE_ANY && m_threadJobTypeId != JOB_TYPE_ANY)
	{
		if (job->typeId != m_threadJobTypeId)
			return false;
	}

	// ��������� ������ � ������
	job->threadId = GetThreadID();
	m_curJob = job;

	return true;
}

const eqParallelJob_t* CEqJobThread::GetCurrentJob() const
{
	return const_cast<eqParallelJob_t*>(m_curJob);
}

//-------------------------------------------------------------------------------------------

CEqParallelJobThreads::CEqParallelJobThreads()
{
	// required by mobile port
	g_eqCore->RegisterInterface(this);
}

CEqParallelJobThreads::~CEqParallelJobThreads()
{
	g_eqCore->UnregisterInterface<CEqParallelJobThreads>();
}

// creates new job thread
bool CEqParallelJobThreads::Init(int numJobTypes, eqJobThreadDesc_t* jobTypes)
{
	ASSERT_MSG(numJobTypes > 0 && jobTypes != nullptr, "EqParallelJobThreads ERROR: Invalid parameters passed to Init!!!");

	int numThreadsSpawned = 0;

	for (int i = 0; i < numJobTypes; i++)
	{
		for (int j = 0; j < jobTypes[i].numThreads; j++)
		{
			CEqJobThread* pJobThread = PPNew CEqJobThread(this, jobTypes[i].jobTypeId);
			m_jobThreads.append(pJobThread);

			pJobThread->StartWorkerThread(EqString::Format("jobThread_%d_%d", jobTypes[i].jobTypeId, j).ToCString());
			numThreadsSpawned++;
		}
	}

	MsgInfo("*Parallel jobs threads: %d\n", numThreadsSpawned);

	m_mainThreadId = Threading::GetCurrentThreadID();

	return true;
}

void CEqParallelJobThreads::Shutdown()
{
	Wait();

	for (int i = 0; i < m_jobThreads.numElem(); i++)
		delete m_jobThreads[i];

	m_jobThreads.clear();
}

// adds the job
eqParallelJob_t* CEqParallelJobThreads::AddJob(int jobTypeId, EQ_JOB_FUNC func, void* args, int count /*= 1*/, EQ_JOB_COMPLETE_FUNC completeFn /*= nullptr*/)
{
	ASSERT(count > 0);

	eqParallelJob_t* job = PPNew eqParallelJob_t(jobTypeId, std::move(func), args, count, std::move(completeFn));
	job->flags = JOB_FLAG_DELETE;

	{
		CScopedMutex m(m_mutex);
		m_workQueue.append(job);
	}

	return job;
}

void CEqParallelJobThreads::AddJob(eqParallelJob_t* job)
{
	CScopedMutex m(m_mutex);
	m_workQueue.append( job );
}

// this submits jobs to the CEqJobThreads
void CEqParallelJobThreads::Submit()
{
	CompleteJobCallbacks();

	if (!m_workQueue.numElem())
		return;

	for (int i = 0; i < m_jobThreads.numElem(); i++)
		m_jobThreads[i]->SignalWork();
}

void CEqParallelJobThreads::CompleteJobCallbacks()
{
	// only for main thread
	if (Threading::GetCurrentThreadID() != m_mainThreadId)
		return;

	// execute all job completion callbacks
	{
		CScopedMutex m(m_completeMutex);

		for (int i = 0; i < m_completedJobs.numElem(); ++i)
		{
			eqParallelJob_t* job = m_completedJobs[i];
			bool deleteJob = (job->flags & JOB_FLAG_DELETE);

			job->onComplete(job);

			// done with it
			if (deleteJob)
				delete job;
		}
		m_completedJobs.clear(false);
	}
}

bool CEqParallelJobThreads::AllJobsCompleted() const
{
	return m_workQueue.numElem() == 0;
}

// wait for completion
void CEqParallelJobThreads::Wait()
{
	for (int i = 0; i < m_jobThreads.numElem(); i++)
		m_jobThreads[i]->WaitForThread();

	CompleteJobCallbacks();
}

// wait for specific job
void CEqParallelJobThreads::WaitForJob(eqParallelJob_t* job)
{
	while(!(job->flags & JOB_FLAG_EXECUTED))
	{
		CompleteJobCallbacks();
		Threading::Yield();
	}
}

// called by job thread
bool CEqParallelJobThreads::AssignFreeJob( CEqJobThread* requestBy )
{
	CScopedMutex m(m_mutex);

	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		eqParallelJob_t* job = m_workQueue[i];

		if (!job)
			continue;

		if (requestBy->AssignJob(job))
		{
			m_workQueue.fastRemoveIndex(i);
			return true;
		}
	}

	return false;
}

void CEqParallelJobThreads::AddCompleted(eqParallelJob_t* job)
{
	CScopedMutex m(m_completeMutex);
	m_completedJobs.append(job);
}

int	CEqParallelJobThreads::GetActiveJobThreadsCount()
{
	int cnt = 0;
	for (int i = 0; i < m_jobThreads.numElem(); i++)
		cnt += (m_jobThreads[i]->IsWorkDone() == false) ? 1 : 0;

	return cnt;
}

int	CEqParallelJobThreads::GetJobThreadsCount()
{
	return m_jobThreads.numElem();
}

int	CEqParallelJobThreads::GetActiveJobsCount(int type /*= -1*/)
{
	CScopedMutex m(m_mutex);

	int cnt = 0;
	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		const eqParallelJob_t* job = m_workQueue[i];
		if (job->typeId == type && job->threadId != 0)
			cnt++;
	}

	return cnt;
}

int	CEqParallelJobThreads::GetPendingJobCount(int type /*= -1*/)
{
	CScopedMutex m(m_mutex);

	int cnt = 0;
	for (int i = 0; i < m_workQueue.numElem(); ++i)
	{
		const eqParallelJob_t* job = m_workQueue[i];
		if (job->typeId == type && job->threadId == 0)
			cnt++;
	}

	return cnt;
}
