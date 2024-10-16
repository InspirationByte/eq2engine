//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "snd_source.h"

class CRIFF_Parser;

//---------------------------------------------------------------------

class CSoundSource_Wave : public ISoundSource
{
public:
	virtual const Format&	GetFormat() const				{ return m_format; }
	virtual int				GetSampleCount() const			{ return m_numSamples; }
	int						GetLoopRegions(int* samplePos) const;

protected:
	int						GetLoopRegion(int offsetInSamples) const;

	void					ParseChunk(CRIFF_Parser &chunk);

	virtual void			ParseFormat(CRIFF_Parser &chunk);
	virtual void			ParseCue(CRIFF_Parser &chunk);
	virtual void			ParseSample(CRIFF_Parser &chunk);
	virtual void			ParseList(CRIFF_Parser& chunk);
	virtual void			ParseData(CRIFF_Parser &chunk) = 0;

	struct LoopRegion
	{
		uint start;
		uint end;
	};

	FixedArray<LoopRegion, SOUND_SOURCE_MAX_LOOP_REGIONS> m_loopRegions;
	Format					m_format;
	int						m_numSamples{ 0 };
};
