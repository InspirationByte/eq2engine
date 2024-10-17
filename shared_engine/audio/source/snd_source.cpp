//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#include <minivorbis.h>
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

ISoundSourcePtr ISoundSource::CreateSound( const char* szFilename )
{
	EqString fileExt = fnmPathExtractExt(szFilename);

	ISoundSourcePtr pSource = nullptr;

	if ( !fileExt.CompareCaseIns("wav"))
	{
		const int filelen = g_fileSystem->GetFileSize( szFilename );

		if ( filelen > STREAM_THRESHOLD )
			pSource = ISoundSourcePtr(CRefPtr_new(CSoundSource_WaveStream));
		else
			pSource = ISoundSourcePtr(CRefPtr_new(CSoundSource_WaveCache));
	}
	else if ( !fileExt.CompareCaseIns("ogg"))
	{
		int filelen = g_fileSystem->GetFileSize( szFilename );

		if ( filelen > STREAM_THRESHOLD )
			pSource = ISoundSourcePtr(CRefPtr_new( CSoundSource_OggStream));
		else
			pSource = ISoundSourcePtr(CRefPtr_new(CSoundSource_OggCache));
	}
	else
		MsgError( "Unknown audio format: %s\n", szFilename );

	pSource->SetFilename(szFilename);

	if(pSource && !pSource->Load())
		return nullptr;

	return pSource;
}

void ISoundSource::Ref_DeleteObject()
{
	g_audioSystem->OnSampleDeleted(this);
	Unload();

	RefCountedObject::Ref_DeleteObject();
}

const char* ISoundSource::GetFilename() const
{
	return m_fileName;
}

void ISoundSource::SetFilename(const char* filename)
{
	m_fileName = filename;
	m_nameHash = StringId24(filename, true);
}