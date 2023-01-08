//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Game system Job Thread
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IEqParallelJobs.h"

// Game System job - inspired by Husky <3
class CGameSystemJob
{
public:
	virtual ~CGameSystemJob() = default;
	CGameSystemJob(EJobTypes jobType, const char* jobName, bool autoStart);

	virtual void	Execute() = 0;
	virtual void	FillJobGroup() = 0;

	void Wait();

protected:

	void AddWait(CGameSystemJob* jobWait);
	void Run();

	EqString				m_jobName;
	EJobTypes				m_type{ JOB_TYPE_ANY };
	bool					m_autoStart{ false };
	Threading::CEqSignal	m_jobDone;
	Array<CGameSystemJob*>	m_waitJobs{ PP_SL };
};