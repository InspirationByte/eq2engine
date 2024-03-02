//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IEqParallelJobs.h"
#include "ds/boundedqueue.h"

class CEqJobThread;

//
// Job thread list
//
class CEqParallelJobManager : public IEqParallelJobManager
{
	friend class CEqJobThread;
public:
	CEqParallelJobManager();
	virtual ~CEqParallelJobManager();

	bool			IsInitialized() const { return m_jobThreads.numElem() > 0; }

	// creates new job thread
	bool			Init();
	void			Shutdown();

	// adds the job
	void			AddJob(IParallelJob* job);
	void			AddJob(EQ_JOB_FUNC func, void* args = nullptr, int count = 1);	// and puts JOB_FLAG_DELETE flag for this job

	// this submits jobs to the CEqJobThreads
	void			Submit();

	void			Wait(int waitTimeout = Threading::WAIT_INFINITE);

	bool			AllJobsCompleted() const;
	int				GetJobThreadsCount();

protected:

	// called from worker thread
	bool					TryPopNewJob( CEqJobThread* requestBy );

	Array<CEqJobThread*>		m_jobThreads{ PP_SL };
	BoundedQueue<IParallelJob*>	m_jobs{ 1024 };
};