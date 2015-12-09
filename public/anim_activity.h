//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Activities for animations and AI
//////////////////////////////////////////////////////////////////////////////////

#ifndef ANIM_ACTIVITY_H
#define ANIM_ACTIVITY_H

typedef enum ACTIVITY_E
{
	ACT_INVALID = -1,

	ACT_RAGDOLL,

	// basic
	ACT_IDLE,
	ACT_WALK,
	ACT_RUN,

	ACT_FLY,
	ACT_FLY_WINGS,

	// for next crouch and weapon animations TranslateActivity is used
	ACT_IDLE_COMBATSTATE,

	ACT_IDLE_WPN,
	ACT_WALK_WPN,
	ACT_RUN_WPN,

	ACT_IDLE_CROUCH,
	ACT_WALK_CROUCH,
	ACT_RUN_CROUCH,

	ACT_IDLE_CROUCH_WPN,
	ACT_WALK_CROUCH_WPN,
	ACT_RUN_CROUCH_WPN,

	ACT_JUMP,
	ACT_JUMP_WPN,

	// Weapon viewmodel activities
	ACT_VM_DEPLOY,
	ACT_VM_IDLE,
	ACT_VM_PRIMARYATTACK,
	ACT_VM_PRIMARYATTACK_EMPTY,
	ACT_VM_RELOAD_FULL,
	ACT_VM_RELOAD_SHORT,
	ACT_VM_MELEE,
	ACT_VM_SPRINT,
	ACT_VM_AIM_IDLE,
	ACT_VM_AIM_SHOOT,

	// grenade (VM and TP)
	ACT_GRN_PULLPIN,
	ACT_GRN_THROW,

	// weapon animations for third person (DONT USE IT FOR MOTION PACKAGES - THEY ARE UNREGISTERED)
	// use weapon class, TranslateActorActivity function
	ACT_WPN_DEPLOY,					// weapon deployment
	ACT_WPN_IDLE,					// inactive idle
	ACT_WPN_IDLE_COMBATSTATE,		// combat idle
	ACT_WPN_AIM_IDLE,				// aiming combat idle
	ACT_WPN_IDLE_MOVEMENT,			// combat movement idle
	ACT_WPN_PRIMARYATTACK,			// primary attack
	ACT_WPN_AIM_PRIMARYATTACK,		// aiming primary attack
	ACT_WPN_SECONDARYATTACK,		// secondary attack
	ACT_WPN_RELOAD,					// reloading weapon

#ifdef STDGAME

	// third person animations
	ACT_AR_IDLE,
	ACT_AR_IDLE_COMBATSTATE,
	ACT_AR_IDLE_MOVEMENT,
	ACT_AR_SHOOT,
	ACT_AR_RELOAD,

#endif // STDGAME

	ACT_COUNT,
}Activity;



Activity GetActivityByName(const char* name);
const char* GetActivityName(Activity act);

#endif