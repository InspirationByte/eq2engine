//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source stream
//////////////////////////////////////////////////////////////////////////////////

#include <minivorbis.h>
#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "snd_ogg_stream.h"

bool CSoundSource_OggStream::Load()
{
	// Open for binary reading
	m_oggFile = g_fileSystem->Open(GetFilename(), "rb");
	if(!m_oggFile)
		return false;

	int ovResult = ov_open_callbacks(m_oggFile, &m_oggStream, nullptr, 0, eqVorbisFile::callbacks);

	if(ovResult < 0)
	{
		m_oggFile = nullptr;

		MsgError("Failed to load sound '%s', because it is not a valid Ogg file (%d)\n", GetFilename(), ovResult);
		return false;
	}

	vorbis_info* info = ov_info(&m_oggStream, -1);
	ParseFormat(*info);

	ParseData(&m_oggStream);

	return m_numSamples > 0;
}

void CSoundSource_OggStream::Unload()
{
	if(m_oggFile)
	{
		ov_clear( &m_oggStream );
	}

	m_oggFile = nullptr;
	m_numSamples = 0;
}

void CSoundSource_OggStream::ParseData(OggVorbis_File* file)
{
	// only get number of samples
	m_numSamples = (uint)ov_pcm_total(file, -1);
	m_dataSize = m_numSamples * m_format.channels * sizeof(short); // Ogg Vorbis is always 16 bit
}

int CSoundSource_OggStream::GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const
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

int CSoundSource_OggStream::ReadData(void* out, int offset, int count) const
{
	const int sampleSize = m_format.channels * (m_format.bitwidth >> 3);
	ov_pcm_seek(const_cast<OggVorbis_File*>(&m_oggStream), offset / sampleSize);

	int totalBytes = 0;
	while(totalBytes < count)
	{
		char* dest = ((char*)out) + totalBytes;
		const int readBytes = ov_read(const_cast<OggVorbis_File*>(&m_oggStream), dest, count - totalBytes, 0, 2, 1, nullptr);

		if (readBytes <= 0)
			break;

		totalBytes += readBytes;
	}

	return 0;
}