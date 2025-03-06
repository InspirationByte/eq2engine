#pragma once

using SoundMixFunc = int (*)(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate);

struct Mixer
{
	static int MixMono8(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate);
	static int MixStereo8(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate);
	static int MixMono16(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate);
	static int MixStereo16(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate);
};