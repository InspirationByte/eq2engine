//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ogg Vorbis source stream
//////////////////////////////////////////////////////////////////////////////////

#include <vorbis/vorbisfile.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "snd_ogg_stream.h"

bool CSoundSource_OggStream::Load(const char* filename)
{
	// Open for binary reading
	m_oggFile = g_fileSystem->Open(filename, "rb");
	if(!m_oggFile)
		return false;

	ov_callbacks cb;

	cb.read_func = eqVorbisFile::fread;
	cb.close_func = eqVorbisFile::fclose;
	cb.seek_func = eqVorbisFile::fseek;
	cb.tell_func = eqVorbisFile::ftell;

	int ovResult = ov_open_callbacks(m_oggFile, &m_oggStream, nullptr, 0, cb);

	if(ovResult < 0)
	{
		g_fileSystem->Close(m_oggFile);
		m_oggFile = nullptr;

		MsgError("Failed to load sound '%s', because it is not a valid Ogg file (%d)\n", filename, ovResult);
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
		g_fileSystem->Close(m_oggFile);
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

int CSoundSource_OggStream::GetSamples(ubyte* pOutput, int nSamples, int nOffset, bool bLooping)
{
	int     nRemaining;
	int     nBytes, nStart;

	int     nSampleSize = m_format.channels * (m_format.bitwidth >> 3);

	nBytes = nSamples * nSampleSize;
	nStart = nOffset * nSampleSize;

	nRemaining = nBytes;

	if (nBytes + nStart > m_dataSize)
		nBytes = m_dataSize - nStart;

	ReadData(pOutput, nOffset, nBytes);
	nRemaining -= nBytes;

	if (nRemaining && bLooping)
	{
		ReadData(pOutput + nBytes, 0, nRemaining);

		return (nBytes + nRemaining) / nSampleSize;
	}

	return nBytes / nSampleSize;
}

int CSoundSource_OggStream::ReadData(ubyte* out, int nStart, int nBytes)
{
	if(nStart >= 0)
		ov_pcm_seek(&m_oggStream, nStart);

	int  samplePos = 0;
	while(samplePos < nBytes)
	{
		char* dest = ((char *)out) + samplePos;
		int readBytes = ov_read(&m_oggStream, dest, nBytes - samplePos, 0, 2, 1, nullptr);

		if (readBytes <= 0)
			break;

		samplePos += readBytes;
	}

	return 0;
}