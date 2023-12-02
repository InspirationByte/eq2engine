//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "snd_wav_source.h"
#include "utils/riff.h"

#define CHUNK_FMT			MAKECHAR4('f','m','t',' ')
#define CHUNK_CUE			MAKECHAR4('c','u','e',' ')
#define CHUNK_DATA			MAKECHAR4('d','a','t','a')
#define CHUNK_SAMPLE		MAKECHAR4('s','m','p','l')
#define CHUNK_LTXT			MAKECHAR4('l','t','x','t')
#define CHUNK_LABEL			MAKECHAR4('l','a','b','l')
#define CHUNK_LIST			MAKECHAR4('L','I','S','T')
#define CHUNK_ADTLLIST		MAKECHAR4('a','d','t','l')

typedef struct // CHUNK_FMT
{
	ushort Format;
	ushort Channels;
	uint   SamplesPerSec;
	uint   BytesPerSec;
	ushort BlockAlign;
	ushort BitsPerSample;
} wavfmthdr_t;

typedef struct // CHUNK_SAMPLE
{
	uint   Manufacturer;
	uint   Product;
	uint   SamplePeriod;
	uint   Note;
	uint   FineTune;
	uint   SMPTEFormat;
	uint   SMPTEOffest;
	uint   Loops;
	uint   SamplerData;
}wavsamplehdr_t;

// right after CHUNK_SAMPLE
typedef struct
{
	uint Identifier;
	uint Type;
	uint Start;
	uint End;
	uint Fraction;
	uint Count;
} wavloop_t;

typedef struct // CHUNK_CUE
{
	uint Name;
	uint Position;
	uint fccChunk;
	uint ChunkStart;
	uint BlockStart;
	uint SampleOffset;
} wavcuehdr_t;

typedef struct // CHUNK_LTXT
{
	uint CueId;
	uint SampleLen;
	uint PurposeId;
	ushort Country;
	ushort Language;
	ushort Dialect;
	ushort CodePage;
} wavltxthdr_t;

//---------------------------------------------------------------------

CSoundSource_Wave::CSoundSource_Wave()
{

}

void CSoundSource_Wave::ParseChunk(CRIFF_Parser &chunk)
{
	const uint chunkId = chunk.GetName();
	const char* name = (char*)&chunkId;

    switch (chunkId)
    {
		case CHUNK_FMT:
			ParseFormat( chunk );
			break;
		case CHUNK_CUE:
			ParseCue( chunk );
			break;
		case CHUNK_LIST:
			ParseList(chunk);
			break;
		case CHUNK_DATA:
			ParseData( chunk );
			break;
		case CHUNK_SAMPLE:
			ParseSample( chunk );
		default:
			break;
    }
}

void CSoundSource_Wave::ParseFormat(CRIFF_Parser &chunk)
{
    wavfmthdr_t wfx;
    chunk.ReadChunk( &wfx, sizeof(wavfmthdr_t) );

    m_format.dataFormat = wfx.Format;
    m_format.channels = wfx.Channels;
    m_format.bitwidth = wfx.BitsPerSample;
    m_format.frequency = wfx.SamplesPerSec;
}

void CSoundSource_Wave::ParseCue(CRIFF_Parser &chunk)
{
	int count;
	chunk.ReadChunk(&count, sizeof(int));

	// now read all CUEs
	for (int i = 0; i < count; i++)
	{
		wavcuehdr_t cue;
		chunk.ReadChunk(&cue, sizeof(wavcuehdr_t));

		// TODO: use wave CUE for something like subtitles etc

		//printf("CUE %d time: %d ms (%d)\n", i+1, sampleTimeMilliseconds, cue.SampleOffset);

		// CUESubtitle_t& sub = m_subtitles[m_numSubtitles];
		// sub.sampleStart = float(cue.SampleOffset) / float(m_format.frequency) * 1000.0f;
		// sub.sampleLength = 0;
		// sub.text = NULL;
		// m_numSubtitles++;
	}

	// dont care about the rest
}

void CSoundSource_Wave::ParseSample(CRIFF_Parser &chunk)
{
	wavsamplehdr_t wsx;
	chunk.ReadChunk(&wsx, sizeof(wavsamplehdr_t));

	for (uint i = 0; i < wsx.Loops; ++i)
	{
		wavloop_t loop;
		chunk.ReadChunk(&loop, sizeof(wavloop_t));

		if (loop.Type == 0)
		{
			if (m_loopRegions.numElem() >= m_loopRegions.numAllocated())
				break;

			// only single loop region supported
			m_loopRegions.append({ loop.Start, loop.End });
		}
	}
}

void CSoundSource_Wave::ParseList(CRIFF_Parser& chunk)
{
	int adtl;
	chunk.ReadChunk(&adtl, sizeof(int));

	if (adtl == CHUNK_ADTLLIST)
	{
		int remainingSize = chunk.GetSize() - 4;

		while (remainingSize > 0)
		{
			RIFFchunk_t listChunk;

			int read = chunk.ReadChunk(&listChunk, sizeof(listChunk));
			remainingSize -= read;

			char* name = (char*)&listChunk.Id;

			char namestr[5];
			memcpy(namestr, name, 4);
			namestr[4] = 0;

			if (listChunk.Id == CHUNK_LTXT)
			{
				wavltxthdr_t ltxt;
				int read = chunk.ReadChunk(&ltxt, sizeof(wavltxthdr_t));
				remainingSize -= read;

				const float sampleLength = float(ltxt.SampleLen) / float(m_format.frequency) * 1000.0f;
			}
			else if (listChunk.Id == CHUNK_LABEL)
			{
				char labelString[128];

				int cueId;
				int stringSize = listChunk.Size - 4 + (listChunk.Size & 1);

				int read = chunk.ReadChunk(&cueId, sizeof(int));
				remainingSize -= read;

				read = chunk.ReadChunk(labelString, stringSize);
				remainingSize -= read;
			}
		}
	}
}

int	CSoundSource_Wave::GetLoopRegions(int* samplePos) const
{
	if(!samplePos)
		return m_loopRegions.numElem();

	for (int i = 0; i < m_loopRegions.numElem(); ++i)
	{
		samplePos[i * 2] = m_loopRegions[i].start;
		samplePos[i * 2 + 1] = m_loopRegions[i].end;
	}
	return m_loopRegions.numElem();
}

int CSoundSource_Wave::GetLoopRegion(int offsetInSamples) const
{
	for (int i = 0; i < m_loopRegions.numElem(); ++i)
	{
		if (offsetInSamples >= m_loopRegions[i].start) // && offsetInSamples <= m_loopRegions[i].end)
			return i;
	}
	return -1;
}