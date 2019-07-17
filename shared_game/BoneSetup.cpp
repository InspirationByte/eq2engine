//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#include "BoneSetup.h"
#include "model.h"

sequencetimer_t::sequencetimer_t()
{
	active = false;
	seq = nullptr;
	seq_idx = -1;
	seq_time = 0.0f;
	currFrame = 0;
	nextFrame = 0;
	eventCounter = 0;
	playbackSpeedScale = 1.0f;
	blendWeight = 0.0f;
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

	sequencedesc_t* seqDesc = seq->s;

	seq_time = time;

	int numAnimationFrames = seq->animations[0]->bones[0].numFrames;
	float maxSeqTime = float(numAnimationFrames - 1);

	bool loop = (seqDesc->flags & SEQFLAG_LOOP) > 0;

	if (loop)
	{
		if (RollingValue(seq_time, 0.0f, float(numAnimationFrames - 1)))
		{
			eventCounter = 0;
		}

		currFrame = floor(seq_time);
		nextFrame = currFrame + 1;

		RollingValue(nextFrame, 0, numAnimationFrames - 1);
	}
	else
	{
		if (seq_time >= maxSeqTime)
		{
			seq_time = maxSeqTime;
			active = false;
		}
		else if (seq_time <= 0.0f)
		{
			active = false;
			seq_time = 0.0f;
		}

		currFrame = floor(seq_time);
		nextFrame = currFrame + 1;

		nextFrame = clamp(nextFrame, 0, numAnimationFrames - 1);
	}
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