//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"
#include "utils/strtools.h"

namespace Threading
{
	CEqJobThread::CEqJobThread()
	{
	}

	int CEqJobThread::Run()
	{
		if (m_jobList.goToFirst())
		{
			do
			{
				eqParallelJob_t* job = m_jobList.getCurrent();

				if(!job)
					continue;

				job->flags |= JOB_FLAG_CURRENT;

				m_curJob = job;

				// выполнение
				(job->func)( job->arguments );

				job->flags |= JOB_FLAG_EXECUTED;
				job->flags &= ~JOB_FLAG_CURRENT;

				m_curJob = NULL;

				if (job->flags & JOB_FLAG_ALLOCATED)
				{
					m_jobList.setCurrent(NULL);
					delete job;
				}
			} while (m_jobList.goToNext());
		}

		m_jobMutex.Lock();
		m_jobList.clear();
		m_jobMutex.Unlock();

		return 0;
	}

	bool CEqJobThread::AddJobToQueue( eqParallelJob_t* job )
	{
		if (m_jobMutex.Lock())
		{
			job->threadId = GetThreadID();

			m_jobList.addLast(job);
			m_jobMutex.Unlock();

			return true;
		}

		return false;
	}

	const eqParallelJob_t* CEqJobThread::GetCurrentJob() const
	{
		return const_cast<eqParallelJob_t*>(m_curJob);
	}

	//-------------------------------------------------------------------------------------------

	CEqParallelJobThreads::CEqParallelJobThreads() : m_mutex( GetGlobalMutex( MUTEXPURPOSE_JOBMANAGER ) )
	{
		m_curThread = 0;
	}

	CEqParallelJobThreads::~CEqParallelJobThreads()
	{
		Shutdown();
	}

	// creates new job thread
	bool CEqParallelJobThreads::Init( int numThreads )
	{
		if(numThreads == 0)
			return false;

		for (int i = 0; i < numThreads; i++)
		{
			m_jobThreads.append( new CEqJobThread );
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
		m_unsubmittedQueue.addLast( job );
		m_mutex.Unlock();
	}

	// this submits jobs to the CEqJobThreads
	void CEqParallelJobThreads::Submit()
	{
		m_mutex.Lock();

		if (m_unsubmittedQueue.goToFirst())
		{
			do
			{
				eqParallelJob_t* job = m_unsubmittedQueue.getCurrent();

				// правязать работу к потоку
				while(!m_jobThreads[m_curThread]->AddJobToQueue(job))
				{
					m_curThread++;

					if (m_curThread >= m_jobThreads.numElem())
						m_curThread = 0; // start over
				}
			} while (m_unsubmittedQueue.goToNext());
		}

		m_unsubmittedQueue.clear();

		for (int i = 0; i < m_jobThreads.numElem(); i++)
		{
			if (m_jobThreads[i]->m_jobList.getCount() > 0)
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

	int	CEqParallelJobThreads::GetNumJobs(int nThread)
	{
		return m_jobThreads[nThread]->m_jobList.getCount();
	}
}

static Threading::CEqParallelJobThreads s_parallelJobs;
Threading::CEqParallelJobThreads* g_parallelJobs = &s_parallelJobs;