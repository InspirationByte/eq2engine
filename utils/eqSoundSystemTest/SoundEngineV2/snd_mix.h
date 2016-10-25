//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: portable code to mix sounds by snd_dma and snd_channel
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_MIX_H
#define SND_MIX_H

#include "snd_defs_public.h"

class CSoundChannel;

typedef float	(*S_MIXFUNC)(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int volume[]);
typedef void	(*S_SPATIALFUNC)(CSoundChannel* chan, const ListenerInfo& listener, int in, int out[]);

// mix
float	S_MixMono8(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int volume[]);
float	S_MixMono16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int volume[]);
float	S_MixStereo16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int volume[]);
float	S_MixStereoToMono16(void* in, int numInSamples, void* out, int numOutSamples, float sampleRate, int volume[]);

// spatial methods
void	S_SpatializeMono(CSoundChannel* chan, const ListenerInfo& listener, int in, int out[]);
void	S_SpatializeStereo(CSoundChannel* chan, const ListenerInfo& listener, int in, int out[]);


#endif // SND_MIX_H