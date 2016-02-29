//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine threads
//////////////////////////////////////////////////////////////////////////////////

#include "eqthread.h"
#include "DebugInterface.h"
#include "platform/MessageBox.h"

namespace Threading
{

#ifdef _WIN32

#define MS_VC_EXCEPTION 0x406D1388

typedef struct tagTHREADNAME_INFO {
	DWORD dwType;		// Must be 0x1000.
	LPCSTR szName;		// Pointer to name (in user addr space).
	DWORD dwThreadID;	// Thread ID (-1=caller thread).
	DWORD dwFlags;		// Reserved for future use, must be zero.
} THREADNAME_INFO;

void SetThreadName( DWORD threadID, const char * name )
{
	THREADNAME_INFO info;

	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadID;
	info.dwFlags = 0;

	__try {
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), (const ULONG_PTR *)&info );
	}
	// this much is just to keep /analyze quiet
	__except( GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
		info.dwFlags = 0;
	}
}

uintptr_t ThreadCreate( threadfunc_t fnThread, void* pThreadParams, ThreadPriority_e nPriority,
								  const char* pszThreadName, int nStackSize, bool bSuspended )
{
	DWORD flags = ( bSuspended ? CREATE_SUSPENDED : 0 );
	// Without this flag the 'dwStackSize' parameter to CreateThread specifies the "Stack Commit Size"
	// and the "Stack Reserve Size" is set to the value specified at link-time.
	// With this flag the 'dwStackSize' parameter to CreateThread specifies the "Stack Reserve Size"
	// and the “Stack Commit Size” is set to the value specified at link-time.
	// For various reasons (some of which historic) we reserve a large amount of stack space in the
	// project settings. By setting this flag and by specifying 64 kB for the "Stack Commit Size" in
	// the project settings we can create new threads with a much smaller reserved (and committed)
	// stack space. It is very important that the "Stack Commit Size" is set to a small value in
	// the project settings. If it is set to a large value we may be both reserving and committing
	// a lot of memory by setting the STACK_SIZE_PARAM_IS_A_RESERVATION flag. There are some
	// 50 threads allocated for normal game play. If, for instance, the commit size is set to 16 MB
	// then by adding this flag we would be reserving and committing 50 x 16 = 800 MB of memory.
	// On the other hand, if this flag is not set and the "Stack Reserve Size" is set to 16 MB in the
	// project settings, then we would still be reserving 50 x 16 = 800 MB of virtual address space.
	flags |= STACK_SIZE_PARAM_IS_A_RESERVATION;

	DWORD threadId;
	HANDLE handle = CreateThread(	NULL,	// LPSECURITY_ATTRIBUTES lpsa, //-V513
									nStackSize,
									(LPTHREAD_START_ROUTINE)fnThread,
									pThreadParams,
									flags,
									&threadId);
	if ( handle == 0 )
	{
		MsgError( "CreateThread error: %i", GetLastError() );
		return (uintptr_t)0;
	}

	SetThreadName( threadId, pszThreadName );

	if( nPriority == TP_HIGHEST )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_HIGHEST );		//  we better sleep enough to do this
	}
	else if( nPriority == TP_ABOVE_NORMAL )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_ABOVE_NORMAL );
	}
	else if( nPriority == TP_BELOW_NORMAL )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_BELOW_NORMAL );
	}
	else if ( nPriority == TP_LOWEST )
	{
		SetThreadPriority( (HANDLE)handle, THREAD_PRIORITY_LOWEST );
	}

	// Under Windows, we don't set the thread affinity and let the OS deal with scheduling

	return (uintptr_t)handle;
}

uintptr_t GetCurrentThreadID()
{
	return GetCurrentThreadId();
}

uintptr_t ThreadGetID(uintptr_t handle)
{
	return (uintptr_t)GetThreadId((HANDLE)handle);
}

void ThreadDestroy( uintptr_t threadHandle )
{
	if ( threadHandle == 0 )
	{
		return;
	}

	WaitForSingleObject( (HANDLE)threadHandle, INFINITE );
	CloseHandle( (HANDLE)threadHandle );
}

void SetCurrentThreadName( const char *name )
{
	SetThreadName(GetCurrentThreadId(), name);
}

void Yield()
{
	SwitchToThread();
}

void SignalCreate( SignalHandle_t& handle, bool bManualReset )
{
	handle = CreateEvent( NULL, bManualReset, FALSE, NULL );
}

void SignalDestroy( SignalHandle_t& handle )
{
	CloseHandle( handle );
}

void SignalRaise( SignalHandle_t& handle )
{
	SetEvent( handle );
}

void SignalClear( SignalHandle_t& handle )
{
	// events are created as auto-reset so this should never be needed
	ResetEvent( handle );
}

bool SignalWait( SignalHandle_t& handle, int nTimeout )
{
	if(handle == 0)
		return true;

	DWORD result = WaitForSingleObject( handle, nTimeout );
	ASSERT( result == WAIT_OBJECT_0 || ( nTimeout != INFINITE && result == WAIT_TIMEOUT ) );
	return ( result == WAIT_OBJECT_0 );
}

void MutexCreate( MutexHandle_t& handle )
{
	InitializeCriticalSection( &handle );
}

void MutexDestroy( MutexHandle_t& handle )
{
	DeleteCriticalSection( &handle );
}

bool MutexLock( MutexHandle_t& handle, bool bBlocking )
{
	if ( TryEnterCriticalSection( &handle ) == 0 )
	{
		if ( !bBlocking )
			return false;

		EnterCriticalSection( &handle );
	}

	return true;
}

void MutexUnlock( MutexHandle_t& handle )
{
	LeaveCriticalSection( &handle );
}

InterlockedInt_t IncrementInterlocked( InterlockedInt_t& value )
{
	return InterlockedIncrementAcquire( &value );
}

InterlockedInt_t DecrementInterlocked( InterlockedInt_t& value )
{
	return InterlockedDecrementAcquire( &value );
}

InterlockedInt_t AddInterlocked( InterlockedInt_t& value, InterlockedInt_t i )
{
	return InterlockedExchangeAdd( &value, i ) + i;
}

InterlockedInt_t SubtractInterlocked( InterlockedInt_t& value, InterlockedInt_t i )
{
	return InterlockedExchangeAdd( &value, - i ) - i;
}

InterlockedInt_t ExchangeInterlocked( InterlockedInt_t& value, InterlockedInt_t exchange )
{
	return InterlockedExchange( &value, exchange );
}

InterlockedInt_t CompareExchangeInterlocked( InterlockedInt_t& value, InterlockedInt_t comparand, InterlockedInt_t exchange )
{
	return InterlockedCompareExchange( &value, exchange, comparand );
}

#else

#ifdef PLAT_POSIX

#else // APPLE / BSD / DROID?
// TODO:
#endif

typedef void* ( *pthread_function_t )( void* );

uintptr_t ThreadCreate( threadfunc_t fnThread, void* pThreadParams, ThreadPriority_e nPriority,
								  const char* pszThreadName, int nStackSize, bool bSuspended )
{
	pthread_attr_t attr;
	pthread_attr_init( &attr );

	if( pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) != 0 )
	{
		ErrorMsg( "ERROR: pthread_attr_setdetachstate %s failed\n", pszThreadName );
		return ( uintptr_t )0;
	}

	pthread_t handle;
	if( pthread_create( ( pthread_t* )&handle, &attr, ( pthread_function_t )fnThread, pThreadParams ) != 0 )
	{
		ErrorMsg( "ERROR: pthread_create %s failed\n", pszThreadName );
		return ( uintptr_t )0;
	}

	pthread_attr_destroy( &attr );

	return ( uintptr_t )handle;
}

uintptr_t GetCurrentThreadID()
{
	return ( uintptr_t )pthread_self();
}

uintptr_t ThreadGetID(uintptr_t threadHandle)
{
    return threadHandle;
}

void ThreadDestroy( uintptr_t threadHandle )
{
	if( threadHandle == 0 )
	{
		return;
	}

	char	name[128];
	name[0] = '\0';

#if defined(DEBUG_THREADS)
	Sys_GetThreadName( ( pthread_t )threadHandle, name, sizeof( name ) );
#endif

#if 0 //!defined(__ANDROID__)
	if( pthread_cancel( ( pthread_t )threadHandle ) != 0 )
	{
		ErrorMsg( "ERROR: pthread_cancel %s failed\n", name );
	}
#endif

	if( pthread_join( ( pthread_t )threadHandle, NULL ) != 0 )
	{
		ErrorMsg( "ERROR: pthread_join %s failed\n", name );
	}
}


void SetCurrentThreadName( const char *name )
{
	//SetThreadName(GetCurrentThreadId(), name);

}

void Yield()
{
#if defined(__ANDROID__) || defined(__APPLE__)
	sched_yield();
#else
	pthread_yield();
#endif
}

void SignalCreate( SignalHandle_t& handle, bool bManualReset )
{
	handle.manualReset = bManualReset;

	// if this is true, the signal is only set to nonsignaled when Clear() is called,
	// else it's "auto-reset" and the state is set to !signaled after a single waiting
	// thread has been released

	// the inital state is always "not signaled"
	handle.signaled = false;
	handle.waiting = 0;

	pthread_mutex_init( &handle.mutex, NULL );

	pthread_cond_init( &handle.cond, NULL );
}

void SignalDestroy( SignalHandle_t& handle )
{
	handle.signaled = false;
	handle.waiting = 0;
	pthread_mutex_destroy( &handle.mutex );
	pthread_cond_destroy( &handle.cond );
}

void SignalRaise( SignalHandle_t& handle )
{
	pthread_mutex_lock( &handle.mutex );

	if( handle.manualReset )
	{
		// signaled until reset
		handle.signaled = true;
		// wake *all* threads waiting on this cond
		pthread_cond_broadcast( &handle.cond );
	}
	else
	{
		// automode: signaled until first thread is released
		if( handle.waiting > 0 )
		{
			// there are waiting threads => release one
			pthread_cond_signal( &handle.cond );
		}
		else
		{
			// no waiting threads, save signal
			handle.signaled = true;
			// while the MSDN documentation is a bit unspecific about what happens
			// when SetEvent() is called n times without a wait inbetween
			// (will only one wait be successful afterwards or n waits?)
			// it seems like the signaled state is a flag, not a counter.
			// http://stackoverflow.com/a/13703585 claims the same.
		}
	}

	pthread_mutex_unlock( &handle.mutex );
}

void SignalClear( SignalHandle_t& handle )
{
	// events are created as auto-reset so this should never be needed
	pthread_mutex_lock( &handle.mutex );

	// TODO: probably signaled could be atomically changed?
	handle.signaled = false;

	pthread_mutex_unlock( &handle.mutex );
}

bool SignalWait( SignalHandle_t& handle, int nTimeout )
{
	int status;
	pthread_mutex_lock( &handle.mutex );

	if( handle.signaled ) // there is a signal that hasn't been used yet
	{
		if( ! handle.manualReset ) // for auto-mode only one thread may be released - this one.
			handle.signaled = false;

		status = 0; // success!
	}
	else // we'll have to wait for a signal
	{
		++handle.waiting;
		if( nTimeout == WAIT_INFINITE )
		{
			status = pthread_cond_wait( &handle.cond, &handle.mutex );
		}
		else
		{
			timespec ts;

			clock_gettime( CLOCK_REALTIME, &ts );

			// DG: handle timeouts > 1s better
			ts.tv_nsec += ( nTimeout % 1000 ) * 1000000; // millisec to nanosec
			ts.tv_sec  += nTimeout / 1000;
			if( ts.tv_nsec >= 1000000000 ) // nanoseconds are more than one second
			{
				ts.tv_nsec -= 1000000000; // remove one second in nanoseconds
				ts.tv_sec += 1; // add one second to seconds
			}
			// DG end
			status = pthread_cond_timedwait( &handle.cond, &handle.mutex, &ts );
		}
		--handle.waiting;
	}

	pthread_mutex_unlock( &handle.mutex );

	ASSERT( status == 0 || ( nTimeout != WAIT_INFINITE && status == ETIMEDOUT ) );

	return ( status == 0 );
}

void MutexCreate( MutexHandle_t& handle )
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
	pthread_mutex_init( &handle, &attr );

	pthread_mutexattr_destroy( &attr );
}

void MutexDestroy( MutexHandle_t& handle )
{
	pthread_mutex_destroy( &handle );
}

bool MutexLock( MutexHandle_t& handle, bool bBlocking )
{
	if( pthread_mutex_trylock( &handle ) != 0 )
	{
		if( !bBlocking )
		{
			return false;
		}
		pthread_mutex_lock( &handle );
	}

	return true;
}

void MutexUnlock( MutexHandle_t& handle )
{
	pthread_mutex_unlock( & handle );
}

InterlockedInt_t IncrementInterlocked( InterlockedInt_t& value )
{
	return __sync_add_and_fetch( &value, 1 );
}

InterlockedInt_t DecrementInterlocked( InterlockedInt_t& value )
{
	return __sync_sub_and_fetch( &value, 1 );
}

InterlockedInt_t AddInterlocked( InterlockedInt_t& value, InterlockedInt_t i )
{
	return __sync_add_and_fetch( &value, i );
}

InterlockedInt_t SubtractInterlocked( InterlockedInt_t& value, InterlockedInt_t i )
{
	return __sync_sub_and_fetch( &value, - i );
}

InterlockedInt_t ExchangeInterlocked( InterlockedInt_t& value, InterlockedInt_t exchange )
{
	return __sync_val_compare_and_swap( &value, value, exchange );
}

InterlockedInt_t CompareExchangeInterlocked( InterlockedInt_t& value, InterlockedInt_t comparand, InterlockedInt_t exchange )
{
	return __sync_val_compare_and_swap( &value, comparand, exchange );
}

#endif // _WIN32

//-------------------------------------------------------------------------------------------------------------------------

CEqThread::CEqThread() : m_SignalWorkerDone(true)
{
	m_nThreadHandle = 0;
	m_bIsWorker = false;
	m_bIsRunning = false;
	m_bIsTerminating = false;
	m_bMoreWorkToDo = false;
}

CEqThread::~CEqThread()
{
	StopThread( true );

	if ( m_nThreadHandle )
	{
		ThreadDestroy( m_nThreadHandle );
	}
}


bool CEqThread::StartThread( const char * name, ThreadPriority_e priority, int stackSize )
{
	if ( m_bIsRunning )
		return false;

	m_szName = name;

	m_bIsTerminating = false;

	if ( m_nThreadHandle )
		ThreadDestroy( m_nThreadHandle );

	m_nThreadHandle = ThreadCreate( (threadfunc_t)ThreadProc, this, priority, name, stackSize, false );

	m_bIsRunning = true;
	return true;
}

bool CEqThread::StartWorkerThread( const char * name_, ThreadPriority_e priority, int stackSize )
{
	if ( m_bIsRunning )
		return false;

	m_bIsWorker = true;

	bool result = StartThread( name_, priority, stackSize );

	m_SignalWorkerDone.Wait();

	return result;
}

void CEqThread::StopThread( bool wait )
{
	if ( !m_bIsRunning )
		return;

	if ( m_bIsWorker )
	{
		m_SignalMutex.Lock();

		m_bMoreWorkToDo = true;

		m_SignalWorkerDone.Clear();

		m_bIsTerminating = true;

		m_SignalMoreWorkToDo.Raise();

		m_SignalMutex.Unlock();
	}
	else
		m_bIsTerminating = true;

	if ( wait )
		WaitForThread();
}

void CEqThread::WaitForThread()
{
	if ( m_bIsWorker )
	{
		m_SignalWorkerDone.Wait();
	}
	else if ( m_bIsRunning )
	{
		ThreadDestroy( m_nThreadHandle );
		m_nThreadHandle = 0;
	}
}

void CEqThread::SignalWork()
{
	if ( m_bIsWorker )
	{
		m_SignalMutex.Lock();

		m_bMoreWorkToDo = true;

		m_SignalWorkerDone.Clear();

		m_SignalMoreWorkToDo.Raise();

		m_SignalMutex.Unlock();
	}
}

/*
========================
idSysThread::IsWorkDone
========================
*/
bool CEqThread::IsWorkDone()
{
	if ( m_bIsWorker )
	{
		// a timeout of 0 will return immediately with true if signaled
		if ( m_SignalWorkerDone.Wait( 0 ) )
			return true;
	}

	return false;
}

/*
========================
idSysThread::ThreadProc
========================
*/
int CEqThread::ThreadProc( CEqThread* thread )
{
	int retVal = 0;

	try
	{
		if ( thread->m_bIsWorker )
		{
			for( ; ; )
			{
				thread->m_SignalMutex.Lock();

				if ( thread->m_bMoreWorkToDo )
				{
					thread->m_bMoreWorkToDo = false;

					thread->m_SignalMoreWorkToDo.Clear();
					thread->m_SignalMutex.Unlock();
				}
				else
				{
					thread->m_SignalWorkerDone.Raise();
					thread->m_SignalMutex.Unlock();

					thread->m_SignalMoreWorkToDo.Wait();
					continue;
				}

				if ( thread->m_bIsTerminating )
					break;

				retVal = thread->Run();
			}

			thread->m_SignalWorkerDone.Raise();
		}
		else
			retVal = thread->Run();
	}
	catch ( CEqException& ex )
	{
		MsgWarning( "Thread '%s' exception: %s", thread->GetName(), ex.GetErrorString() );

		// We don't handle threads terminating unexpectedly very well, so just terminate the whole process
#ifdef _WIN32
		_exit( 0 );
#else
		exit( 0 );
#endif // _WIN32
	}

	thread->m_bIsRunning = false;

	return retVal;
}

int CEqThread::Run()
{
	// The Run() is not pure virtual because on destruction of a derived class
	// the virtual function pointer will be set to NULL before the idSysThread
	// destructor actually stops the thread.

	return 0;
}

};
