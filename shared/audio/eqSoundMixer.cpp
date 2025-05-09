#include "core/core_common.h"
#include "eqSoundMixer.h"

struct int16stereo
{
	int16 l;
	int16 r;
};

struct uint8stereo
{
	uint8 l;
	uint8 r;
};

using f32stereo = Vector2D;

template<typename T2>
float sampleValueMono(const T2 src) { return src; }

// special case for 8 bit fmt
template<> float sampleValueMono(const uint8 src) { return float(src) * 256 - SHRT_MAX; }

template<typename T2>
f32stereo sampleValueStereo(const T2 src) { return { sampleValueMono(src.l), sampleValueMono(src.r) }; }

template<typename OUT>
void mixSample(const float in, const float sampleFrac, OUT& out);

template<typename OUT>
void mixSample(const f32stereo in, const float sampleFrac, OUT& out);

template<>
void mixSample(const float in, const float sampleFrac, int16& out)
{
	const int result = (((SHRT_MAX - out) * in) / SHRT_MAX) + out;
	out = clamp(result, SHRT_MIN, SHRT_MAX);
}

template<>
void mixSample(const f32stereo in, const float sampleFrac, int16stereo& out)
{
	mixSample(in.x, sampleFrac, out.l);
	mixSample(in.y, sampleFrac, out.r);
}

template<typename OUT, typename IN>
int MixSamplesMono(const IN* in, int numInSamples, OUT* out, int numOutSamples, float volume, float rate)
{
	float samplePos = 0.0f;
	int i = 0;
	for (; i < numOutSamples && samplePos < numInSamples; ++i)
	{
		const int srcSamplePos = floorf(samplePos);
		const float sampleFrac = samplePos - floorf(samplePos);

		const float srcValA = sampleValueMono(in[srcSamplePos]);
		const float srcValB = sampleValueMono(in[min(numInSamples - 1, srcSamplePos + 1)]);
		const float sourceVal = lerp(srcValA, srcValB, sampleFrac) * volume;

		mixSample(sourceVal, sampleFrac, out[i]);

		samplePos += rate;
	}

	return i + 1;
}

template<typename OUT, typename IN>
int MixSamplesStereo(const IN* in, int numInSamples, OUT* out, int numOutSamples, float volume, float rate)
{
	float samplePos = 0.0f;
	int i = 0;
	for (; i < numOutSamples && samplePos < numInSamples; ++i)
	{
		const int srcSamplePos = floorf(samplePos);
		const float sampleFrac = samplePos - floorf(samplePos);

		const f32stereo srcValA = sampleValueStereo(in[srcSamplePos]);
		const f32stereo srcValB = sampleValueStereo(in[min(numInSamples - 1, srcSamplePos + 1)]);
		const f32stereo sourceVal = lerp(srcValA, srcValB, sampleFrac) * volume;

		mixSample(sourceVal, sampleFrac, out[i]);

		samplePos += rate;
	}

	return i + 1;
}

// mix 8 bit into 16 bit mono sound
int Mixer::MixMono8(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate)
{
	return MixSamplesMono(reinterpret_cast<const uint8*>(in), numInSamples, reinterpret_cast<int16*>(out), numOutSamples, volume, rate);
}

// mix 8 bit stereo into 16 bit stereo
int Mixer::MixStereo8(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate)
{
	return MixSamplesStereo(reinterpret_cast<const uint8stereo*>(in), numInSamples, reinterpret_cast<int16stereo*>(out), numOutSamples, volume, rate);
}

// mix 16 bit mono into 16 bit mono sound
int Mixer::MixMono16(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate)
{
	return MixSamplesMono(reinterpret_cast<const int16*>(in), numInSamples, reinterpret_cast<int16*>(out), numOutSamples, volume, rate);
}

// mix 16 bit stereo into 16 bit stereo
int Mixer::MixStereo16(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate)
{
	return MixSamplesStereo(reinterpret_cast<const int16stereo*>(in), numInSamples, reinterpret_cast<int16stereo*>(out), numOutSamples, volume, rate);
}