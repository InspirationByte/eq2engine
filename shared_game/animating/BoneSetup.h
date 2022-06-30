//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"
#include "anim_activity.h"

typedef struct modelheader_s studiohdr_t;
typedef struct animframe_s animframe_t;
typedef struct sequencedesc_s sequencedesc_t;
typedef struct sequenceevent_s sequenceevent_t;;
typedef struct posecontroller_s posecontroller_t;

struct qanimframe_t
{
	qanimframe_t() = default;
	qanimframe_t(animframe_t& frame);

	Quaternion	angBoneAngles{ identity() };
	Vector3D	vecBonePosition{ vec3_zero };
	float		pad{ 0.0f };
};

struct gikchain_t;
typedef struct studioiklink_s studioiklink_t;

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
	~gikchain_t() 
	{
		delete[] links;
	}

	char				name[64];
	bool				enable; // if false then it will be updated from FK

	Vector3D			local_target; // local target position

	giklink_t*			links { nullptr };
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

	studioAnimation_t*	animations[MAX_BLEND_WIDTH];
};

// sequence timer with events
struct sequencetimer_t
{
	gsequence_t* seq{ nullptr };
	int			 seq_idx{ -1 };
	float		 seq_time{ 0.0f };
				 
	int			 nextFrame{ 0 };
	int			 currFrame{ 0 };
	int			 eventCounter{ 0 };

	float		 playbackSpeedScale{ 1.0f };
	float		 blendWeight{ 0.0f };
	bool		 active{ false };

	void Reset();
	void ResetPlayback();

	void AdvanceFrame(float fDt);
	void SetTime(float time);

#ifdef DECLARE_DATAMAP
	DECLARE_DATAMAP();
#endif // NOENGINE
};
