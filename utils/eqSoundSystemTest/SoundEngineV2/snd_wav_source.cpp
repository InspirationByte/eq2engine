//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source base class
//////////////////////////////////////////////////////////////////////////////////

#include "snd_wav_source.h"

#include "snd_wav_cache.h"
#include "snd_wav_stream.h"

#include "utils/eqstring.h"

#include "DebugInterface.h"

//-----------------------------------------------------------------

ISoundSource* ISoundSource::CreateSound( const char* szFilename )
{
	EqString fileExt = _Es(szFilename).Path_Extract_Ext();

	ISoundSource* pSource = NULL;

    if ( !fileExt.CompareCaseIns("wav"))
    {
		int filelen = g_fileSystem->GetFileSize( szFilename );

		if ( filelen > STREAM_THRESHOLD )
			pSource = (ISoundSource*)new CSoundSource_WaveStream;
		else
			pSource = (ISoundSource*)new CSoundSource_WaveCache;
    }
    else
		MsgError( "unknown sound format: %s\n", szFilename );


    if ( pSource )
	{
		if(!pSource->Load( szFilename ))
		{
			MsgError( "Cannot load sound '%s'\n", szFilename );
			delete pSource;
		}

        return pSource;
	}
    else if ( pSource )
        delete pSource;
    
	Msg("Almost fucked up\n");

    return NULL;
}

void ISoundSource::DestroySound(ISoundSource *pSound)
{
    if ( pSound )
    {
        pSound->Unload( );
        delete pSound;
    }
}

//---------------------------------------------------------------------

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
