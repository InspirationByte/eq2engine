#include "core/core_common.h"
#include "eqSoundMixer.h"

// mix 8 bit into 16 bit mono sound
int Mixer::MixMono8(float volume, const uint8* in, int numInSamples, short* out, int numOutSamples)
{
	const int maxSamples = min(numInSamples, numOutSamples);
	for (int i = 0; i < maxSamples; ++i)
	{
		const float src_val = ((short)in[i] * 256 - SHRT_MAX) * volume;
		const int result = (((SHRT_MAX - out[i]) * src_val) / SHRT_MAX) + out[i];
		out[i] = clamp(result, SHRT_MIN, SHRT_MAX);
	}

	return maxSamples;
}

// mix 8 bit stereo into 16 bit stereo
int Mixer::MixStereo8(float volume, const uint8* in, int numInSamples, short* out, int numOutSamples)
{
	const int maxSamples = 2 * min(numInSamples, numOutSamples);
	for (int i = 0; i < maxSamples; ++i)
	{
		const float src_val = ((short)in[i] * 256 - SHRT_MAX) * volume;
		const int result = (((SHRT_MAX - out[i]) * src_val) / SHRT_MAX) + out[i];
		out[i] = clamp(result, SHRT_MIN, SHRT_MAX);
	}

	return maxSamples;
}

// mix 16 bit mono into 16 bit mono sound
int Mixer::MixMono16(float volume, const short* in, int numInSamples, short* out, int numOutSamples)
{
	const int maxSamples = min(numInSamples, numOutSamples);
	for (int i = 0; i < maxSamples; ++i)
	{
		const float src_val = in[i] * volume;
		const int result = (((SHRT_MAX - out[i]) * src_val) / SHRT_MAX) + out[i];
		out[i] = clamp(result, SHRT_MIN, SHRT_MAX);
	}

	return maxSamples;
}

// mix 16 bit stereo into 16 bit stereo
int Mixer::MixStereo16(float volume, const short* in, int numInSamples, short* out, int numOutSamples)
{
	const int maxSamples = 2 * min(numInSamples, numOutSamples);
	for (int i = 0; i < maxSamples; ++i)
	{
		const float src_val = in[i] * volume;
		const int result = (((SHRT_MAX - out[i]) * src_val) / SHRT_MAX) + out[i];
		out[i] = clamp(result, SHRT_MIN, SHRT_MAX);
	}

	return maxSamples;
}