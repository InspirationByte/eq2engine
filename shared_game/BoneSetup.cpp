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
	if(base_sequence == NULL)
		return;

	if(!bPlaying)
		return;

	float frame_time = fDt*playbackSpeedScale*base_sequence->framerate;

	// set new sequence time
	float new_seq_time = sequence_time+frame_time;
	SetTime(new_seq_time);

	int curNumFrames = base_sequence->animations[0]->bones[0].numframes-1;

	// stop non-looping
	if(!(base_sequence->flags & SEQFLAG_LOOP))
	{
		if(new_seq_time >= curNumFrames)
			bPlaying = false; // stop
	}
	else
	{
		if(sequence_time >= curNumFrames-1)
		{
			ResetPlayback();

			if(curNumFrames > 0)
				nextFrame = 1;
		}
	}

	for(int i = 0; i < base_sequence->numEvents; i++)
	{
		if(sequence_time > base_sequence->events[i]->frame)
		{
			if(ignore_events.findIndex(i) == -1)
			{
				called_events.append(base_sequence->events[i]);
				ignore_events.append(i);
			}
		}
	}
}

void sequencetimer_t::SetTime(float time)
{
	if(base_sequence == NULL)
		return;

	sequence_time = time;

	// compute frame numbers
	currFrame = floor(sequence_time);
	nextFrame = currFrame+1;

	int curNumFrames = base_sequence->animations[0]->bones[0].numframes-1;

	// check max frame bounds
	if(base_sequence->flags & SEQFLAG_LOOP)
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

void sequencetimer_t::ResetPlayback(bool frame_reset )
{
	ignore_events.clear();

	sequence_time = 0;

	if(frame_reset)
	{
		nextFrame = 0;
		currFrame = 0;
	}
}