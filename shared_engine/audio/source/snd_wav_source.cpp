//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "snd_wav_source.h"
#include "utils/riff.h"

#define CHUNK_FMT			MCHAR4('f','m','t',' ')
#define CHUNK_CUE			MCHAR4('c','u','e',' ')
#define CHUNK_DATA			MCHAR4('d','a','t','a')
#define CHUNK_SAMPLE		MCHAR4('s','m','p','l')
#define CHUNK_LTXT			MCHAR4('l','t','x','t')
#define CHUNK_LABEL			MCHAR4('l','a','b','l')
#define CHUNK_LIST			MCHAR4('L','I','S','T')
#define CHUNK_ADTLLIST		MCHAR4('a','d','t','l')

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

	struct
	{
		uint Identifier;
		uint Type;
		uint Start;
		uint End;
		uint Fraction;
		uint Count;
	}Loop[1];
}wavsamplehdr_t;

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

CSoundSource_Wave::CSoundSource_Wave() : m_loopStart(0), m_loopEnd(0), m_numSamples(0)
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
		m_loopStart = cue.SampleOffset;

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

	if (wsx.Loop[0].Type == 0) // only single loop region supported
	{
		ASSERT(wsx.Loops > 0);
		m_loopStart = wsx.Loop[0].Start;
		m_loopEnd = wsx.Loop[0].End;
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

float CSoundSource_Wave::GetLoopPosition(float flPosition) const
{
    while ( flPosition > m_numSamples )
        flPosition -= m_numSamples;

    return flPosition;
}
