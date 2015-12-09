//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: AI Behavior base and definitions
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include "BaseActor.h"

enum TaskType_e
{
	TASK_INVALID = -1,

	TASK_MOVEMENT_TO_TARGET = 0,	// movement by path
	TASK_MOVEMENT_TO_ENEMY,			// movement to enemy
	TASK_MOVEMENT_TO_PLAYER,		// movement to player
	TASK_WAIT,						// wait some times
	TASK_WAITPVS,					// waiting for visibility
	TASK_DEFEND,					// defending a point from potential enemies
	TASK_RANGE_ATTACK,				// attacking something
	TASK_GETOFF,					// get out from target
	TASK_FACE_PLAYER,
	TASK_FACE_ENEMY,				// face player!
	TASK_RELOAD,					// reload weapon
	TASK_GETCOVER,					// get cover (reload/protection when low health)

	TASK_LAST,						// the last task to add custom
};

enum TaskFailure_e
{
	TASKFAIL_NO_FAIL = -1,

	TASKFAIL_NO_TARGET = 0,
	TASKFAIL_NO_ENEMY,
	TASKFAIL_NO_PLAYER,
	TASKFAIL_NO_ROUTE,
	TASKFAIL_NO_ROUTE_GOAL,
};

enum TaskStatus_e 
{
	TASKSTATUS_NEW = 0,			// Just started
	TASKSTATUS_RUN,				// Is running
	TASKSTATUS_COMPLETE,		// task was completed
};

enum AIVisibility_e
{
	AIVIS_VISIBLE = 0,			// visible, in view cone
	AIVIS_BEHINDVIEW,			// behind view
	AIVIS_OCCLUDED,				// in invisible room or out from portal planes, or just failed to trace
};

enum Condtion_e
{
	COND_HAS_ENEMY = 0,
	COND_HAS_TARGET,
	COND_HAS_WEAPON,
	COND_HAS_COVER,
	COND_NEEDS_WEAPON_RELOAD,

	COND_COUNT,
};

// embedded structure
struct AITask_t
{
	int				task_type;
	float			fTime;
	TaskStatus_e	status;
};

class CAIBaseNPCHuman;

#define MAX_CONDITION_BYTES 32

// basic behavior
//template< class BASE_NPC >
class CBasicBehavior
{
public:
	CBasicBehavior();

	virtual int			RunTask( AITask_t* task );
	virtual void		TaskFail( AITask_t* task );
	virtual void		StartTask( AITask_t* task );
	virtual int			SelectTask( AITask_t* oldtask );

	void				RunBehavior();

	//virtual bool		NeedsChangeBehavior();

	CAIBaseNPCHuman*	GetOuter();
	void				SetOuter(CAIBaseNPCHuman* pActor);

	bool				IsEnabled();
	void				Enable();
	void				Disable();

private:
	void				NewTask(TaskType_e task_type, float fDelay);

protected:
	DkList<AITask_t>	m_Tasks;

	CAIBaseNPCHuman*	m_pOuter;

	bool				m_bEnabled;
};

#endif // BEHAVIOR_H