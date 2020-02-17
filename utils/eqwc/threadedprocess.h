//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Threaded processing
//////////////////////////////////////////////////////////////////////////////////

#ifndef THREADEDPROCESS_H
#define THREADEDPROCESS_H

extern int	numthreads;

void	ThreadSetDefault(void);

// process arrays with threads
void	RunThreadsOn(int workcnt, bool showpacifier, void (*func) (int));
void	RunThreadsOnIndividual(int workcnt, bool showpacifier, void (*func) (int));

void	ThreadLock(void);
void	ThreadUnlock(void);
int		GetThreadWork(void);
void	ThreadWorkerFunction(int threadnum);

#endif // THREADEDPROCESS_H