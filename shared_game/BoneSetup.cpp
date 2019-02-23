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

void sequencetimer_t::AdvanceFrame(float fDt)
{
	if(seq == NULL)
		return;

	if(!bPlaying)
		return;

	sequencedesc_t* seqDesc = seq->s;

	float frame_time = fDt * playbackSpeedScale * seqDesc->framerate;

	// set new sequence time
	float new_seq_time = sequence_time+frame_time;
	SetTime(new_seq_time);
	
	int numAnimationFrames = seq->animations[0]->bones[0].numFrames - 1;

	// stop non-looping
	if(!(seqDesc->flags & SEQFLAG_LOOP))
	{
		if(new_seq_time >= numAnimationFrames)
			bPlaying = false; // stop
	}
	else
	{
		if(sequence_time >= numAnimationFrames-1)
		{
			ResetPlayback();

			if(numAnimationFrames > 0)
				nextFrame = 1;
		}
	}

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

void sequencetimer_t::SetTime(float time)
{
	if(seq == NULL)
		return;

	sequence_time = time;

	sequencedesc_t* seqDesc = seq->s;

	// compute frame numbers
	currFrame = floor(sequence_time);
	nextFrame = currFrame+1;

	int curNumFrames = seq->animations[0]->bones[0].numFrames-1;

	// check max frame bounds
	if(seqDesc->flags & SEQFLAG_LOOP)
	{
		if(nextFrame >= curNumFrames-1)
		{
			nextFrame = 0;
			currFrame = curNumFrames-1;
		}
	}
	else
	{
		if(nextFrame >= curNumFrames-1)
		{
			nextFrame = curNumFrames-1;
			currFrame = curNumFrames-2;
		}
	}

	sequence_time = clamp(sequence_time, 0, curNumFrames-1); // clamp time to last one frame

	if(currFrame < 0)
		currFrame = 0;

	if(nextFrame < 0)
		nextFrame = 0;
}

void sequencetimer_t::Reset()
{
	bPlaying = false;
	seq = NULL;
	playbackSpeedScale = 1.0f;

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