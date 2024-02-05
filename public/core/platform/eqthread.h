//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine threads
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef PLAT_POSIX

#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>

#endif // PLAT_POSIX

namespace Threading
{

enum ThreadPriority_e
{
	TP_LOWEST = 0,
	TP_BELOW_NORMAL,
	TP_NORMAL,
	TP_ABOVE_NORMAL,
	TP_HIGHEST
};

#ifdef _WIN32
struct EQWIN32_CRITICAL_SECTION
{
	int data[10];
};
struct EQWIN32_SRWLOCK
{
	int data[2];
};
using SignalHandle_t = HANDLE;
using MutexHandle_t = EQWIN32_CRITICAL_SECTION;
using ReadWriteLockHandle_t = EQWIN32_SRWLOCK;
#else
struct SignalHandle_t
{
	pthread_cond_t 	cond;
	pthread_mutex_t mutex;
	volatile int 	waiting; 	// number of threads waiting for a signal
	volatile bool 	manualReset;
	volatile bool 	signaled; 	// is it signaled right now?
};
using MutexHandle_t = pthread_mutex_t;
using ReadWriteLockHandle_t = pthread_rwlock_t;
#endif // _WIN32

static constexpr const int WAIT_INFINITE = -1;
static constexpr const int DEFAULT_THREAD_STACK_SIZE = 3072 * 1024;

void			YieldCurrentThread();
uintptr_t		GetCurrentThreadID();
void			SetCurrentThreadName(const char* name);

// for profiler needs
void			GetThreadName(uintptr_t threadID, char* name, int maxLength);

//----------------------------------------------------------------------------------------
// A mutex is an object that can only be locked by one thread at a time. 
// It's used to prevent two threads from accessing the same piece of data simultaneously.
//----------------------------------------------------------------------------------------
class CEqMutex
{
public:
	CEqMutex();
	~CEqMutex();

	bool			Lock( bool blocking = true );
	void			Unlock();

private:
	CEqMutex(const CEqMutex& s) = delete;
	void			operator=(const CEqMutex& s) = delete;

	MutexHandle_t	m_nHandle;
};


class CScopedMutex
{
public:
	CScopedMutex( CEqMutex &m, bool blocking = true )
		: m_mutex(m) 
	{ 
		m_mutex.Lock(blocking);
	}

	~CScopedMutex()								
	{
		m_mutex.Unlock();
	}

private:
	CEqMutex& m_mutex;
};

//----------------------------------------------------------------------------------------
// Read/Write locker is an object that can be locked both in shared and exclusive ways.
// It's used to prevent two threads from writing the same piece of data simultaneously,
// but allows read-only access for all threads when Write lock is not engaged.
//----------------------------------------------------------------------------------------
class CEqReadWriteLock
{
public:
	CEqReadWriteLock();
	~CEqReadWriteLock();

	void					LockRead();
	void					UnlockRead();

	bool					LockWrite(bool blocking = true);
	void					UnlockWrite();

private:
	CEqReadWriteLock(const CEqReadWriteLock& s) = delete;
	void					operator=(const CEqReadWriteLock& s) = delete;

	ReadWriteLockHandle_t	m_nHandle;
};


class CScopedReadLocker
{
public:
	CScopedReadLocker(CEqReadWriteLock& lock)
		: m_lock(lock)
	{
		m_lock.LockRead();
	}

	~CScopedReadLocker()
	{
		m_lock.UnlockRead();
	}

private:
	CEqReadWriteLock& m_lock;
};

class CScopedWriteLocker
{
public:
	CScopedWriteLocker(CEqReadWriteLock& lock, bool blocking = true)
		: m_lock(lock)
	{
		m_lock.LockWrite(blocking);
	}

	~CScopedWriteLocker()
	{
		m_lock.UnlockWrite();
	}

private:
	CEqReadWriteLock& m_lock;
};

//----------------------------------------------------------------------------------------
// A signal is an object that a thread can wait on for it to be raised. 
// It's used to indicate data is available or that a thread has reached a specific point.
//----------------------------------------------------------------------------------------

class CEqSignal
{
public:
	CEqSignal( bool manualReset = false );
	~CEqSignal();

	void	Raise();
	void	Clear();

	// Wait returns true if the object is in a signalled state and
	// returns false if the wait timed out. Wait also clears the signalled
	// state when the signalled state is reached within the time out period.
	bool	Wait(int timeout = WAIT_INFINITE);

private:
	SignalHandle_t	m_nHandle;

	CEqSignal( const CEqSignal & s ) = delete;
	void operator=( const CEqSignal & s ) = delete;
};

/*----------------------------------------------------------------------------------------
CEqThread is an abstract base class, to be extended by classes implementing the
CEqThread::Run() method.

	class CMyThread : public CEqThread {
	public:
		virtual int Run()
		{
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	CMyThread thread;
	thread.Start( "myThread" );

A worker thread is a thread that waits in place (without consuming CPU)
until work is available. A worker thread is implemented as normal, except that, instead of
calling the Start() method, the StartWorker() method is called to start the thread.
Note that the ThreadCreate function does not support the concept of worker threads.

	class CMyWorkerThread : public CMyThread
	{
	public:
		virtual int Run()
		{
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	CMyWorkerThread thread;
	thread.StartThread( "myWorkerThread" );

	// main thread loop
	for ( ; ; ) {
		// setup work for the thread here (by modifying class data on the thread)
		thread.SignalWork();           // kick in the worker thread
		// run other code in the main thread here (in parallel with the worker thread)
		thread.WaitForThread();        // wait for the worker thread to finish
		// use results from worker thread here
	}

In the above example, the thread does not continuously run in parallel with the main Thread,
but only for a certain period of time in a very controlled manner. Work is set up for the
Thread and then the thread is signalled to process that work while the main thread continues.
After doing other work, the main thread can wait for the worker thread to finish, if it has not
finished already. When the worker thread is done, the main thread can safely use the results
from the worker thread.

------------------------------------------------------------------------------------------
*/

class CEqThread
{
public:
					CEqThread();
	virtual			~CEqThread();

	const char *	GetName()				const { return m_szName.GetData(); }
	uintptr_t		GetThreadHandle()		const { return m_nThreadHandle; }
	uintptr_t		GetThreadID()			const;

	bool			IsWorker()				const { return m_bIsWorker; }
	bool			IsRunning()				const { return m_bIsRunning; }
	bool			IsTerminating()			const { return m_bIsTerminating; }

	// Thread Start/Stop/Wait
	bool			StartThread( const char* name, ThreadPriority_e priority = TP_NORMAL, int stackSize = DEFAULT_THREAD_STACK_SIZE );
	bool			StartWorkerThread( const char* name, ThreadPriority_e priority = TP_NORMAL, int stackSize = DEFAULT_THREAD_STACK_SIZE );

	void			StopThread( bool wait = true );

	void			WaitForThread(int timeout = WAIT_INFINITE);
	void			SignalWork();
	bool			IsWorkDone();

protected:
	// The routine that performs the work.
	virtual int		Run();

private:
	EqString		m_szName;
	uintptr_t		m_nThreadHandle{ 0 };

	bool			m_bIsWorker{ false };
	bool			m_bIsRunning{ false };

	volatile bool	m_bIsTerminating{ false };
	volatile bool	m_bMoreWorkToDo{ false };

	CEqSignal		m_SignalWorkerDone;
	CEqSignal		m_SignalMoreWorkToDo;
	CEqMutex		m_SignalMutex;

	static int		ThreadProc( CEqThread* thread );

	// you can't do any copy and assignment operations
					CEqThread( const CEqThread& s ) {}
	void			operator = ( const CEqThread& s ) {}
};

};
