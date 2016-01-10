//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound emitter
//////////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <stdlib.h>

#include "DebugInterface.h"
#include "alsound_local.h"

DkSoundEmitterLocal::DkSoundEmitterLocal()
{
	m_virtual = false;
	m_virtualState = AL_STOPPED;
	m_nChannel = -1;
	vPosition = vec3_zero;
	m_sample = NULL;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);
	m_params = pSoundSystem->m_defaultParams;
}

DkSoundEmitterLocal::~DkSoundEmitterLocal()
{
}

void DkSoundEmitterLocal::SetPosition(Vector3D& position)
{
	//Take over position
	vPosition = position;

	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_pChannels[m_nChannel];

	alSourcefv(c->alSource,AL_POSITION, vPosition);
}

void DkSoundEmitterLocal::SetVelocity(Vector3D& velocity)
{
	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_pChannels[m_nChannel];

	alSourcefv(c->alSource,AL_VELOCITY, velocity);
}

void DkSoundEmitterLocal::SetSample(ISoundSample* sample)
{
	m_sample = dynamic_cast<DkSoundSampleLocal*>(sample);
}

ISoundSample* DkSoundEmitterLocal::GetSample() const
{
	return m_sample;
}

void DkSoundEmitterLocal::Play()
{
	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	if(m_sample && (GetState() == SOUND_STATE_PAUSED))
	{
		// resume sound playing
		if( m_nChannel != -1 )
		{
			m_sample->WaitForLoad();

			sndChannel_t *chnl = pSoundSystem->m_pChannels[m_nChannel];
			alSourcePlay(chnl->alSource);
			chnl->alState = AL_PLAYING;
		}

		return;
	}

	// stop playing first
	Stop();

	if(!m_sample)
	{
        MsgWarning("ISoundEmitter::Play - no sample!!!\n");
        return;
	}

	if(m_nChannel < 0)
		m_nChannel = pSoundSystem->RequestChannel( this );

	if(m_nChannel > -1)
	{
		sndChannel_t *c = pSoundSystem->m_pChannels[m_nChannel];

		if(m_sample->m_bLooping)
			alSourcei(c->alSource,AL_LOOPING, AL_TRUE);
		else
			alSourcei(c->alSource,AL_LOOPING, AL_FALSE);

		m_sample->WaitForLoad();

		alSourcefv(c->alSource,AL_POSITION, vPosition);
		alSourcei(c->alSource, AL_BUFFER, m_sample->m_nALBuffer);

		alSourcePlay(c->alSource);

		alSourcef(c->alSource,AL_REFERENCE_DISTANCE, m_params.referenceDistance);
		alSourcef(c->alSource,AL_MAX_DISTANCE, m_params.maxDistance);
		alSourcef(c->alSource,AL_ROLLOFF_FACTOR, m_params.rolloff);
		alSourcef(c->alSource,AL_GAIN, m_params.volume);
		alSourcef(c->alSource, AL_AIR_ABSORPTION_FACTOR, m_params.airAbsorption);

		alSourcef(c->alSource,AL_PITCH, m_params.pitch);

		alSourcefv(c->alSource,AL_POSITION, vPosition);

		c->alState = AL_PLAYING;
		m_virtual = false;
	}
	else
	{
		m_virtual = true;
		m_virtualState = AL_PLAYING;
	}
}

void DkSoundEmitterLocal::Stop()
{
	if(m_nChannel != -1)
	{
		DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

		sndChannel_t *chnl = pSoundSystem->m_pChannels[m_nChannel];

		alSourceStop(chnl->alSource);
		chnl->alState = AL_STOPPED;
		m_nChannel = -1;
	}
	else if(m_virtual)
	{
		// exit from virtual mode
		m_virtualState = AL_STOPPED;
		m_virtual = false;
	}
}

void DkSoundEmitterLocal::Pause()
{
	if(m_nChannel != -1)
	{
		DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

		sndChannel_t *chnl = pSoundSystem->m_pChannels[m_nChannel];

		alSourcePause(chnl->alSource);
		chnl->alState = AL_PAUSED;
	}
	else if(m_virtual)
	{
		m_virtualState = AL_STOPPED;
	}
}

bool DkSoundEmitterLocal::IsStopped() const
{
	// only looping samples couldn't be get stopped in virtual mode...
	bool virtualLooped = m_virtual && m_sample->m_bLooping && (m_virtualState == AL_STOPPED);

	return(m_nChannel < 0) && !virtualLooped;
}

ESoundState DkSoundEmitterLocal::GetState() const
{
	if(IsStopped())
		return SOUND_STATE_STOPPED;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t* chnl = pSoundSystem->m_pChannels[m_nChannel];

	switch(chnl->alState)
	{
		case AL_PLAYING:
			return SOUND_STATE_PLAYING;
		case AL_PAUSED:
			return SOUND_STATE_PAUSED;
	}

	return SOUND_STATE_STOPPED;
}

bool DkSoundEmitterLocal::IsVirtual() const
{
	return m_virtual;
}

void DkSoundEmitterLocal::Update()
{
	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	if(m_nChannel == -1)
	{
		if(m_virtual && m_virtualState == AL_PLAYING)
			Play();

		return;
	}
	else if(m_sample && (m_sample->m_nChannels == 1))
	{
		if( (length(vPosition - soundsystem->GetListenerPosition())) > m_params.maxDistance)
		{
			sndChannel_t* chnl = pSoundSystem->m_pChannels[m_nChannel];

			// free channel and switch to virtual mode
			if(chnl->alState == AL_PLAYING)
			{
				Stop();
				m_virtual = true;
				m_virtualState = AL_PLAYING;
				return;
			}
		}
	}
}

void DkSoundEmitterLocal::GetParams(soundParams_t *param) const
{
	*param = m_params;
}

void DkSoundEmitterLocal::SetVolume(float val)
{
	m_params.volume = val;

	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_pChannels[m_nChannel];

	alSourcef(c->alSource,AL_GAIN, m_params.volume);
}


void DkSoundEmitterLocal::SetPitch(float val)
{
	m_params.pitch = val;

	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_pChannels[m_nChannel];

	alSourcef(c->alSource,AL_PITCH, m_params.pitch);
}

void DkSoundEmitterLocal::SetParams(soundParams_t *param)
{
	m_params = *param;

	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_pChannels[m_nChannel];

	alSourcefv(c->alSource,AL_POSITION, vPosition);

	alSourcef(c->alSource,AL_REFERENCE_DISTANCE, m_params.referenceDistance);
	alSourcef(c->alSource,AL_MAX_DISTANCE, m_params.maxDistance);
	alSourcef(c->alSource,AL_ROLLOFF_FACTOR, m_params.rolloff);

	alSourcef(c->alSource, AL_AIR_ABSORPTION_FACTOR, m_params.airAbsorption);

	alSourcef(c->alSource,AL_GAIN, m_params.volume);
	alSourcef(c->alSource,AL_PITCH, m_params.pitch);
}
