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

#include "IEqModel.h"
#include "anim_activity.h"

#if !defined(EDITOR) && !defined(NO_GAME)
#include "DataMap.h"
#endif // EDITOR

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
	PPMEM_MANAGED_OBJECT()

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
	float						seq_time;

	int							nextFrame;
	int							currFrame;

	float						playbackSpeedScale;

	bool						bPlaying;

	int							eventCounter;

#if !defined(EDITOR) && !defined(NO_GAME)
	DECLARE_DATAMAP();
#endif // EDITOR

	sequencetimer_t();

	void Reset();

	void AdvanceFrame(float fDt);

	void SetTime(float time);

	void ResetPlayback();
};

inline Matrix4x4 CalculateLocalBonematrix(const animframe_t &frame)
{
	Matrix4x4 bonetransform(Quaternion(frame.angBoneAngles.x,frame.angBoneAngles.y,frame.angBoneAngles.z));
	//bonetransform.setRotation();
	bonetransform.setTranslation(frame.vecBonePosition);

	return bonetransform;
}

// computes blending animation index and normalized weight
inline void ComputeAnimationBlend(int numWeights, float blendrange[2], float blendValue, float &blendWeight, int &blendMainAnimation1, int &blendMainAnimation2)
{
	blendValue = clamp(blendValue, blendrange[0], blendrange[1]);

	// convert to value in range 0..1.
	float actualBlendValue = (blendValue - blendrange[0]) / (blendrange[1]-blendrange[0]);

	// compute animation index
	float normalizedBlend = actualBlendValue*(float)(numWeights-1);

	int blendMainAnimation = (int)normalizedBlend;

	blendWeight = normalizedBlend - ((int)normalizedBlend);

	int minAnim = blendMainAnimation;
	int maxAnim = blendMainAnimation+1;

	if( maxAnim > numWeights-1 )
	{
		maxAnim = numWeights-1;
		minAnim = numWeights-2;

		blendWeight = 1.0f;
	}

	if(minAnim < 0)
		minAnim = 0;

	blendMainAnimation1 = minAnim;
	blendMainAnimation2 = maxAnim;
}

// interpolates frame transform
inline void InterpolateFrameTransform(const animframe_t &frame1, const animframe_t &frame2, float value, animframe_t &out)
{
	Quaternion q1(frame1.angBoneAngles.x, frame1.angBoneAngles.y, frame1.angBoneAngles.z);
	Quaternion q2(frame2.angBoneAngles.x, frame2.angBoneAngles.y, frame2.angBoneAngles.z);

	Quaternion finQuat = slerp(q1, q2, value);

	out.angBoneAngles = eulers(finQuat);
	out.vecBonePosition = lerp(frame1.vecBonePosition, frame2.vecBonePosition, value);
}

// adds transform TODO: Quaternion rotation
inline void AddFrameTransform(const animframe_t &frame1, const animframe_t &frame2, animframe_t &out)
{
	out.angBoneAngles = frame1.angBoneAngles + frame2.angBoneAngles;
	out.vecBonePosition = frame1.vecBonePosition + frame2.vecBonePosition;
}

// adds and multiplies transform
inline void AddMultiplyFrameTransform(const animframe_t &frame1, const animframe_t &frame2, animframe_t &out)
{
	Quaternion q1(frame1.angBoneAngles.x, frame1.angBoneAngles.y, frame1.angBoneAngles.z);
	Quaternion q2(frame2.angBoneAngles.x, frame2.angBoneAngles.y, frame2.angBoneAngles.z);

	Quaternion finQuat = q1*q2;

	out.angBoneAngles = eulers(finQuat);

	out.vecBonePosition = frame1.vecBonePosition + frame2.vecBonePosition;
}

// zero frame
inline void ZeroFrameTransform(animframe_t &frame)
{
	frame.angBoneAngles = vec3_zero;
	frame.vecBonePosition = vec3_zero;
}

#endif // BONESETUP_H