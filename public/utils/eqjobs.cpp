//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine threading, the parallel job lists for execution
//				on another threads//////////////////////////////////////////////////////////////////////////////////

#include "eqjobs.h"

#define MIN_JOBLIST_JOBS 32

namespace Threading
{

int	CEqJobThread::Run()
{
	// run and clear
	for(int i = 0; i < m_pJobList.numElem(); i++)
	{
		(m_pJobList[i].fnJob)( m_pJobList[i].args );
		m_pJobList[i].executed = 1;
	}

	m_AddJobMutex.Lock();
	m_pJobList.clear();
	m_AddJobMutex.Unlock();

	return 0;
}

void CEqJobThread::AddJobList( CEqJobList* pList )
{
	m_AddJobMutex.Lock();

	m_pJobList.append( pList->m_pJobList );

	m_AddJobMutex.Unlock();
}

//------------------------------------------------------------------------------------------------

CEqJobList::CEqJobList()
{
	m_pJobList.resize( MIN_JOBLIST_JOBS );
	m_bThreaded = true;
	m_Thread.StartWorkerThread("JobList1");
}
	
// adds the job
void CEqJobList::AddJob( jobFunction_t func, void* args )
{
	eqjob_t job;
	job.fnJob = func;
	job.args = args;
	job.executed = 0;

	m_pJobList.append( job );
}

// this submits jobs to the CEqJobThreads
void CEqJobList::Submit()
{
	// call a joblist threads
	m_Thread.AddJobList(this);
	m_pJobList.clear();

	m_Thread.SignalWork();
}

// waits for execution
void CEqJobList::Wait()
{
	m_Thread.WaitForThread();
}

//------------------------------------------------------------------------------------------------

}