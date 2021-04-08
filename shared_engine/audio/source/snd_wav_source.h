//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_WAV_SOURCE_H
#define SND_WAV_SOURCE_H

#include "snd_source.h"

#include "utils/eqstring.h"
#include "utils/riff.h"

//---------------------------------------------------------------------

class CSoundSource_Wave : public ISoundSource
{
public:
	CSoundSource_Wave();

	virtual soundFormat_t*	GetFormat() const				{ return (soundFormat_t*)&m_format; }
	virtual const char*		GetFilename() const				{ return m_filename.ToCString(); }
	virtual float			GetLoopPosition(float flPosition) const;
	virtual int				GetSampleCount() const			{ return m_numSamples; }

protected:
	void					ParseChunk(CRIFF_Parser &chunk);

	virtual void			ParseFormat(CRIFF_Parser &chunk);
	virtual void			ParseCue(CRIFF_Parser &chunk);
	virtual void			ParseSample(CRIFF_Parser &chunk);
	virtual void			ParseData(CRIFF_Parser &chunk) = 0;

	soundFormat_t			m_format;
	EqString				m_filename;

	int						m_numSamples;
	int						m_loopStart;
	int						m_loopEnd;
};

#endif // SND_WAV_SOURCE_H
