#pragma once

struct Mixer
{
	static int MixMono8(float volume, const uint8* in, int numInSamples, short* out, int numOutSamples);
	static int MixStereo8(float volume, const uint8* in, int numInSamples, short* out, int numOutSamples);
	static int MixMono16(float volume, const short* in, int numInSamples, short* out, int numOutSamples);
	static int MixStereo16(float volume, const short* in, int numInSamples, short* out, int numOutSamples);
};