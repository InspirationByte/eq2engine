//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenAL-compatible sound source (alBuffer)
//				No streaming support for this type, it just holds alBuffer
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_AL_SOURCE_H
#define SND_AL_SOURCE_H

#include "snd_source.h"
#include "utils/eqstring.h"

#include <AL/al.h>

class CSoundSource_WaveCache;
class CSoundSource_OggCache;

class CSoundSource_OpenALCache : public ISoundSource
{
	friend class CEqAudioSystemAL;
public:
	CSoundSource_OpenALCache(ISoundSource* source);

	virtual int             GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping) { return 0; };
	virtual ubyte*			GetDataPtr(int& dataSize) const { return 0; };

	virtual soundFormat_t*	GetFormat() { return &m_format; };
	virtual const char*		GetFilename() const { return m_filename.c_str(); }
	virtual int				GetSampleCount() const { return 0; };

	virtual float           GetLoopPosition(float flPosition) { return 0; };

	virtual bool			IsStreaming() { return false; }

private:
	void					InitWav(CSoundSource_WaveCache* wav);
	void					InitOgg(CSoundSource_OggCache* ogg);

	virtual bool			Load(const char *szFilename) { return false; };
	virtual void			Unload();

	ALuint					m_alBuffer;
	soundFormat_t			m_format;
	EqString				m_filename;
};

#endif // SND_AL_SOURCE_H