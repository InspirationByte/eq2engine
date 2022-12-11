//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#include <vorbis/vorbisfile.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "snd_source.h"

#include "audio/IEqAudioSystem.h"

#include "snd_wav_cache.h"
#include "snd_wav_stream.h"
#include "snd_ogg_cache.h"
#include "snd_ogg_stream.h"

#define STREAM_THRESHOLD    (1024 * 1024)     // 1mb

//-----------------------------------------------------------------

CRefPtr<ISoundSource> ISoundSource::CreateSound( const char* szFilename )
{
	EqString fileExt = _Es(szFilename).Path_Extract_Ext();

	CRefPtr<ISoundSource> pSource = nullptr;

	if ( !fileExt.CompareCaseIns("wav"))
	{
		const int filelen = g_fileSystem->GetFileSize( szFilename );

		if ( filelen > STREAM_THRESHOLD )
			pSource = static_cast<CRefPtr<ISoundSource>>(CRefPtr_new(CSoundSource_WaveStream));
		else
			pSource = static_cast<CRefPtr<ISoundSource>>(CRefPtr_new(CSoundSource_WaveCache));
	}
	else if ( !fileExt.CompareCaseIns("ogg"))
	{
		int filelen = g_fileSystem->GetFileSize( szFilename );

		if ( filelen > STREAM_THRESHOLD )
			pSource = static_cast<CRefPtr<ISoundSource>>(CRefPtr_new( CSoundSource_OggStream));
		else
			pSource = static_cast<CRefPtr<ISoundSource>>(CRefPtr_new(CSoundSource_OggCache));
	}
	else
		MsgError( "Unknown audio format: %s\n", szFilename );

	if(pSource && !pSource->Load( szFilename ))
	{
		MsgError( "Cannot load sound '%s'\n", szFilename );
		return nullptr;
	}

	return pSource;
}

void ISoundSource::Ref_DeleteObject()
{
	g_audioSystem->OnSampleDeleted(this);
	this->Unload();
	delete this;
}