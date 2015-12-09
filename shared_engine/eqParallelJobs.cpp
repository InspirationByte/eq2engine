//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

#include "eqParallelJobs.h"
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
				eqparalleljob_t* job = m_jobList.getCurrent();

				(job->func)(job->arguments);
				job->flags |= JOB_FLAG_EXECUTED;

				if (job->flags & JOB_FLAG_ALLOCATED)
					delete job;

			} while (m_jobList.goToNext());
		}

		m_jobMutex.Lock();
		m_jobList.clear();
		m_jobMutex.Unlock();

		return 0;
	}

	bool CEqJobThread::AddJobToQueue( eqparalleljob_t* job )
	{
		if (m_jobMutex.Lock())
		{
			m_jobList.addLast(job);
			m_jobMutex.Unlock();

			return true;
		}

		return false;
	}

	//-------------------------------------------------------------------------------------------

	CEqParallelJobThreads::CEqParallelJobThreads()
	{
		m_curThread = 0;
	}

	CEqParallelJobThreads::~CEqParallelJobThreads()
	{
		Shutdown();
	}

	// creates new job thread
	bool CEqParallelJobThreads::CreateJobThreads(int numThreads)
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
		eqparalleljob_t* job = new eqparalleljob_t;
		job->flags = JOB_FLAG_ALLOCATED;
		job->func = func;
		job->arguments = args;

		AddJob( job );
	}

	void CEqParallelJobThreads::AddJob(eqparalleljob_t* job)
	{
		// don't put to busy thread
		while(!m_jobThreads[m_curThread]->AddJobToQueue(job))
		{
			m_curThread++;

			if (m_curThread >= m_jobThreads.numElem())
				m_curThread = 0; // start over
		}
	}

	// this submits jobs to the CEqJobThreads
	void CEqParallelJobThreads::Submit()
	{
		for (int i = 0; i < m_jobThreads.numElem(); i++)
		{
			if (m_jobThreads[i]->m_jobList.getCount() > 0)
				m_jobThreads[i]->SignalWork();
		}
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