//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IEqParallelJobs.h"

//
// Job thread list
//
class CEqParallelJobManager : public IEqParallelJobManager
{
public:
	CEqParallelJobManager();
	virtual ~CEqParallelJobManager();

	bool			IsInitialized() const { return m_jobMng->GetJobThreadsCount(); }

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
	int				GetJobThreadsCount() const { return m_jobMng->GetJobThreadsCount(); }

protected:

	CEqJobManager*	m_jobMng{ nullptr };
};