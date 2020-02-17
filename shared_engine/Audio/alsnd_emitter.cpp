//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Sound system
//
//				Sound emitter
//////////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <stdlib.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <vorbis/vorbisfile.h>

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

void DkSoundEmitterLocal::SetPosition(const Vector3D& position)
{
	//Take over position
	vPosition = position;

	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_channels[m_nChannel];

	alSourcefv(c->alSource, AL_POSITION, vPosition);
}

void DkSoundEmitterLocal::SetVelocity(const Vector3D& velocity)
{
	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *c = pSoundSystem->m_channels[m_nChannel];

	alSourcefv(c->alSource,AL_VELOCITY, velocity);
}

void DkSoundEmitterLocal::SetSample(ISoundSample* sample)
{
	Stop();

#pragma todo("workaround for disabled RTTI")
	if((ISoundSample*)m_sample == (ISoundSample*)&zeroSample)
	{
		m_sample = NULL;
		return;
	}

	m_sample = (DkSoundSampleLocal*)(sample);
}

ISoundSample* DkSoundEmitterLocal::GetSample() const
{
	return m_sample;
}

void DkSoundEmitterLocal::UpdateParams()
{
	if (m_nChannel < 0)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];
	chnl->emitter = this;

	alSourcefv(chnl->alSource, AL_POSITION, vPosition);
	alSourcef(chnl->alSource, AL_REFERENCE_DISTANCE, m_params.referenceDistance);
	alSourcef(chnl->alSource, AL_MAX_DISTANCE, m_params.maxDistance);
	alSourcef(chnl->alSource, AL_ROLLOFF_FACTOR, m_params.rolloff);
	alSourcef(chnl->alSource, AL_AIR_ABSORPTION_FACTOR, m_params.airAbsorption);
	alSourcef(chnl->alSource, AL_GAIN, m_params.volume);
	alSourcef(chnl->alSource, AL_PITCH, m_params.pitch);
}

bool DkSoundEmitterLocal::SelfAssignChannel()
{
	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	if(m_nChannel < 0)
		m_nChannel = pSoundSystem->RequestChannel( this );

	if(m_nChannel >= 0)
	{
		sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];
		chnl->emitter = this;
		chnl->lastPauseOffsetBytes = 0;

		// wait for sample
		m_sample->WaitForLoad();

		alSourcei(chnl->alSource, AL_BUFFER, m_sample->m_alBuffer);

		alSourcei(chnl->alSource, AL_LOOPING, (m_sample->m_flags & SAMPLE_FLAG_LOOPING) > 0 ? AL_TRUE : AL_FALSE);

		alSourceRewind(chnl->alSource);

		alSourcefv(chnl->alSource, AL_VELOCITY, vec3_zero);
		alSourcei(chnl->alSource, AL_SAMPLE_OFFSET, 0);

		UpdateParams();

		return true;
	}

	return false;
}

bool DkSoundEmitterLocal::DropChannel()
{
	if(m_nChannel < 0)
		return false;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t *chnl = pSoundSystem->m_channels[m_nChannel];
	chnl->lastPauseOffsetBytes = 0;

	// stop the sound
	alSourceStop(chnl->alSource);

	alSourceRewind(chnl->alSource);
	alSourcei(chnl->alSource, AL_SAMPLE_OFFSET, 0);
	alSourcei(chnl->alSource, AL_BUFFER, AL_NONE);

	chnl->alState = AL_STOPPED;
	chnl->emitter = NULL;
	m_nChannel = -1;

	return true;
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

			sndChannel_t *chnl = pSoundSystem->m_channels[m_nChannel];
			
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

	if(!HasToBeVirtual() && SelfAssignChannel() )
	{
		sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];

		alSourcePlay(chnl->alSource);

		chnl->alState = AL_PLAYING;
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
	bool hadChannel = DropChannel();

	if(!hadChannel && m_virtual)
	{
		// exit from virtual mode
		m_virtualState = AL_STOPPED;
		m_virtual = false;
	}
}

void DkSoundEmitterLocal::StopLoop()
{
	if(m_nChannel >= 0)
	{
		DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

		sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];
		alSourcei(chnl->alSource, AL_LOOPING, AL_FALSE );
	}
}

void DkSoundEmitterLocal::Pause()
{
	if(m_nChannel >= 0)
	{
		DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

		sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];

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
	bool virtualLooped = m_virtual && (m_sample->m_flags & SAMPLE_FLAG_LOOPING) && (m_virtualState == AL_STOPPED);

	return(m_nChannel < 0) && !virtualLooped;
}

ESoundState DkSoundEmitterLocal::GetState() const
{
	if(IsStopped())
		return SOUND_STATE_STOPPED;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];

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

bool DkSoundEmitterLocal::HasToBeVirtual()
{
	if ((length(vPosition - soundsystem->GetListenerPosition())) > m_params.maxDistance)
		return true;

	return false;
}

void DkSoundEmitterLocal::Update()
{
	if(m_nChannel == -1)
	{
		if(m_virtual && m_virtualState == AL_PLAYING)
			Play();
	}
	else if(m_sample && HasToBeVirtual())
	{
		DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

		sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];

		// free channel and switch to virtual mode
		if (chnl->alState == AL_PLAYING)
		{
			Stop();

			m_virtual = true;
			m_virtualState = AL_PLAYING;
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

	sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];

	alSourcef(chnl->alSource,AL_GAIN, m_params.volume);
}


void DkSoundEmitterLocal::SetPitch(float val)
{
	m_params.pitch = val;

	if(m_nChannel == -1)
		return;

	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	sndChannel_t* chnl = pSoundSystem->m_channels[m_nChannel];

	alSourcef(chnl->alSource,AL_PITCH, m_params.pitch);
}

void DkSoundEmitterLocal::SetParams(soundParams_t *param)
{
	m_params = *param;

	UpdateParams();
}
