//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#include "snd_wav_source.h"

//---------------------------------------------------------------------

CSoundSource_Wave::CSoundSource_Wave() : m_loopStart(0), m_numSamples(0)
{

}

void CSoundSource_Wave::ParseChunk(CRIFF_Parser &chunk)
{
	int nameId = chunk.GetName();

	char* name = (char*)&nameId;

	char namestr[5];
	memcpy(namestr, name, 4);
	namestr[4] = 0;

    switch ( chunk.GetName() )
    {
		case CHUNK_FMT:
			ParseFormat( chunk );
			break;
		case CHUNK_CUE:
			ParseCue( chunk );
			break;
		case CHUNK_DATA:
			ParseData( chunk );
			break;
		default:
			break;
    }
}

void CSoundSource_Wave::ParseFormat(CRIFF_Parser &chunk)
{
    wavfmthdr_t wfx;

    chunk.ReadData( (ubyte*)&wfx, chunk.GetSize() );

    m_format.format = wfx.Format;
    m_format.channels = wfx.Channels;
    m_format.bitwidth = wfx.BitsPerSample;
    m_format.frequency = wfx.SamplesPerSec;
}

void CSoundSource_Wave::ParseCue(CRIFF_Parser &chunk)
{
	wavcuehdr_t cue;

    int cue_count = chunk.ReadInt();
	cue_count;

    chunk.ReadData( (ubyte *)&cue, sizeof(cue) );
    m_loopStart = cue.SampleOffset;

    // dont care about the rest
}

float CSoundSource_Wave::GetLoopPosition(float flPosition)
{
    while ( flPosition > m_numSamples )
        flPosition -= m_numSamples;

    return flPosition;
}
