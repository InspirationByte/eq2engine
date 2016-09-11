//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Ambient sound
//////////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <stdlib.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <vorbis/vorbisfile.h>

#include "DebugInterface.h"
#include "alsound_local.h"

#define EQSOUND_STREAM_BUFFER_SIZE		(1024*32) //16 kb

extern size_t	eqogg_read(void *ptr, size_t size, size_t nmemb, void *datasource);
extern int		eqogg_seek(void *datasource, ogg_int64_t offset, int whence);
extern long		eqogg_tell(void *datasource);

DkSoundAmbient::DkSoundAmbient()
{
	m_loaded = false;
	m_playing = false;
	m_ready = false;

	m_volume = 1.0;
	m_pitch = 1.0;

	m_oggFile = NULL;
	m_sample = NULL;

	alGenSources(1, &m_alSource);
	alGenBuffers(2, m_buffer);
}

DkSoundAmbient::~DkSoundAmbient()
{
	alDeleteSources(1, &m_alSource);
	alDeleteBuffers(2, m_buffer);
}

void DkSoundAmbient::Play()
{
	if(!m_sample)
		return;

	m_sample->WaitForLoad();

	alSourcei(m_alSource, AL_SOURCE_RELATIVE, AL_TRUE);

	alSourcei(m_alSource, AL_LOOPING, (m_sample->m_flags & SAMPLE_FLAG_LOOPING) && !(m_sample->m_flags & SAMPLE_FLAG_STREAMED));

	alSourcef(m_alSource, AL_GAIN, m_volume);
	alSourcef(m_alSource, AL_PITCH, m_pitch);

	if(GetState() != SOUND_STATE_PAUSED)
		alSourceRewind(m_alSource);

	//Play
	if( m_sample->m_flags & SAMPLE_FLAG_STREAMED )
	{
		EqString soundPath = (_Es(SOUND_DEFAULT_PATH) +  m_sample->m_szName).c_str();

		if(GetState() != SOUND_STATE_PAUSED)
		{
			OpenStreamFile( soundPath.c_str() );
			StartStreaming();
		}
		else
			alSourcePlay(m_alSource);
	}
	else
	{
		alSourcei(m_alSource, AL_BUFFER, m_sample->m_alBuffer);
		alSourcePlay(m_alSource);
	}
}

void DkSoundAmbient::Stop()
{
	alSourceStop(m_alSource);
	alSourceRewind(m_alSource);
	alSourcei(m_alSource, AL_BUFFER, AL_NONE);

	StopStreaming();
}

void DkSoundAmbient::StopLoop()
{
	alSourcei(m_alSource, AL_LOOPING, AL_FALSE);
}

void DkSoundAmbient::Pause()
{
	alSourcePause(m_alSource);
}

void DkSoundAmbient::SetVolume(float val)
{
	alSourcef(m_alSource, AL_GAIN, val);
	m_volume = val;
}

void DkSoundAmbient::SetPitch(float val)
{
	alSourcef(m_alSource, AL_PITCH, val);
	m_pitch = val;
}

void DkSoundAmbient::SetSample(ISoundSample* sample)
{
	Stop();

#pragma todo("workaround for disabled RTTI")

	m_sample = (DkSoundSampleLocal*)(sample);
}

ISoundSample* DkSoundAmbient::GetSample() const
{
	return m_sample;
}

ESoundState DkSoundAmbient::GetState() const
{
	ALint alState;

	alGetSourcei(m_alSource, AL_SOURCE_STATE, &alState);

	switch(alState)
	{
		case AL_PLAYING:
			return SOUND_STATE_PLAYING;
		case AL_PAUSED:
			return SOUND_STATE_PAUSED;
		default:
			return SOUND_STATE_STOPPED;
	}
}

void DkSoundAmbient::Update()
{
	if(!m_sample)
		return;

	if( m_sample->m_flags & SAMPLE_FLAG_STREAMED )
		UpdateStreaming();
	else if(GetState() == SOUND_STATE_STOPPED)
		m_sample = NULL;
}

void DkSoundAmbient::OpenStreamFile(const char *name)
{
	if( m_playing )
		StopStreaming();

	//Close previous file
	if(m_oggFile)
	{
		ov_clear(&m_oggStream);
		g_fileSystem->Close(m_oggFile);
		m_oggFile = NULL;
		m_loaded = false;
		m_ready = false;
	}

	// Open for binary reading
	m_oggFile = g_fileSystem->Open(name,"rb");

	if(m_oggFile == NULL)
	{
		MsgError("Failed to load OGG stream '%s'!\n", name);
		return;
	}

	ov_callbacks cb;

	cb.read_func = eqogg_read;
	cb.close_func = NULL;
	cb.seek_func = eqogg_seek;
	cb.tell_func = eqogg_tell;

	int ovResult = ov_open_callbacks(m_oggFile, &m_oggStream, NULL, 0, cb);

	if(ovResult < 0)
	{
		g_fileSystem->Close(m_oggFile);
		MsgError("Can't open sound '%s', is not an ogg file (%d)\n", name, ovResult);
		return;
	}

	m_vorbisInfo = ov_info(&m_oggStream, -1);
	//m_vorbisComment = ov_comment(&m_oggStream, -1);

	if(m_vorbisInfo->channels == 1)
		m_format = AL_FORMAT_MONO16;
	else
		m_format = AL_FORMAT_STEREO16;

	m_loaded = true;
}

void DkSoundAmbient::UpdateStreaming()
{
	if(!m_loaded)
		return;

	if(m_ready && !m_playing)
	{
		// prepare buffers for streaming and play
		UploadStream(m_buffer[0]);
		UploadStream(m_buffer[1]);

		alSourceQueueBuffers(m_alSource, 2, m_buffer);
		alSourcePlay(m_alSource);

		m_playing = true;
	}

	if(!m_playing)
		return;

	int	processedBuffers;

    alGetSourcei(m_alSource, AL_BUFFERS_PROCESSED, &processedBuffers);

	bool hasData = true;

    while(processedBuffers--)
    {
        ALuint buffer;

        alSourceUnqueueBuffers(m_alSource, 1, &buffer);

        hasData = UploadStream(buffer);

		if(hasData)
			alSourceQueueBuffers(m_alSource, 1, &buffer);
		else
			break;
    }


	//Make sure music isn't stopped
	ALenum state;
	alGetSourcei(m_alSource, AL_SOURCE_STATE, &state);

	if(state == AL_STOPPED)
	{
		//Make sure music is looping
		if(!hasData && (m_sample->m_flags & SAMPLE_FLAG_LOOPING))
		{
			ResetStream();
		}

		alSourcePlay(m_alSource);
	}

}

void DkSoundAmbient::ResetStream()
{
	if(!m_loaded)
		return;

	m_playing = false;
	m_ready = true;

	ov_time_seek(&m_oggStream, 0.0);
}

void DkSoundAmbient::StartStreaming()
{
	if(!m_loaded)
		return;

	m_ready = true;
}

void DkSoundAmbient::StopStreaming()
{
	if(!m_loaded)
		return;

	m_ready = false;
	m_playing = false;

	EmptyStreamQueue();

	g_fileSystem->Close(m_oggFile);
	m_oggFile = NULL;
	m_loaded = false;
}

bool DkSoundAmbient::UploadStream(ALuint buffer)
{
	static char pcm[EQSOUND_STREAM_BUFFER_SIZE];

    int  size = 0;
    int  section;
    int  result;

    while(size < EQSOUND_STREAM_BUFFER_SIZE)
    {
        result = ov_read(&m_oggStream, pcm + size, EQSOUND_STREAM_BUFFER_SIZE - size, 0, 2, 1, &section);

        if(result > 0)
            size += result;
		else
			break;
    }

    if(size == 0)
        return false;

    alBufferData(buffer, m_format, pcm, size, m_vorbisInfo->rate);

    return true;
}

void DkSoundAmbient::EmptyStreamQueue()
{
	int queued = 0;

    alGetSourcei(m_alSource, AL_BUFFERS_QUEUED, &queued);

    while(queued--)
    {
        ALuint buffer;

        alSourceUnqueueBuffers(m_alSource, 1, &buffer);
    }
}
