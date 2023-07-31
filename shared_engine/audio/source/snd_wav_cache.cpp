//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Cached WAVe data
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "snd_wav_cache.h"
#include "utils/riff.h"

bool CSoundSource_WaveCache::Load()
{
	CRIFF_Parser reader(GetFilename());

	while ( reader.GetName( ) )
	{
		ParseChunk( reader );
		reader.ChunkNext();
	}

	reader.ChunkClose();

	return m_numSamples > 0;
}

void CSoundSource_WaveCache::Unload()
{
	PPFree(m_dataCache);
	m_dataCache = nullptr;
	m_cacheSize = 0;
	m_numSamples = 0;
}

void CSoundSource_WaveCache::ParseData(CRIFF_Parser &chunk)
{
	int sample;

	m_dataCache = (ubyte *)PPAlloc( chunk.GetSize( ) );
	m_cacheSize = chunk.GetSize( );

	m_numSamples = m_cacheSize / (m_format.channels * (m_format.bitwidth >> 3));

	//
	//  read
	//
	chunk.ReadChunk( m_dataCache );

	/*
	//
	//  convert
	//
	for ( int i = 0; i < m_numSamples; i++ )
	{
		if ( m_format.bitwidth == 16 )
		{
			sample = ((short *)m_dataCache)[i];
			((short *)m_dataCache)[i] = sample;
		}
		else
		{
			sample = (int)((unsigned char)(m_dataCache[i]) - 128);
			((signed char *)m_dataCache)[i] = sample;
		}
	}
	*/
}

int CSoundSource_WaveCache::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
{
	const int sampleSize = m_format.channels * (m_format.bitwidth >> 3);

	int minSample = 0;
	int maxSample = m_numSamples;

	const int loopRegionId = GetLoopRegion(startOffset);
	if (loopRegionId != -1)
	{
		minSample = m_loopRegions[loopRegionId].start;
		maxSample = m_loopRegions[loopRegionId].end;
	}

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
