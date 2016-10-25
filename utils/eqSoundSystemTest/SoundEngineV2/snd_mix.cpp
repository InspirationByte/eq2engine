//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: portable code to mix sounds by snd_dma and snd_channel
//////////////////////////////////////////////////////////////////////////////////

#include "snd_mix.h"
#include "snd_channel.h"

#pragma warning(disable:4244)

#define SAMPLE_M(a,b,c)   ( (float)(a[b]*(1-c))+(float)(a[b+1]*(c)) )
#define SAMPLE_S(a,b,c,d) ( (float)(a[c].b*(1-d))+(float)(a[c+1].b*(d)) )

#define ATTN_LEN        1000.0f

float S_MixMono16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int* volume)
{
	short* input = (short*)in;
	samplepair_t* output = (samplepair_t*)out;

	float nSamplePos = 0.0f;
	float sampleFrac = 0.0f;

	int i = 0;
	int srcSample;

	while( i < numOutSamples && nSamplePos < numInSamples )  // don't overdo sample count
	{
		srcSample = floor( nSamplePos );
		sampleFrac = nSamplePos-srcSample;

		output[i].left += (int)(volume[0] * SAMPLE_M(input,srcSample,sampleFrac)) >> 8;
		output[i].right += (int)(volume[1] * SAMPLE_M(input,srcSample,sampleFrac)) >> 8;

		nSamplePos += sampleRate;
		i++;
	}

	return nSamplePos;
}

float S_MixStereo16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int* volume)
{
	stereo16_t* input = (stereo16_t*)in;
	samplepair_t* output = (samplepair_t*)out;

	float nSamplePos = 0.0f;
	float sampleFrac = 0.0f;

	int i = 0;
	int srcSample;
	while( i < numOutSamples && nSamplePos < numInSamples ) // don't overdo sample count
	{
		srcSample = floor( nSamplePos );
		sampleFrac = nSamplePos-srcSample;

		output[i].left += (int)((float)volume[0] * SAMPLE_S(input,left,srcSample,sampleFrac)) >> 8;
		output[i].right += (int)((float)volume[1] * SAMPLE_S(input,right,srcSample,sampleFrac)) >> 8;

		nSamplePos += sampleRate;
		i++;
	}

	return nSamplePos;
}

//----------------------------------------------------------------

void S_SpatializeMono(ISoundChannel* chan, const ListenerInfo& listener, int in, int *out)
{
	float atten = chan->GetAttenuation();

	if ( atten == ATTN_STATIC )
	{
		out[0] = out[1] = in;
	}
	else
	{
		Vector3D vDir = listener.origin - chan->GetOrigin();
		float len = lengthSqr(vDir);

		float attn = clamp( powf(ATTN_LEN / len, atten), 0.0f, 1.0f );
		out[0] = out[1] = in * attn;
	}
}

void S_SpatializeStereo(ISoundChannel* chan, const ListenerInfo& listener, int in, int out[])
{
	float atten = chan->GetAttenuation();

	if ( atten == ATTN_STATIC )
	{
		out[0] = in;
		out[1] = in;
	}
	else
	{
		Vector3D vDir = listener.origin - chan->GetOrigin();
		float len = lengthSqr(vDir);

		vDir = normalize(vDir);

		float dotRight = dot(vDir, listener.right );

		float attn = clamp( powf(ATTN_LEN / len, atten), 0.0f, 1.0f );

		out[0] = in * (0.5f * (1.0f + dotRight) * attn);
		out[1] = in * (0.5f * (1.0f - dotRight) * attn);
	}
}
