//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenAL-compatible sound source (alBuffer)
//				No streaming support for this type, it just holds alBuffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "snd_source.h"

class CSoundSource_OpenALCache : public ISoundSource
{
	friend class CEqAudioSystemAL;
	friend class CEqAudioSourceAL;
public:
	CSoundSource_OpenALCache(ISoundSource* source);

	virtual int             GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const;
	virtual void*			GetDataPtr(int& dataSize) const;

	virtual const Format&	GetFormat() const;
	virtual int				GetSampleCount() const;

	int						GetLoopRegions(int* samplePos) const { return 0; }

	virtual bool			IsStreaming() const;

private:

	virtual bool			Load();
	virtual void			Unload();

	uint					m_alBuffer{ 0 };
	Format					m_format;
};

ALenum GetSoundSourceFormatAsALEnum(const ISoundSource::Format& fmt);