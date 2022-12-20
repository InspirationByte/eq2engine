//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source cache
//////////////////////////////////////////////////////////////////////////////////

#include <vorbis/vorbisfile.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "snd_ogg_cache.h"

bool CSoundSource_OggCache::Load()
{
	// Open for binary reading
	IFile* pFile = g_fileSystem->Open(GetFilename(), "rb");
	if(!pFile)
		return false;

	OggVorbis_File oggFile;

	ov_callbacks cb;

	cb.read_func = eqVorbisFile::fread;
	cb.close_func = eqVorbisFile::fclose;
	cb.seek_func = eqVorbisFile::fseek;
	cb.tell_func = eqVorbisFile::ftell;

	int ovResult = ov_open_callbacks(pFile, &oggFile, nullptr, 0, cb);

	if(ovResult < 0)
	{
		g_fileSystem->Close(pFile);
		MsgError("Failed to load sound '%s', because it is not a valid Ogg file (%d)\n", GetFilename(), ovResult);
		return false;
	}

	vorbis_info* info = ov_info(&oggFile, -1);
	ParseFormat(*info);

	ParseData(&oggFile);

	ov_clear( &oggFile );
	g_fileSystem->Close(pFile);

	return m_numSamples > 0;
}

void CSoundSource_OggCache::Unload()
{
	PPFree(m_dataCache);
	m_dataCache = nullptr;
	m_cacheSize = 0;
	m_numSamples = 0;
}

void CSoundSource_OggCache::ParseData(OggVorbis_File* file)
{
	m_numSamples = (uint)ov_pcm_total(file, -1);

	m_cacheSize = m_numSamples * m_format.channels * sizeof(short); // Ogg Vorbis is always 16 bit
	m_dataCache = (ubyte*)PPAlloc(m_cacheSize);

	int samplePos = 0;
	while (samplePos < m_cacheSize)
	{
		char* dest = ((char *)m_dataCache) + samplePos;
		int readBytes = ov_read(file, dest, m_cacheSize-samplePos, 0, 2, 1, nullptr);

		if (readBytes <= 0)
			break;

		samplePos += readBytes;
	}
}

int CSoundSource_OggCache::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
{
	const int sampleSize = m_format.channels * (m_format.bitwidth >> 3);

	const int minSample = 0;
	const int maxSample = m_numSamples;

	int currentOffset = startOffset;
	int numSamplesRead = 0;
	int remainingSamples = samplesToRead;
	while (remainingSamples > 0)
	{
		const int numToRead = min(remainingSamples, maxSample - currentOffset);
		memcpy((ubyte*)out + numSamplesRead * sampleSize, m_dataCache + currentOffset * sampleSize, numToRead * sampleSize);
		numSamplesRead += numToRead;

		if (numToRead < remainingSamples)
		{
			if (loop)
				currentOffset = minSample;
			else
				break;
		}
		remainingSamples -= numToRead;
	}

	return numSamplesRead;
}