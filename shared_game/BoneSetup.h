//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#ifndef BONESETUP_H
#define BONESETUP_H

#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "model.h"
#include "anim_activity.h"

#if !defined(NO_GAME) && !defined(NOENGINE)
#include "DataMap.h"
#endif // #if !EDITOR && !NOENGINE

struct gikchain_t;

struct giklink_t
{
	studioiklink_t*	l;

	Quaternion		quat;
	Vector3D		position;

	giklink_t*		parent;

	Matrix4x4		localTrans;
	Matrix4x4		absTrans;

	gikchain_t*		chain;
};


// used by baseanimating
struct gikchain_t
{
	char				name[64];
	bool				enable; // if false then it will be updated from FK

	Vector3D			local_target; // local target position

	giklink_t*			links;
	int					numLinks;
};

// pose controller
struct gposecontroller_t
{
	posecontroller_t*	p;
	float				value;
	float				interpolatedValue;
};

// translated game sequence
struct gsequence_t
{
	Activity			activity;
	gposecontroller_t*	posecontroller;
	sequencedesc_t*		s;

	sequenceevent_t*	events[MAX_EVENTS_PER_SEQ];
	gsequence_t*		blends[MAX_SEQUENCE_BLENDS];

	studioHwData_t::motionData_t::animation_t*	animations[MAX_BLEND_WIDTH];
};

// sequence timer with events
struct sequencetimer_t
{
	gsequence_t*				seq;
	int							seq_idx;
	float						seq_time;

	int							nextFrame;
	int							currFrame;

	float						playbackSpeedScale;

	bool						active;

	int							eventCounter;

	float						blendWeight;

	sequencetimer_t();

	void Reset();

	void AdvanceFrame(float fDt);

	void SetTime(float time);

	void ResetPlayback();

#ifdef DECLARE_DATAMAP
	DECLARE_DATAMAP();
#endif // NOENGINE
};

#endif // BONESETUP_H