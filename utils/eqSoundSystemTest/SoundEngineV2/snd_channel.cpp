//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine channel
//////////////////////////////////////////////////////////////////////////////////

#include "snd_channel.h"
#include "snd_dma.h"
#include "snd_source.h"

#include "DebugInterface.h"


#pragma warning(disable:4244)

#define SAMPLES_THRESH	8		// read some extra samples

CSoundChannel::CSoundChannel() : 
	m_pitch(1.0f),
	m_volume(1.0f),
	m_origin(0.0f),
	m_attenuation(ATTN_STATIC),
	m_sourceMixer(NULL),
	m_spatialize(S_SpatializeStereo),
	m_playbackState(false),
	m_looping(false),
	m_reserved(false)
{
	
}

int CSoundChannel::PlaySound( int soundId, bool loop )
{
	if ( soundId < 0 )
		return -1;

	ISoundSource* source = m_owner->GetSound( soundId );

	if ( !source )
	{
		MsgError( "could not play sound %i: does not exist\n", soundId );
		return -1;
	}

	if(SetupSource(source))
	{
		m_playbackState = true;
		m_looping = loop;
	}
	else
		m_playbackState = false;

	return 0;
}

int CSoundChannel::PlaySound(int nSound)
{
	return PlaySound( nSound, false );
}

int CSoundChannel::PlayLoop(int nSound)
{
	return PlaySound( nSound, true );
}

void CSoundChannel::StopSound()
{
	m_playbackState = false;
	m_looping = false;
	m_source = NULL;
}

//----------------------------------------------------------------

bool CSoundChannel::SetupSource(ISoundSource* source)
{
	m_playbackSamplePos = 0.0f;

	m_source = source;

	// select proper mix format
	soundFormat_t* format = m_source->GetFormat();

	m_sourceMixer = NULL; // reset before proceed

	if ( format->channels == 1 && format->bitwidth == 8 )
		m_sourceMixer = S_MixMono8;
	else if ( format->channels == 1 && format->bitwidth == 16 )
		m_sourceMixer = S_MixMono16;
	else if ( format->channels == 2 && format->bitwidth == 16 )
		m_sourceMixer = S_MixStereo16;

	if(!m_sourceMixer)
	{
		MsgError("Unsupported sound format for '%s'\n", m_source->GetFilename());
		m_source = NULL;
		return false;
	}

	return true;
}

void CSoundChannel::MixChannel(paintbuffer_t* input, paintbuffer_t* output, int numSamples)
{
	soundFormat_t* format = m_source->GetFormat();

	float sampleRate = m_pitch * (float)format->frequency / (float)output->frequency;

	// read needed samples
	int readSampleCount = numSamples*sampleRate;

	// get rid of clicks if pitch is not uniform
	// FIXME: not properly working with streaming Ogg
	if(sampleRate != m_pitch)
		readSampleCount += SAMPLES_THRESH;

	int readSamples = m_source->GetSamples( (ubyte *)input->data, readSampleCount, floor(m_playbackSamplePos), m_looping );

	int volume = (int)(m_volume * output->volume * 255) >> 8;

	// spatialize sound
	int spatial_vol[2];
	(*m_spatialize)(this, m_owner->GetListener(), volume, spatial_vol);

	// mix them into given output buffer
	float sampleOffset = (*m_sourceMixer)( input->data, readSamples, output->data, numSamples, sampleRate, spatial_vol );

	if ( readSamples < readSampleCount ) // stop sound if we read less samples than we wanted
		m_playbackState = false;

	// advance playback
	m_playbackSamplePos = m_source->GetLoopPosition( m_playbackSamplePos+sampleOffset );
}