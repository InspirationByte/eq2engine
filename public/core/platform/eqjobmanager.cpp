#include "core/core_common.h"
#include "eqjobmanager.h"

using namespace Threading;

IParallelJob::~IParallelJob()
{
	SAFE_DELETE(m_doneEvent);
}

void IParallelJob::InitJob()
{
	ASSERT(m_phase != JOB_STARTED);
	m_nextJobs.clear();

	m_phase = JOB_INIT;
	m_primeJobs = 1;

	if (m_doneEvent)
		m_doneEvent->Clear();
}

void IParallelJob::InitSignal()
{
	if (!m_doneEvent)
	{
		m_doneEvent = PPNew Threading::CEqSignal(true);
		m_doneEvent->Raise();
	}
}


void IParallelJob::AddWait(IParallelJob* jobToWait)
{
	ASSERT(jobToWait != this);
	ASSERT(m_phase == JOB_INIT || (m_phase == JOB_STARTED && m_primeJobs > 0));

	if (jobToWait->m_phase != JOB_DONE)
	{
		Atomic::Increment(m_primeJobs);
		jobToWait->m_nextJobs.append(this);
	}
}

//---------------------------------------------------------- 

class CEqJobManager;

class CEqJobManager::WorkerThread : public CEqThread
{
	friend class CEqJobManager;
public:
	WorkerThread(CEqJobManager& jobManager);

	int						Run();
protected:
	CEqJobManager&      	m_jobManager;
};

CEqJobManager::WorkerThread::WorkerThread(CEqJobManager& jobMng) 
	: m_jobManager(jobMng)
{
}

int CEqJobManager::WorkerThread::Run()
{
	// thread will find job by himself
	IParallelJob* job = m_jobManager.ExtractJobFromQueue();
	if (!job)
		return 0;

	m_jobManager.ExecuteJob(*job);
	return 0;
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
	m_queueSize = queueSize;
	m_workerThreads = ArrayRef(PPAllocStructArray(WorkerThread, numThreads), numThreads);
	for (int i = 0; i < numThreads; ++i)
	{
		new (&m_workerThreads[i]) WorkerThread(*this);
		m_workerThreads[i].StartWorkerThread(EqString::Format("%s_%d", name, i));
	}
}

void CEqJobManager::StartJob(IParallelJob* job)
{
	ASSERT(job->m_phase == IParallelJob::JOB_INIT);
	ASSERT(job->m_primeJobs > 0);
	
	job->m_jobMng = this;
	job->m_phase = IParallelJob::JOB_STARTED;

	const bool canBeStarted = Atomic::Decrement(job->m_primeJobs) == 0;
	if (canBeStarted)
	{
		const bool queued = m_jobQueue.enqueue(job);
		ASSERT_MSG(queued, "Jobs queue is too small (%d), please increase", m_queueSize);
		Submit();
	}
}

void CEqJobManager::ExecuteJob(IParallelJob& job)
{
	ASSERT(job.m_primeJobs <= 0);
	ASSERT(job.m_phase == IParallelJob::JOB_STARTED);
	ASSERT(job.m_jobMng == this);

	// execute
	{
		PROF_EVENT(job.m_jobName);
		job.Execute();
	}
	job.m_phase = IParallelJob::JOB_DONE;

	IParallelJob** unblockedJobs = reinterpret_cast<IParallelJob**>(m_queueSize * sizeof(IParallelJob*));
	int numUnblocked = 0;

	for (IParallelJob* nextJob : job.m_nextJobs)
	{
		const bool canBeStarted = Atomic::Decrement(nextJob->m_primeJobs) == 0;
		if (canBeStarted)
			unblockedJobs[numUnblocked++] = nextJob;
	}

	if (job.m_doneEvent)
		job.m_doneEvent->Raise();

	for (int i = 0; i < numUnblocked; ++i)
	{
		IParallelJob* unblockedJob = unblockedJobs[i];
		CEqJobManager* unblockedMng = unblockedJob->m_jobMng;
		unblockedMng->StartJob(unblockedJob);
		unblockedMng->Submit();
	}

	if (job.m_deleteJob)
		delete& job;
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
		if(!thread.WaitForThread(0))
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

IParallelJob* CEqJobManager::ExtractJobFromQueue()
{
	IParallelJob* job = nullptr;
	if (!m_jobQueue.dequeue(job))
		return nullptr;

	return job;
}