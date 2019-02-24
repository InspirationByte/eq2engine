//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Bone setup
//////////////////////////////////////////////////////////////////////////////////

#include "BoneSetup.h"

#if !defined(EDITOR) && !defined(NO_GAME)
BEGIN_DATAMAP_NO_BASE(sequencetimer_t)
	DEFINE_FIELD(sequence_index, VTYPE_INTEGER),
	DEFINE_FIELD(sequence_time, VTYPE_FLOAT),
	DEFINE_FIELD(nextFrame, VTYPE_INTEGER),
	DEFINE_FIELD(currFrame, VTYPE_INTEGER),
	DEFINE_FIELD(playbackSpeedScale, VTYPE_FLOAT),
	DEFINE_FIELD(bPlaying, VTYPE_BOOLEAN),
END_DATAMAP()
#endif // EDITOR

sequencetimer_t::sequencetimer_t()
{
	bPlaying = false;
	seq = NULL;
	sequence_time = 0.0f;
	currFrame = 0;
	nextFrame = 0;
	playbackSpeedScale = 1.0f;
	sequence_index = -1;
}

void sequencetimer_t::AdvanceFrame(float fDt)
{
	if(seq == NULL)
		return;

	if(!bPlaying)
		return;

	sequencedesc_t* seqDesc = seq->s;
	int numAnimationFrames = seq->animations[0]->bones[0].numFrames - 1;

	float frame_time = fDt * playbackSpeedScale * seqDesc->framerate;

	// set new sequence time
	sequence_time = sequence_time+frame_time;
	currFrame = floor(sequence_time);

	if (currFrame > numAnimationFrames -1)
	{
		if (seqDesc->flags & SEQFLAG_LOOP)
			ResetPlayback();
		else
			bPlaying = false;
	}

	nextFrame = currFrame+1;

	if (nextFrame > numAnimationFrames - 1)
		nextFrame = (seqDesc->flags & SEQFLAG_LOOP) ? 0 : numAnimationFrames-1;

	for(int i = 0; i < seqDesc->numEvents; i++)
	{
		sequenceevent_t* evt = seq->events[i];

		if (sequence_time < evt->frame)
			continue;

		if(ignore_events.findIndex(i) == -1)
		{
			called_events.append(evt);
			ignore_events.append(i);
		}
	}
}

void sequencetimer_t::SetTime(float time)
{
	if(seq == NULL)
		return;

	sequencedesc_t* seqDesc = seq->s;
	int numAnimationFrames = seq->animations[0]->bones[0].numFrames;

	sequence_time = time;

	// compute frame numbers
	currFrame = floor(sequence_time);

	if (currFrame > numAnimationFrames-1)
		currFrame = (seqDesc->flags & SEQFLAG_LOOP) ? 0 : numAnimationFrames-1;

	nextFrame = currFrame+1;

	if (nextFrame > numAnimationFrames-1)
		nextFrame = (seqDesc->flags & SEQFLAG_LOOP) ? 0 : numAnimationFrames-1;
}

void sequencetimer_t::Reset()
{
	bPlaying = false;
	playbackSpeedScale = 1.0f;

	seq = nullptr;
	sequence_index = -1;

	ResetPlayback(true);
}

void sequencetimer_t::ResetPlayback(bool frame_reset )
{
	ignore_events.clear();
	called_events.clear();

	sequence_time = 0.0f;

	if(frame_reset)
	{
		nextFrame = 0;
		currFrame = 0;
	}
}