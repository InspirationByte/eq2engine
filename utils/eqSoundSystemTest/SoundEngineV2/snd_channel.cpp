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

#define SAMPLE_M(a,b,c)   ( (float)(a[b]*(1-c))+(float)(a[b+1]*(c)) )
#define SAMPLE_S(a,b,c,d) ( (float)(a[c].b*(1-d))+(float)(a[c+1].b*(d)) )

#define SAMPLES_THRESH	8		// read some extra samples

#define ATTN_LEN        1000.0f

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
        m_sourceMixer = &CSoundChannel::m_mixMono16;
    else if ( format->channels == 2 && format->bitwidth == 16 )
        m_sourceMixer = &CSoundChannel::m_mixStereo16;

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

void CSoundChannel::MixChannel(paintbuffer_t *pBuffer, int nSamples)
{
    int volume = (int)(m_flVolume * pBuffer->nVolume * 255) >> 8;

	soundFormat_t* format = m_pSound->GetFormat();

	float sampleRate = m_flPitch * (float)format->frequency / (float)pBuffer->nFrequency;

	// mix
	(this->*m_sourceMixer)( pBuffer->pData, sampleRate, volume, nSamples );
}

void CSoundChannel::m_mixMono16(void *pBuffer, float flRate, int nVolume, int nSamples)
{
    samplepair_t* output = (samplepair_t*)pBuffer;
    short* input = (short *)gSound->GetChannelBuffer( )->pData;

    float sampleFrac = 0.0f;

    int spatial_vol[2];
    m_SpatializeStereo( nVolume, spatial_vol );

	int readSampleCount = nSamples*flRate;

    int nSamplesRead = m_pSound->GetSamples( (ubyte *)input, readSampleCount + SAMPLES_THRESH, m_nSamplePos, m_bLooping );
    float nSamplePos = m_nSamplePos - floor( m_nSamplePos );

	int i = 0;
	int srcSample;
	while( i < nSamples && nSamplePos < nSamplesRead )  // don't overdo sample count
    {
        srcSample = floor( nSamplePos );
        sampleFrac = nSamplePos-srcSample;

        output[i].left += (int)(spatial_vol[0] * SAMPLE_M(input,srcSample,sampleFrac)) >> 8;
        output[i].right += (int)(spatial_vol[1] * SAMPLE_M(input,srcSample,sampleFrac)) >> 8;

        m_nSamplePos += flRate;
        nSamplePos += flRate;
		i++;
    }

    if ( nSamplesRead < readSampleCount )
        m_bPlaying = false;

    m_nSamplePos = m_pSound->GetLoopPosition( m_nSamplePos );
}

void CSoundChannel::m_mixStereo16(void *pBuffer, float flRate, int nVolume, int nSamples)
{
    samplepair_t* output = (samplepair_t*)pBuffer;
    stereo16_t* input = (stereo16_t*)gSound->GetChannelBuffer()->pData;

    int spatial_vol[2];
    m_SpatializeStereo( nVolume, spatial_vol );

	int readSampleCount = nSamples*flRate;

    int nSamplesRead = m_pSound->GetSamples( (ubyte *)input, readSampleCount + SAMPLES_THRESH, m_nSamplePos, m_bLooping );

    float nSamplePos = m_nSamplePos - floor( m_nSamplePos );
	float sampleFrac = 0.0f;

	int i = 0;
	int srcSample;
	while( i < nSamples && nSamplePos < nSamplesRead ) // don't overdo sample count
    {
        srcSample = floor( nSamplePos );
        sampleFrac = nSamplePos-srcSample;

        output[i].left += (int)((float)spatial_vol[0] * SAMPLE_S(input,left,srcSample,sampleFrac)) >> 8;
        output[i].right += (int)((float)spatial_vol[1] * SAMPLE_S(input,right,srcSample,sampleFrac)) >> 8;

        m_nSamplePos += flRate;
        nSamplePos += flRate;
		i++;
    }

    if ( nSamplesRead < readSampleCount )
        m_bPlaying = false;

    m_nSamplePos = m_pSound->GetLoopPosition( m_nSamplePos );
}

//----------------------------------------------------------------

void CSoundChannel::m_SpatializeMono (int in, int *out)
{
    if ( m_flAttenuation == ATTN_STATIC )
        out[0] = in;
    else
    {
        Vector3D vDir = gSound->m_listener.origin-m_vOrigin;
        float len = lengthSqr(vDir);

		// BUG_WARNING

        float attn = clamp( powf(ATTN_LEN / len, m_flAttenuation), 0, 1 );
        out[0] = in * attn;
    }
}

void CSoundChannel::m_SpatializeStereo (int in, int out[])
{
    if ( m_flAttenuation == ATTN_STATIC )
    {
        out[0] = in;
        out[1] = in;
    }
    else
    {
        Vector3D vDir = gSound->m_listener.origin-m_vOrigin;
        float len = lengthSqr(vDir);

		vDir = normalize(vDir);

		float dotRight = dot(vDir, gSound->m_listener.right );

        float attn = clamp( powf(ATTN_LEN / len, m_flAttenuation), 0, 1 );

        out[0] = in * (0.5f * (1 + dotRight) * attn);
        out[1] = in * (0.5f * (1 - dotRight) * attn);
    }
}
