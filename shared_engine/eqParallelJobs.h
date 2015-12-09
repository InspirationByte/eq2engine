//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium multithreaded parallel jobs
//////////////////////////////////////////////////////////////////////////////////

/*

This is the diferrent conception of jobs
	Each thread processes single job it got. Each job is assigned to thread in order

	g_parallelJobList->Submit() ----
									\
									 \
							for( each job in jobs )
								m_threads[threadId++]->AddJob( job )

							// each thread is signalled for work
							for( each thread in m_threads )
								thread->SignalWork()

*/

#ifndef EQPARALLELJOBS_H
#define EQPARALLELJOBS_H

#include "utils/eqthread.h"
#include "utils/DkList.h"
#include "utils/DkLinkedList.h"

namespace Threading
{
	typedef void(*jobFunction_t)(void *);

	enum EJobFlags
	{
		JOB_FLAG_ALLOCATED	= (1 << 0),
		JOB_FLAG_EXECUTED	= (1 << 1),
	};

	struct eqparalleljob_t
	{
		eqparalleljob_t()
		{
			flags = 0;
			arguments = nullptr;
		}

		jobFunction_t	func;
		void*			arguments;
		int				flags;		// EJobFlags
	};

	//
	// The job execution thread
	//
	class CEqJobThread : public CEqThread
	{
		friend class CEqParallelJobThreads;
	public:

		CEqJobThread();

		int				Run();
		bool			AddJobToQueue(eqparalleljob_t* job);

	protected:

		CEqMutex						m_jobMutex;

		DkLinkedList<eqparalleljob_t*>	m_jobList;	// this thread job list
	};

	//
	// Job thread list
	//
	class CEqParallelJobThreads // TODO: interface
	{
	public:
		CEqParallelJobThreads();
		virtual ~CEqParallelJobThreads();

		// creates new job thread
		bool					CreateJobThreads( int numThreads );
		void					Shutdown();

		// adds the job
		void					AddJob( jobFunction_t func, void* args );	// and puts JOB_FLAG_ALLOCATED flag for this job
		void					AddJob( eqparalleljob_t* job );

		// this submits jobs to the CEqJobThreads
		void					Submit();

		// wait for completion
		void					Wait();
		int						GetNumJobs(int nThread);

		CEqMutex&				GetMutex() { return m_mutex; }


	protected:

		CEqMutex				m_mutex;

		DkList<CEqJobThread*>	m_jobThreads;
		int						m_curThread;
	};
}

extern Threading::CEqParallelJobThreads* g_parallelJobs;

#endif // EQPARALLELJOBS_H