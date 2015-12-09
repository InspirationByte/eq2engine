//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Task types
//////////////////////////////////////////////////////////////////////////////////

#ifndef AITASKTYPES_H
#define AITASKTYPES_H

// task/action type
enum AITaskActionType_e
{
	AITASK_INVALID = -1,
	AITASK_SCRIPT,			// super, being controlled by script
	AITASK_WAIT,			// wait only for interrupt
	AITASK_IDLE_WAIT_PVS,	// idle wait for PVS
	AITASK_MOVEMENTTARGET,
};

// the real goal of task that AI only knows....
enum AITaskGoal_e
{
	AIGOAL_NONE = 0,	// this makes AI doing something without cause. Just because.

	AIGOAL_KILL,		// makes AI to kill target

	AIGOAL_SCRIPT_GOAL,	// by interrupt

	AIGOAL_COUNT,
};

// TODO: registrator, that will be used for saved games and Editor's key-values

#endif // AITASKTYPES_H