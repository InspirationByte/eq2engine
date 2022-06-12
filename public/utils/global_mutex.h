//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqEngine mutex storage
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EMutexPurpose
{
	MUTEXPURPOSE_JOBMANAGER = 0,	// global locking
	MUTEXPURPOSE_LOG,
	MUTEXPURPOSE_NET_THREAD,		// network thread uses
	MUTEXPURPOSE_MODEL_LOADER,		// model loading
	MUTEXPURPOSE_LEVEL_LOADER,		// level loading
	MUTEXPURPOSE_AI_NAVIGATION,		// AI navigation
	MUTEXPURPOSE_PHYSICS,			// physics engine
	MUTEXPURPOSE_RENDERER,			// rendering
	MUTEXPURPOSE_PARTICLES,			// particle system
	MUTEXPURPOSE_GAME,				// game thread locking
	MUTEXPURPOSE_AUDIO,

	MUTEXPURPOSE_USED,
};

// TODO: interface

Threading::CEqMutex& GetGlobalMutex( EMutexPurpose purpose );