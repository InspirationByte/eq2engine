//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source base class
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_OGG_SOURCE_H
#define SND_OGG_SOURCE_H

#include "snd_source.h"
#include "utils/eqstring.h"

#define STREAM_THRESHOLD    0x10000     // 65k

//---------------------------------------------------------------------

class CSoundSource_Ogg : public ISoundSource
{
public:
	virtual soundFormat_t*	GetFormat()						{ return &m_format; }
	virtual const char*		GetFilename() const				{ return m_filename.c_str(); }
	virtual float			GetLoopPosition(float flPosition);

protected:
    soundFormat_t			m_format;
    EqString				m_filename;

    int						m_numSamples;
    int						m_loopStart;
};

#endif // SND_OGG_SOURCE_H
