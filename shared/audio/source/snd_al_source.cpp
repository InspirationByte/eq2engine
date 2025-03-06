//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenAL-compatible sound source (alBuffer)
//				No streaming support for this type, it just holds alBuffer
//////////////////////////////////////////////////////////////////////////////////

#include <AL/al.h>
#include <AL/alext.h>
#include <minivorbis.h>

#include "core/core_common.h"
#include "snd_al_source.h"
#include "snd_ogg_cache.h"
#include "snd_wav_cache.h"

ALenum GetSoundSourceFormatAsALEnum(const ISoundSource::Format& fmt)
{
	ALenum alFormat;
	if (fmt.bitwidth == 8)
		alFormat = fmt.channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	else if (fmt.bitwidth == 16)
		alFormat = fmt.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	else
		alFormat = AL_FORMAT_MONO16;

	return alFormat;
}

CSoundSource_OpenALCache::CSoundSource_OpenALCache(ISoundSource* source)
{
	SetFilename(source->GetFilename());
	m_format = source->GetFormat();

	int dataSize = 0;
	const void* dataPtr = source->GetDataPtr(dataSize);
	if (!dataPtr || !dataSize)
	{
		ASSERT_FAIL("Input source has data for creating AL cache source (unsupported format)");
		return;
	}

	const ALenum alFormat = GetSoundSourceFormatAsALEnum(m_format);

	alGenBuffers(1, &m_alBuffer);
	alBufferData(m_alBuffer, alFormat, dataPtr, dataSize, m_format.frequency);

	// setup additional loop points
	int loopPoints[SOUND_SOURCE_MAX_LOOP_REGIONS * 2]{ 0 };
	const int numLoopRegions = source->GetLoopRegions(loopPoints);

	if (numLoopRegions)
	{
		if (loopPoints[1] == -1)
			alGetBufferi(m_alBuffer, AL_SAMPLE_LENGTH_SOFT, &loopPoints[1]); // loop to the end

		const int sampleOffs[] = { loopPoints[0], loopPoints[1] };
		alBufferiv(m_alBuffer, AL_LOOP_POINTS_SOFT, sampleOffs);
	}
}

void CSoundSource_OpenALCache::Unload()
{
	alDeleteBuffers(1, &m_alBuffer);
}

int CSoundSource_OpenALCache::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
{
	return 0; 
}

void* CSoundSource_OpenALCache::GetDataPtr(int& dataSize) const 
{
	return nullptr; 
}

const ISoundSource::Format& CSoundSource_OpenALCache::GetFormat() const
{
	return m_format;
}

int CSoundSource_OpenALCache::GetSampleCount() const 
{ 
	return 0;
};

bool CSoundSource_OpenALCache::IsStreaming() const
{
	return false;
}

bool CSoundSource_OpenALCache::Load() 
{
	return false; 
}