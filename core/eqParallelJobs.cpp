//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

#include "InterfaceManager.h"

#include "eqParallelJobs.h"
#include "DebugInterface.h"
#include "utils/strtools.h"

EXPORTED_INTERFACE(IEqParallelJobThreads, CEqParallelJobThreads);

CEqJobThread::CEqJobThread(CEqParallelJobThreads* owner ) : m_owner(owner), m_curJob(nullptr)
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
			(job->func)(job->arguments, iter++ );
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

	// применить работу к потоку
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
}

CEqParallelJobThreads::~CEqParallelJobThreads()
{
}

// creates new job thread
bool CEqParallelJobThreads::Init( int numThreads )
{
	if (numThreads == 0)
		numThreads = 1;

	MsgInfo("*Parallel jobs threads: %d\n", numThreads);

	for (int i = 0; i < numThreads; i++)
	{
		m_jobThreads.append(new CEqJobThread(this));
		m_jobThreads[i]->StartWorkerThread(varargs("jobThread_%d", i));
	}

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
eqParallelJob_t* CEqParallelJobThreads::AddJob(jobFunction_t func, void* args, int count /*= 1*/)
{
	eqParallelJob_t* job = new eqParallelJob_t;
	job->flags = JOB_FLAG_DELETE;
	job->func = func;
	job->arguments = args;
	job->numIter = count;

	AddJob( job );

	return job;
}

void CEqParallelJobThreads::AddJob(eqParallelJob_t* job)
{
	m_mutex.Lock();
	m_workQueue.addLast( job );
	m_mutex.Unlock();
}

// this submits jobs to the CEqJobThreads
void CEqParallelJobThreads::Submit()
{
	m_mutex.Lock();

	CompleteJobCallbacks();

	// now signal threads to search for their new jobs
	if (m_workQueue.getCount())
	{
		m_mutex.Unlock();

		for (int i = 0; i < m_jobThreads.numElem(); i++)
			m_jobThreads[i]->SignalWork();
	}
	else
		m_mutex.Unlock();
}

void CEqParallelJobThreads::CompleteJobCallbacks()
{
	// only for main thread
	if (Threading::GetCurrentThreadID() != m_mainThreadId)
		return;

	// execute all job completion callbacks first
	if (m_completedJobs.goToFirst())
	{
		do
		{
			eqParallelJob_t* job = m_completedJobs.getCurrent();

			job->onComplete(job);

			// done with it
			if (job->flags & JOB_FLAG_DELETE)
				delete job;

		} while (m_completedJobs.goToNext());

		m_completedJobs.clear();
	}
}

bool CEqParallelJobThreads::AllJobsCompleted() const
{
	return m_workQueue.getCount() == 0;
}

// wait for completion
void CEqParallelJobThreads::Wait()
{
	for (int i = 0; i < m_jobThreads.numElem(); i++)
		m_jobThreads[i]->WaitForThread();

	m_mutex.Lock();
	CompleteJobCallbacks();
	m_mutex.Unlock();
}

// wait for specific job
void CEqParallelJobThreads::WaitForJob(eqParallelJob_t* job)
{
	while(!(job->flags & JOB_FLAG_EXECUTED)) { Threading::Yield(); }

	// FIXME: should I do this here too?
	m_mutex.Lock();
	CompleteJobCallbacks();
	m_mutex.Unlock();
}

// called by job thread
bool CEqParallelJobThreads::AssignFreeJob( CEqJobThread* requestBy )
{
	m_mutex.Lock();

	if( m_workQueue.goToFirst() )
	{
		do
		{
			eqParallelJob_t* job = m_workQueue.getCurrent();

			if(!job)
				continue;

			// привязать работу
			if( requestBy->AssignJob( job ) )
			{
				m_workQueue.removeCurrent();
				m_mutex.Unlock();
				return true;
			}

		} while (m_workQueue.goToNext());
	}

	m_mutex.Unlock();

	return false;
}

void CEqParallelJobThreads::AddCompleted(eqParallelJob_t* job)
{
	m_mutex.Lock();

	m_completedJobs.addLast(job);

	m_mutex.Unlock();
}