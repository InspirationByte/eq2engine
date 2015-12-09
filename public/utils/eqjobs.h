//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine threading, the parallel job lists for execution
//				on another threads//////////////////////////////////////////////////////////////////////////////////

#ifndef EQJOBS_H
#define EQJOBS_H

#include "eqthread.h"
#include "DkLinkedList.h"
#include "DkList.h"
#include "ppmem.h"

#ifdef AddJob
#undef AddJob
#endif

namespace Threading
{

typedef void (*jobFunction_t)(void *);

struct eqjob_t
{
	jobFunction_t	fnJob;
	void*			args;
	int				executed;

	// just testing
	PPMEM_MANAGED_OBJECT();
};

enum JobListStatus_e
{
	JOBLIST_IDLE = 0,
	JOBLIST_RUNNING,
	JOBLIST_DONE,
};

//------------------------------------------------------------------------------------------------
// The JOB execution thread
//------------------------------------------------------------------------------------------------

class CEqJobThread : public CEqThread
{
	friend class CEqJobList;

public:
	int					Run();
	void				AddJobList( CEqJobList* pList );

protected:

	CEqMutex			m_AddJobMutex;

	DkList<eqjob_t>		m_pJobList;	// this thread job list
};

//------------------------------------------------------------------------------------------------
// The JOB list
// TODO:	make it parallel, for now jobs runs in single threads and I haven't tested it with more
//			than 2 cores...
//------------------------------------------------------------------------------------------------

class CEqJobList
{
	friend class CEqJobListThreads;
	friend class CEqJobThread;

public:
	CEqJobList();
	
	// adds the job
	void				AddJob( jobFunction_t func, void* args );

	// this submits jobs to the CEqJobThreads
	void				Submit();

	// waits for execution
	void				Wait();

protected:
	//CEqJobListThreads*	m_pThreads;
	CEqJobThread		m_Thread;

	DkList<eqjob_t>		m_pJobList;	// this thread job list
	bool				m_bThreaded;
};

//------------------------------------------------------------------------------------------------
// The job manager - contains threads and used to allocate job lists
//------------------------------------------------------------------------------------------------

class CEqJobManager
{
	
};

};

#endif // EQJOBS_H