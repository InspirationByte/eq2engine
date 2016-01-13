//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"
#include "DebugInterface.h"
#include "utils/strtools.h"

namespace Threading
{
	CEqJobThread::CEqJobThread( CEqParallelJobThreads* owner ) : m_owner(owner)
	{
	}

	int CEqJobThread::Run()
	{
		// thread will find job by himself
		while( m_owner->AssignFreeJob( this ) )
		{
			m_curJob->flags |= JOB_FLAG_CURRENT;

			// execute
			(m_curJob->func)( m_curJob->arguments );

			m_curJob->flags |= JOB_FLAG_EXECUTED;
			m_curJob->flags &= ~JOB_FLAG_CURRENT;

			if( m_curJob->flags & JOB_FLAG_ALLOCATED )
				delete m_curJob;

			m_curJob = nullptr;
		}

		return 0;
	}

	bool CEqJobThread::AssignJob( eqParallelJob_t* job )
	{
		if( m_curJob )
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

	CEqParallelJobThreads::CEqParallelJobThreads() : m_mutex( GetGlobalMutex( MUTEXPURPOSE_JOBMANAGER ) )
	{
	}

	CEqParallelJobThreads::~CEqParallelJobThreads()
	{
		Shutdown();
	}

	// creates new job thread
	bool CEqParallelJobThreads::Init( int numThreads )
	{
		if(numThreads == 0)
			numThreads = 1;

		MsgInfo("Parallel jobs thread count: %d\n", numThreads);

		for (int i = 0; i < numThreads; i++)
		{
			m_jobThreads.append( new CEqJobThread(this) );
			m_jobThreads[i]->StartWorkerThread( varargs("jobThread_%d", i) );
		}

		return true;
	}

	void CEqParallelJobThreads::Shutdown()
	{
		for (int i = 0; i < m_jobThreads.numElem(); i++)
		{
			m_jobThreads[i]->StopThread(true);
			delete m_jobThreads[i];
		}

		m_jobThreads.clear();
	}

	// adds the job
	void CEqParallelJobThreads::AddJob(jobFunction_t func, void* args)
	{
		eqParallelJob_t* job = new eqParallelJob_t;
		job->flags = JOB_FLAG_ALLOCATED;
		job->func = func;
		job->arguments = args;

		AddJob( job );
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

		if( m_workQueue.getCount() )
		{
			for (int i = 0; i < m_jobThreads.numElem(); i++)
				m_jobThreads[i]->SignalWork();
		}

		m_mutex.Unlock();
	}

	// wait for completion
	void CEqParallelJobThreads::Wait()
	{
		for (int i = 0; i < m_jobThreads.numElem(); i++)
			m_jobThreads[i]->WaitForThread();
	}

	// wait for specific job
	void CEqParallelJobThreads::WaitForJob(eqParallelJob_t* job)
	{
		// TODO: make right code?
	}

	// called by job thread
	bool CEqParallelJobThreads::AssignFreeJob( CEqJobThread* requestBy )
	{
		CScopedMutex m(m_mutex);

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
					return true;
				}

			} while (m_workQueue.goToNext());
		}

		return false;
	}
}

static Threading::CEqParallelJobThreads s_parallelJobs;
Threading::CEqParallelJobThreads* g_parallelJobs = &s_parallelJobs;