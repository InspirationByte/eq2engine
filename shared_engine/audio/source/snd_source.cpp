//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#include "snd_source.h"

#include "core/IFileSystem.h"
#include "core/DebugInterface.h"

#include "utils/eqstring.h"

#include "snd_wav_cache.h"
#include "snd_wav_stream.h"

#include "snd_ogg_cache.h"
#include "snd_ogg_stream.h"

#define STREAM_THRESHOLD    (1024*1024)     // 1mb

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
	else if ( !fileExt.CompareCaseIns("ogg"))
	{
		int filelen = g_fileSystem->GetFileSize( szFilename );

		if ( filelen > STREAM_THRESHOLD )
			pSource = (ISoundSource*)new CSoundSource_OggStream;
		else
			pSource = (ISoundSource*)new CSoundSource_OggCache;
	}
	else
		MsgError( "Unknown sound format: %s\n", szFilename );


	if ( pSource )
	{
		if(!pSource->Load( szFilename ))
		{
			MsgError( "Cannot load sound '%s'\n", szFilename );
			delete pSource;
			pSource = nullptr;
		}

		return pSource;
	}
	else if ( pSource )
		delete pSource;

	return nullptr;
}

void ISoundSource::DestroySound(ISoundSource *pSound)
{
    if ( pSound )
    {
        pSound->Unload( );
        delete pSound;
    }
}