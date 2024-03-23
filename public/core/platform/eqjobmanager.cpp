#include "core/core_common.h"
#include "eqjobmanager.h"

using namespace Threading;

IParallelJob::~IParallelJob()
{
	SAFE_DELETE(m_jobSignalDone);
}

void IParallelJob::InitSignal()
{
	if (!m_jobSignalDone)
	{
		m_jobSignalDone = PPNew Threading::CEqSignal(true);
		m_jobSignalDone->Raise();
	}
}

void IParallelJob::OnAddedToQueue()
{
	if (m_jobSignalDone)
		m_jobSignalDone->Clear();

	FillJobGroup();
}

bool IParallelJob::WaitForJobGroup(int timeout)
{
	// wait until all jobs has been done
	for (Threading::CEqSignal* signal : m_waitList)
	{
		if (signal->Wait(timeout))
			return false;
	}
	m_waitList.clear();

	return true;
}

void IParallelJob::Run()
{
	PROF_EVENT(m_jobName);
	Execute();

	if(m_jobSignalDone)
		m_jobSignalDone->Raise();	
}

void IParallelJob::AddWait(Threading::CEqSignal* jobWait)
{
	if (!jobWait)
		return;
	m_waitList.append(jobWait);
}

void IParallelJob::AddWait(IParallelJob* jobWait)
{
	jobWait->InitSignal();

	AddWait(jobWait->GetJobSignal());
}

//---------------------------------------------------------- 

class CEqJobManager;

class CEqJobManager::WorkerThread : public CEqThread
{
	friend class CEqJobManager;
public:
	WorkerThread(CEqJobManager& jobManager);

	int						Run();
	bool					TryAssignJob(IParallelJob* job);

	const IParallelJob*		GetCurrentJob() const;

protected:

	volatile IParallelJob*	m_curJob;
	CEqJobManager&      	m_jobManager;
};

CEqJobManager::WorkerThread::WorkerThread(CEqJobManager& jobMng) 
	: m_jobManager(jobMng), 
	m_curJob(nullptr)
{
}

int CEqJobManager::WorkerThread::Run()
{
	// thread will find job by himself
	if( !m_jobManager.TryPopNewJob( *this ) )
	{
		//SignalWork();
		return 0;
	}

	IParallelJob* job = const_cast<IParallelJob*>(m_curJob);
	m_curJob = nullptr;

	// execute
	job->Run();
	job->m_phase = IParallelJob::JOB_DONE;

	if(job->m_deleteJob)
		delete job;

	SignalWork();

	return 0;
}

bool CEqJobManager::WorkerThread::TryAssignJob( IParallelJob* pjob )
{
	if( m_curJob )
		return false;

	m_curJob = pjob;

	return true;
}

const IParallelJob* CEqJobManager::WorkerThread::GetCurrentJob() const
{
	return const_cast<IParallelJob*>(m_curJob);
}

//---------------------------------------------------------- 

CEqJobManager::~CEqJobManager()
{
	for (WorkerThread& thread : m_workerThreads)
		thread.~WorkerThread();

	PPFree(m_workerThreads.ptr());
}

CEqJobManager::CEqJobManager(const char* name, int numThreads, int queueSize)
	: m_jobQueue(queueSize)
{
	m_workerThreads = ArrayRef(PPAllocStructArray(WorkerThread, numThreads), numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		new (&m_workerThreads[i]) WorkerThread(*this);
		m_workerThreads[i].StartWorkerThread(EqString::Format("%s_%d", name, i));
	}
}

void CEqJobManager::AddJob(IParallelJob* job)
{
	if(job->m_phase == IParallelJob::JOB_STARTED)
		return;

	job->m_phase = IParallelJob::JOB_STARTED;

	while (!m_jobQueue.enqueue(job))
	{
		Submit();
		Threading::YieldCurrentThread();
	}
	
	job->OnAddedToQueue();

	Submit();
}

void CEqJobManager::Submit()
{
	for (WorkerThread& thread : m_workerThreads)
		thread.SignalWork();
}

bool CEqJobManager::AllJobsCompleted() const
{
	for (WorkerThread& thread : m_workerThreads)
	{
		if(!thread.GetCurrentJob())
			return false;
	}

	IParallelJob* job = nullptr;
	if (!m_jobQueue.dequeue(job))
		return true;

	m_jobQueue.enqueue(job);
	return false;
}

void CEqJobManager::Wait(int waitTimeout)
{
	for (WorkerThread& thread : m_workerThreads)
		thread.WaitForThread(waitTimeout);
}

bool CEqJobManager::TryPopNewJob(WorkerThread& thread)
{
	IParallelJob* job = nullptr;
	if(!m_jobQueue.dequeue(job))
		return false;
	
	if(job->WaitForJobGroup(0) && thread.TryAssignJob(job))
	{
		return true;
	}
	else
		m_jobQueue.enqueue(job);

	return false;
}