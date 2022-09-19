//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "BoneSetup.h"

qanimframe_t::qanimframe_t(animframe_t& frame)
{
	angBoneAngles = Quaternion(frame.angBoneAngles.x, frame.angBoneAngles.y, frame.angBoneAngles.z);
	vecBonePosition = frame.vecBonePosition;
	pad = 0.0f;
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

void sequencetimer_t::AdvanceFrame(float fDt)
{
	if(!seq)
		return;

	sequencedesc_t* seqDesc = seq->s;

	if (!active)
	{
		SetTime(seq_time);
		return;
	}

	float frame_time = fDt * playbackSpeedScale * seqDesc->framerate;
	SetTime(seq_time + frame_time);
}

void sequencetimer_t::SetTime(float time)
{
	if(!seq)
		return;

	const sequencedesc_t* seqDesc = seq->s;

	const int numAnimationFrames = seq->animations[0]->bones[0].numFrames;
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

	seq_time = time;
}

void sequencetimer_t::Reset()
{
	active = false;
	seq = nullptr;
	seq_idx = -1;
	blendWeight = 0.0f;

	playbackSpeedScale = 1.0f;

	ResetPlayback();
}

void sequencetimer_t::ResetPlayback()
{
	seq_time = 0.0f;
	blendWeight = 0.0f;
	eventCounter = 0;
	nextFrame = 0;
	currFrame = 0;
}