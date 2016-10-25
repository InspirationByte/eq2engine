//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: portable code to mix sounds by snd_dma and snd_channel
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_MIX_H
#define SND_MIX_H

#include "snd_defs_public.h"

class ISoundChannel;

typedef float (*MIXFUNC)(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int* volume);

float	S_MixMono16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int* volume);
float	S_MixStereo16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int* volume);

void	S_SpatializeMono(ISoundChannel* chan, const ListenerInfo& listener, int in, int *out);
void	S_SpatializeStereo(ISoundChannel* chan, const ListenerInfo& listener, int in, int out[]);


#endif // SND_MIX_H