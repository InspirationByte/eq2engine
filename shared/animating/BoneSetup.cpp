//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "BoneSetup.h"

AnimFrame::AnimFrame(const animframe_t& frame)
{
	angBoneAngles = rotateXYZ(frame.angBoneAngles.x, frame.angBoneAngles.y, frame.angBoneAngles.z);
	vecBonePosition = frame.vecBonePosition;
	pad = 0.0f;
}

AnimIkChain::~AnimIkChain()
{
	if(links.ptr())
		PPDeleteArrayRef(links);
}

template <typename T>
bool RollingValue(T& inout, T min, T max)
{
	T old = inout;
	T move = max-min;
	if (inout < min) inout += move;
	if (inout > max) inout -= move;

	return old != inout;
}

void AnimSequenceTimer::AdvanceFrame(float fDt)
{
	if(!seq)
		return;

	if (!active)
	{
		SetTime(seqTime);
		return;
	}

	const sequencedesc_t* seqDesc = seq->desc;
	const float timeDelta = fDt * playbackSpeedScale * seqDesc->framerate;
	SetTime(seqTime + timeDelta);
}

void AnimSequenceTimer::SetTime(float time)
{
	if(!seq)
		return;

	const sequencedesc_t* seqDesc = seq->desc;

	const int numAnimationFrames = seq->animations[0]->numFrames;
	const bool loop = (seqDesc->flags & SEQFLAG_LOOP);

	if (loop)
	{
		if (RollingValue(time, 0.0f, float(numAnimationFrames - 1)))
		{
			eventCounter = 0;
		}

		currFrame = int(floor(time)) % numAnimationFrames;
		nextFrame = (currFrame + 1) % numAnimationFrames;
	}
	else
	{
		if (time >= numAnimationFrames)
		{
			time = numAnimationFrames;
			active = false;
		}
		else if (time <= 0.0f)
		{
			active = false;
			time = 0.0f;
		}

		currFrame = min(int(floor(time)), numAnimationFrames - 1);
		nextFrame = min(currFrame + 1, numAnimationFrames - 1);
	}

	seqTime = time;
}

void AnimSequenceTimer::Reset()
{
	seq = nullptr;
	seqId = -1;

	active = false;
	blendWeight = 0.0f;

	playbackSpeedScale = 1.0f;
	transitionRemainingTime = 0.0f;

	ResetPlayback();
}

void AnimSequenceTimer::ResetPlayback()
{
	transitionRemainingTime = 0.0f;
	seqTime = 0.0f;
	nextFrame = 0;
	currFrame = 0;

	blendWeight = 0.0f;
	eventCounter = 0;
}