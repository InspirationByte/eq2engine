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
	m_flPitch(1.0f),
	m_flVolume(1.0f),
	m_vOrigin(0.0f),
	m_flAttenuation(ATTN_STATIC),
	m_sourceMixer(NULL)
{
	
}

int CSoundChannel::PlaySound( int nSound, bool bLooping )
{
	if ( nSound < 0 )
		return -1;

	m_pSound = gSound->GetSound( nSound );

	if ( !m_pSound )
	{
		MsgError( "could not play sound %i: does not exist\n", nSound );
		return -1;
	}

	// select the mixer function
	soundFormat_t* format = m_pSound->GetFormat();

	if ( format->channels == 1 && format->bitwidth == 16 )
		m_sourceMixer = S_MixMono16;
	else if ( format->channels == 2 && format->bitwidth == 16 )
		m_sourceMixer = S_MixStereo16;

	m_nSamplePos = 0;
	m_bPlaying = true;
	m_bLooping = bLooping;

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

void CSoundChannel::StopSound ()
{
	m_bPlaying = false;
	m_bLooping = false;
}

//----------------------------------------------------------------

void CSoundChannel::MixChannel(paintbuffer_t* input, paintbuffer_t* output, int numSamples)
{
	soundFormat_t* format = m_pSound->GetFormat();

	float sampleRate = m_flPitch * (float)format->frequency / (float)output->nFrequency;

	// read needed samples
	int readSampleCount = numSamples*sampleRate;
	int readSamples = m_pSound->GetSamples( (ubyte *)input->pData, readSampleCount + SAMPLES_THRESH, floor(m_nSamplePos), m_bLooping );

	int volume = (int)(m_flVolume * output->nVolume * 255) >> 8;

	// spatialize sound
	int spatial_vol[2];
	S_SpatializeStereo(this, gSound->GetListener(), volume, spatial_vol);

	// mix them into given output buffer
	float sampleOffset = (*m_sourceMixer)( input->pData, readSamples, output->pData, numSamples, sampleRate, spatial_vol );

	if ( readSamples < readSampleCount ) // stop sound if we read less samples than we wanted
		m_bPlaying = false;

	// advance playback
	m_nSamplePos = m_pSound->GetLoopPosition( m_nSamplePos+sampleOffset );
}