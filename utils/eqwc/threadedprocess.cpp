//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Threaded processing
//////////////////////////////////////////////////////////////////////////////////

#include "Platform.h"
#include "DebugInterface.h"
#include "threadedprocess.h"
#include "cmd_pacifier.h"

#define	MAX_THREADS	64

int					dispatch;
int					workcount;
int					oldf;
bool				pacifier;

bool				threaded;

int					numthreads = -1;
CRITICAL_SECTION	crit;
static int			enter;

void ThreadSetDefault(void)
{
	SYSTEM_INFO     info;

	if(numthreads == -1)		// not set manually
	{
		GetSystemInfo(&info);
		numthreads = info.dwNumberOfProcessors;
		if(numthreads < 1 || numthreads > 32)
			numthreads = 1;
	}

	Msg("%i threads\n", numthreads);
}

void RunThreadsOn(int workcnt, bool showpacifier, void (*func) (int))
{
	int             threadid[MAX_THREADS];
	HANDLE          threadhandle[MAX_THREADS];
	int             i;
	int             start, end;

	start = Platform_GetCurrentTime();
	dispatch = 0;
	workcount = workcnt;
	oldf = -1;
	pacifier = showpacifier;
	threaded = true;

	//
	// run threads in parallel
	//
	InitializeCriticalSection(&crit);

	if(numthreads == 1)
	{							// use same thread
		func(0);
	}
	else
	{
		for(i = 0; i < numthreads; i++)
		{
			threadhandle[i] = CreateThread(NULL,	// LPSECURITY_ATTRIBUTES lpsa,
										   //0,     // DWORD cbStack,
										   /* ydnar: cranking stack size to eliminate radiosity crash with 1MB stack on win32 */
										   (4096 * 1024), (LPTHREAD_START_ROUTINE) func,	// LPTHREAD_START_ROUTINE lpStartAddr,
										   (LPVOID) i,	// LPVOID lpvThreadParm,
										   0,	//   DWORD fdwCreate,
										   (DWORD*)&threadid[i]);
		}

		for(i = 0; i < numthreads; i++)
			WaitForSingleObject(threadhandle[i], INFINITE);
	}
	DeleteCriticalSection(&crit);

	threaded = false;
	end = Platform_GetCurrentTime();

	if(pacifier)
		Msg(" (%i)\n", end - start);
}

void ThreadLock(void)
{
	if(!threaded)
		return;
	EnterCriticalSection(&crit);
	if(enter)
		MsgError("Recursive ThreadLock\n");
	enter = 1;
}

void ThreadUnlock(void)
{
	if(!threaded)
		return;
	if(!enter)
		MsgError("ThreadUnlock without lock\n");
	enter = 0;
	LeaveCriticalSection(&crit);
}

int GetThreadWork(void)
{
	int             r;
	float            f;

	ThreadLock();

	if(dispatch == workcount)
	{
		ThreadUnlock();
		return -1;
	}

	f = (float)dispatch / (float)workcount;

	if(pacifier)
	{
		UpdatePacifier(f);
		fflush(stdout);
	}

	/*
	if(f != oldf)
	{
		oldf = f;
		if(pacifier)
		{
			//Msg("%i...", f);
			UpdatePacifier(f);
			fflush(stdout);
		}
	}
	*/

	r = dispatch;
	dispatch++;
	ThreadUnlock();

	return r;
}

void            (*workfunction) (int);

void ThreadWorkerFunction(int threadnum)
{
	int             work;

	while(1)
	{
		work = GetThreadWork();
		if(work == -1)
			break;
//Msg ("thread %i, work %i\n", threadnum, work);
		workfunction(work);
	}
}

void RunThreadsOnIndividual(int workcnt, bool showpacifier, void (*func) (int))
{
	if(numthreads == -1)
		ThreadSetDefault();
	workfunction = func;
	RunThreadsOn(workcnt, showpacifier, ThreadWorkerFunction);
}