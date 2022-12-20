//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenAL-compatible sound source (alBuffer)
//				No streaming support for this type, it just holds alBuffer
//////////////////////////////////////////////////////////////////////////////////

#include <AL/al.h>
#include <AL/alext.h>
#include <vorbis/vorbisfile.h>

#include "core/core_common.h"
#include "snd_al_source.h"
#include "snd_ogg_cache.h"
#include "snd_wav_cache.h"

CSoundSource_OpenALCache::CSoundSource_OpenALCache(ISoundSource* source)
{
	if (_Es(source->GetFilename()).Path_Extract_Ext() == "wav")
	{
		CSoundSource_WaveCache* wav = (CSoundSource_WaveCache*)source;
		InitWav(wav);
	}
	else if (_Es(source->GetFilename()).Path_Extract_Ext() == "ogg")
	{
		CSoundSource_OggCache* ogg = (CSoundSource_OggCache*)source;
		InitOgg(ogg);
	}
}

void CSoundSource_OpenALCache::InitWav(CSoundSource_WaveCache* wav)
{
	alGenBuffers(1, &m_alBuffer);

	SetFilename(wav->GetFilename());

	m_format = wav->GetFormat();
	ALenum alFormat;

	if (m_format.bitwidth == 8)
		alFormat = m_format.channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	else if (m_format.bitwidth == 16)
		alFormat = m_format.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	else
		alFormat = AL_FORMAT_MONO16;

	alBufferData(m_alBuffer, alFormat, wav->m_dataCache, wav->m_cacheSize, m_format.frequency);

	int loopPoints[SOUND_SOURCE_MAX_LOOP_REGIONS * 2]{ 0 };
	const int numLoopRegions = wav->GetLoopRegions(loopPoints);

	// setup additional loop points
	if (numLoopRegions)
	{
		if (loopPoints[1] == -1)
			alGetBufferi(m_alBuffer, AL_SAMPLE_LENGTH_SOFT, &loopPoints[1]); // loop to the end

		const int sampleOffs[] = { loopPoints[0], loopPoints[1] };
		alBufferiv(m_alBuffer, AL_LOOP_POINTS_SOFT, sampleOffs);
	}
}

void CSoundSource_OpenALCache::InitOgg(CSoundSource_OggCache* ogg)
{
	alGenBuffers(1, &m_alBuffer);

	SetFilename(ogg->GetFilename());

	m_format = ogg->GetFormat();
	ALenum alFormat;

	if (m_format.bitwidth == 8)
		alFormat = m_format.channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	else if (m_format.bitwidth == 16)
		alFormat = m_format.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	else
		alFormat = AL_FORMAT_MONO16;

	alBufferData(m_alBuffer, alFormat, ogg->m_dataCache, ogg->m_cacheSize, m_format.frequency);
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