//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Game system Job Thread
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "sys/sys_job.h"

CGameSystemJob::CGameSystemJob(EJobTypes jobType, const char* jobName, bool autoStart)
	: m_type(jobType), m_jobName(jobName)
{
}

void CGameSystemJob::Wait()
{
	m_jobDone.Wait();
}

void CGameSystemJob::Run()
{
	m_jobDone.Clear();
	m_waitJobs.clear();

	FillJobGroup();

	for (int i = 0; i < m_waitJobs.numElem(); ++i)
		m_waitJobs[i]->Wait();

	g_parallelJobs->AddJob(m_type, [this](void*, int i) {
		Execute();
		m_jobDone.Raise();
	});
	g_parallelJobs->Submit();
}

void CGameSystemJob::AddWait(CGameSystemJob* jobWait)
{
	m_waitJobs.append(jobWait);
}
