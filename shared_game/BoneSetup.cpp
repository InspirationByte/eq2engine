//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#include "BoneSetup.h"

#if !defined(EDITOR) && !defined(NO_GAME)
BEGIN_DATAMAP_NO_BASE(sequencetimer_t)
	DEFINE_FIELD(seq_time, VTYPE_FLOAT),
	DEFINE_FIELD(nextFrame, VTYPE_INTEGER),
	DEFINE_FIELD(currFrame, VTYPE_INTEGER),
	DEFINE_FIELD(playbackSpeedScale, VTYPE_FLOAT),
	DEFINE_FIELD(bPlaying, VTYPE_BOOLEAN),
END_DATAMAP()
#endif // EDITOR

sequencetimer_t::sequencetimer_t()
{
	bPlaying = false;
	seq = nullptr;
	seq_time = 0.0f;
	currFrame = 0;
	nextFrame = 0;
	eventCounter = 0;
	playbackSpeedScale = 1.0f;
}

void sequencetimer_t::AdvanceFrame(float fDt)
{
	if(!seq)
		return;

	if(!bPlaying)
		return;

	sequencedesc_t* seqDesc = seq->s;
	int numAnimationFrames = seq->animations[0]->bones[0].numFrames;
	bool loop = (seqDesc->flags & SEQFLAG_LOOP) > 0;

	float frame_time = fDt * playbackSpeedScale * seqDesc->framerate;

	int oldFrame = currFrame;

	// set new sequence time
	seq_time = seq_time+frame_time;
	currFrame = floor(seq_time);

	if (currFrame >= numAnimationFrames)
	{
		currFrame--;

		if (loop)
			ResetPlayback();
		else
			bPlaying = false;
	}

	nextFrame = currFrame+1;

	if (nextFrame >= numAnimationFrames)
		nextFrame = loop ? 0 : numAnimationFrames-1;
}

void sequencetimer_t::SetTime(float time)
{
	if(!seq)
		return;

	sequencedesc_t* seqDesc = seq->s;
	int numAnimationFrames = seq->animations[0]->bones[0].numFrames;
	bool loop = (seqDesc->flags & SEQFLAG_LOOP) > 0;

	seq_time = time;

	// compute frame numbers
	currFrame = floor(seq_time);

	if (currFrame >= numAnimationFrames)
	{
		if (seqDesc->flags & SEQFLAG_LOOP)
			ResetPlayback();
		else
			bPlaying = false;
	}

	nextFrame = currFrame + 1;

	if (nextFrame >= numAnimationFrames)
		nextFrame = loop ? 0 : numAnimationFrames-1;
}

void sequencetimer_t::Reset()
{
	bPlaying = false;
	seq = nullptr;

	playbackSpeedScale = 1.0f;

	ResetPlayback();
}

void sequencetimer_t::ResetPlayback()
{
	seq_time = 0.0f;
	eventCounter = 0;
	nextFrame = 0;
	currFrame = 0;
}