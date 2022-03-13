//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqEngine mutex storage
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQGLOBALMUTEX_H
#define EQGLOBALMUTEX_H

#include "utils/eqthread.h"


enum EMutexPurpose
{
	MUTEXPURPOSE_JOBMANAGER = 0,	// global locking
	MUTEXPURPOSE_LOG,
	MUTEXPURPOSE_NET_THREAD,		// network thread uses
	MUTEXPURPOSE_MODEL_LOADER,		// model loading
	MUTEXPURPOSE_LEVEL_LOADER,		// level loading
	MUTEXPURPOSE_PHYSICS,			// physics engine
	MUTEXPURPOSE_RENDERER,			// rendering
	MUTEXPURPOSE_PARTICLES,			// particle system
	MUTEXPURPOSE_GAME,				// game thread locking
	MUTEXPURPOSE_AUDIO,

	MUTEXPURPOSE_USED,
};

// TODO: interface

Threading::CEqMutex& GetGlobalMutex( EMutexPurpose purpose );

#endif // EQGLOBALMUTEX_H