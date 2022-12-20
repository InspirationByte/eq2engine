//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Streamed WAVe source
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "snd_wav_stream.h"
#include "utils/riff.h"

//--------------------------------------------------------

bool CSoundSource_WaveStream::Load()
{
	m_reader = PPNew CRIFF_Parser(GetFilename());

	while ( m_reader->GetName( ) )
	{
		ParseChunk( *m_reader );
		m_reader->ChunkNext();
	}

	return m_numSamples > 0;
}

void CSoundSource_WaveStream::Unload()
{
	m_reader->ChunkClose();
	delete m_reader;
	m_numSamples = 0;
}

void CSoundSource_WaveStream::ParseData(CRIFF_Parser &chunk)
{
	m_dataOffset = chunk.GetPos();
	m_dataSize = chunk.GetSize();

	chunk.SkipData(m_dataSize);

	m_numSamples = m_dataSize / (m_format.channels * (m_format.bitwidth >> 3));
}

int CSoundSource_WaveStream::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
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
		ReadData((ubyte*)out + numSamplesRead * sampleSize, currentOffset * sampleSize, numToRead * sampleSize);
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

int CSoundSource_WaveStream::ReadData(void* out, int offset, int count) const
{
	int sample;

	m_reader->SetPos( m_dataOffset + offset);
	m_reader->ReadData(out, count);

	/*
	int fin = nBytes / (m_format.bitwidth >> 3);

	for ( int i = 0; i < fin; i++ )
	{
		if ( m_format.bitwidth == 16 )
		{
			sample = ((short *)pOutput)[i];
			((short *)pOutput)[i] = sample;
		}
		else
		{
			sample = (int)((unsigned char)(pOutput[i]) - 128);
			((signed char *)pOutput)[i] = sample;
		}
	}
	*/

	return 0;
}
