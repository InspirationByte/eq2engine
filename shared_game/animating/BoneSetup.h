//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"
#include "anim_activity.h"

typedef struct studioModelHeader_s studioHdr_t;
typedef struct animframe_s animframe_t;
typedef struct sequencedesc_s sequencedesc_t;
typedef struct sequenceevent_s sequenceevent_t;;
typedef struct posecontroller_s posecontroller_t;

struct qanimframe_t
{
	qanimframe_t() = default;
	qanimframe_t(animframe_t& frame);

	Quaternion	angBoneAngles{ qidentity };
	Vector3D	vecBonePosition{ vec3_zero };
	float		pad{ 0.0f };
};

struct gikchain_t;
typedef struct studioIkLink_s studioIkLink_t;

struct giklink_t
{
	studioIkLink_t* l{ nullptr };
	giklink_t*		parent{ nullptr };
	gikchain_t*		chain{ nullptr };

	Matrix4x4		localTrans{ identity4 };
	Matrix4x4		absTrans{ identity4 };
	Quaternion		quat{ qidentity };
	Vector3D		position{ vec3_zero };
};


// used by baseanimating
struct gikchain_t
{
	~gikchain_t() 
	{
		delete[] links;
	}
	const studioIkChain_t*	c{ nullptr };
	Vector3D				localTarget{ vec3_zero };
	giklink_t*				links { nullptr };
	int						numLinks{ 0 };
	bool					enable{ false }; // if false then it will be updated from FK
};

// pose controller
struct gposecontroller_t
{
	const posecontroller_t*	p{ nullptr };
	float				value{ 0.0f };
	float				interpolatedValue{ 0.0f };
};

// translated game sequence
struct gsequence_t
{
	Activity					activity{ ACT_INVALID };
	gposecontroller_t*			posecontroller{ nullptr };
	const sequencedesc_t*		s{ nullptr };

	gsequence_t*				blends[MAX_SEQUENCE_BLENDS]{ nullptr };
	const sequenceevent_t*		events[MAX_EVENTS_PER_SEQ]{ nullptr };
	const studioAnimation_t*	animations[MAX_BLEND_WIDTH]{ nullptr };
};

// sequence timer with events
struct sequencetimer_t
{
	gsequence_t* seq{ nullptr };
	int			 seqIdx{ -1 };
	float		 seqTime{ 0.0f };
				 
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
