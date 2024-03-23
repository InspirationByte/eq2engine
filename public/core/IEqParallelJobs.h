//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Multithreaded job manager
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/platform/eqjobmanager.h"

//--------------------------------------------
// job manager

class IEqParallelJobManager : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_ParallelJobManager_004")

	// creates new job thread
	virtual bool			Init() = 0;
	virtual void			Shutdown() = 0;

	// adds the job to the queue
	virtual void			AddJob(IParallelJob* job) = 0;

	// adds the job to the queue
	virtual void			AddJob(EQ_JOB_FUNC jobFn, void* args = nullptr, int count = 1) = 0;	// and puts JOB_FLAG_DELETE flag for this job

	virtual void			Wait(int waitTimeout = Threading::WAIT_INFINITE) = 0;

	virtual bool			AllJobsCompleted() const = 0;
	virtual int				GetJobThreadsCount() const = 0;
};

INTERFACE_SINGLETON(IEqParallelJobManager, CEqParallelJobManager, g_parallelJobs)
