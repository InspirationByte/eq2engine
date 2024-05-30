#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

#include "core/core_common.h"

#include "eqSoundCommonAL.h"
#include "eqAudioSourceAL.h"
#include "eqAudioSystemAL.h"
#include "eqSoundMixer.h"

#include "source/snd_al_source.h"

#define EQSND_SILENCE_SIZE			128
#define EQSND_STREAM_BUFFER_SIZE	(1024*8) // 8 kb

static const short _silence[EQSND_SILENCE_SIZE] = { 0 };

static int GetLoopRegionIdx(int offsetInSamples, int* points, int regionCount)
{
	for (int i = 0; i < regionCount; ++i)
	{
		if (offsetInSamples >= points[i * 2]) //&& offsetInSamples <= points[i*2+1])
			return i;
	}
	return -1;
}

static int WrapAroundSampleOffset(int sampleOffset, const ISoundSource* sample, bool looping)
{
	const int sampleCount = sample->GetSampleCount();

	if (looping)
	{
		int loopPoints[SOUND_SOURCE_MAX_LOOP_REGIONS * 2];
		const int numLoopRegions = sample->GetLoopRegions(loopPoints);

		const int loopRegionIdx = GetLoopRegionIdx(sampleOffset, loopPoints, numLoopRegions);
		const int sampleMin = (loopRegionIdx == -1) ? 0 : loopPoints[loopRegionIdx * 2];
		const int sampleMax = (loopRegionIdx == -1) ? sampleCount : loopPoints[loopRegionIdx * 2 + 1];

		const int sampleRange = sampleMax - sampleMin;

		if (sampleRange > 0)
			sampleOffset = sampleMin + ((sampleOffset - sampleMin) % sampleRange);
		else
			sampleOffset = sampleMin;
	}
	else
		sampleOffset = min(sampleOffset, sampleCount);

	return sampleOffset;
}


//----------------------------------------------------------------------------------------------
// Sound source
//----------------------------------------------------------------------------------------------

CEqAudioSourceAL::CEqAudioSourceAL(CEqAudioSystemAL* owner) 
	: m_owner(owner)
{
}

CEqAudioSourceAL::~CEqAudioSourceAL()
{
	Release();
}

// Updates channel with user parameters
void CEqAudioSourceAL::UpdateParams(const Params& params, int overrideUpdateFlags)
{
	int mask = overrideUpdateFlags == -1 ? params.updateFlags : overrideUpdateFlags;

	// apply update flags from mixer
	if (mask & UPDATE_CHANNEL)
	{
		m_channel = params.channel;
		mask &= ~UPDATE_CHANNEL;
	}

	CEqAudioSystemAL::MixerChannel mixChannel;

	const int channel = m_channel;
	if (m_owner->m_mixerChannels.inRange(channel))
		mixChannel = m_owner->m_mixerChannels[channel];

	mask |= mixChannel.updateFlags;

	if (mask == 0)
		return;

	const ALuint thisSource = m_source;

	// is that source needs setup again?
	if (thisSource == 0)
		return;

	ALuint qbuffer;
	int numQueued;

	if (mask & UPDATE_POSITION)
		alSourcefv(thisSource, AL_POSITION, params.position);

	if (mask & UPDATE_VELOCITY)
		alSourcefv(thisSource, AL_VELOCITY, params.velocity);

	if (mask & UPDATE_DIRECTION)
		alSourcefv(thisSource, AL_DIRECTION, params.direction);

	if (mask & UPDATE_CONE_ANGLES)
	{
		alSourcef(thisSource, AL_CONE_INNER_ANGLE, params.coneAngles.x);
		alSourcef(thisSource, AL_CONE_OUTER_ANGLE, params.coneAngles.y);
	}

	if(params.updateFlags & UPDATE_VOLUME)
		m_volume = params.volume;

	if (params.updateFlags & UPDATE_PITCH)
		m_pitch = params.pitch;

	if (mask & UPDATE_VOLUME)
	{
		alSourcef(thisSource, AL_GAIN, m_volume.x * mixChannel.volume);
		alSourcef(thisSource, AL_CONE_OUTER_GAIN, m_volume.y);
		alSourcef(thisSource, AL_CONE_OUTER_GAINHF, m_volume.z);
	}

	if (mask & UPDATE_PITCH)
		alSourcef(thisSource, AL_PITCH, m_pitch * mixChannel.pitch);

	if (mask & UPDATE_REF_DIST)
		alSourcef(thisSource, AL_REFERENCE_DISTANCE, params.referenceDistance);

	if (mask & UPDATE_AIRABSORPTION)
		alSourcef(thisSource, AL_AIR_ABSORPTION_FACTOR, params.airAbsorption);

	if (mask & UPDATE_ROLLOFF)
		alSourcei(thisSource, AL_ROLLOFF_FACTOR, params.rolloff);

	if (mask & UPDATE_EFFECTSLOT)
	{
		if (params.effectSlot < 0)
			alSource3i(thisSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
		else
			alSource3i(thisSource, AL_AUXILIARY_SEND_FILTER, m_owner->m_effectSlots[params.effectSlot], 0, AL_FILTER_NULL);
	}

	if (mask & UPDATE_BANDPASS)
	{
		if (!m_filter)
		{
			GetAlExt().alGenFilters(1, &m_filter);
			ALCheckError("gen buffers");

			GetAlExt().alFilteri(m_filter, AL_FILTER_TYPE, AL_FILTER_BANDPASS);
			GetAlExt().alFilterf(m_filter, AL_BANDPASS_GAIN, 1.0f);
		}

		GetAlExt().alFilterf(m_filter, AL_BANDPASS_GAINLF, params.bandPass.x);
		GetAlExt().alFilterf(m_filter, AL_BANDPASS_GAINHF, params.bandPass.y);

		alSourcei(thisSource, AL_DIRECT_FILTER, m_filter);
	}

	if (mask & UPDATE_RELATIVE)
	{
		const int tempValue = params.relative == true ? AL_TRUE : AL_FALSE;
		alSourcei(thisSource, AL_SOURCE_RELATIVE, tempValue);

		// temporary enable direct channels on relative sources (as spec says it will be only for non-mono sources)
		alSourcei(thisSource, AL_DIRECT_CHANNELS_SOFT, tempValue);
	}

	const bool isStreaming = IsStreamed();

	if (mask & UPDATE_LOOPING)
	{
		m_looping = params.looping;

		if (!isStreaming)
			alSourcei(thisSource, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);
	}

	if (mask & UPDATE_DO_REWIND)
	{
		for (int i = 0; i < m_streams.numElem(); ++i)
			m_streams[i].curPos = 0;

#ifdef USE_ALSOFT_BUFFER_CALLBACK
		if(!GetAlExt().alBufferCallbackSOFT && !isStreaming)
#else
		if (!isStreaming)
#endif
		{
			alSourceRewind(thisSource);
		}
	}

	if (mask & UPDATE_RELEASE_ON_STOP)
		m_releaseOnStop = params.releaseOnStop;

	// change state
	if (mask & UPDATE_STATE)
	{
		if (params.state == STOPPED)
		{
			alSourceStop(thisSource);
		}
		else if (params.state == PAUSED)
		{
			// HACK: make source armed
			if (m_state != PLAYING)
				alSourcePlay(thisSource);

			alSourcePause(thisSource);
		}
		else if (params.state == PLAYING)
		{
			// re-queue stream buffers
			if (isStreaming)
			{
				alSourceStop(thisSource);

				// first dequeue buffers
				numQueued = 0;
				alGetSourcei(thisSource, AL_BUFFERS_QUEUED, &numQueued);

				while (numQueued--)
					alSourceUnqueueBuffers(thisSource, 1, &qbuffer);

				for (int i = 0; i < EQSND_STREAM_BUFFER_COUNT; i++)
				{
					if (!QueueStreamChannel(m_buffers[i]))
						break; // too short
				}
				ALCheckError("queue buffers");
			}

			alSourcePlay(thisSource);
		}

		m_state = params.state;
	}

	ALCheckError("source update");
}

void CEqAudioSourceAL::SetSamplePlaybackPosition(int sourceIdx, float seconds)
{
	if (sourceIdx == -1)
	{
		for (int i = 0; i < m_streams.numElem(); ++i)
		{
			const ISoundSource::Format& fmt = m_streams[i].sample->GetFormat();
			m_streams[i].curPos = WrapAroundSampleOffset(seconds * fmt.frequency, m_streams[i].sample, m_looping);
		}
		return;
	}

	if (!m_streams.inRange(sourceIdx))
		return;
	const ISoundSource::Format& fmt = m_streams[sourceIdx].sample->GetFormat();
	m_streams[sourceIdx].curPos = WrapAroundSampleOffset(seconds * fmt.frequency, m_streams[sourceIdx].sample, m_looping);
}

float CEqAudioSourceAL::GetSamplePlaybackPosition(int sourceIdx) const
{
	if (m_streams.inRange(sourceIdx))
	{
		const ISoundSource::Format& fmt = m_streams[sourceIdx].sample->GetFormat();
		return m_streams[sourceIdx].curPos / fmt.frequency;
	}
	return 0.0f;
}

void CEqAudioSourceAL::SetSampleVolume(int sourceIdx, float volume)
{
	if (sourceIdx == -1)
	{
		for (int i = 0; i < m_streams.numElem(); ++i)
			m_streams[i].volume = volume;

		return;
	}

	if(m_streams.inRange(sourceIdx))
		m_streams[sourceIdx].volume = volume;
}

float CEqAudioSourceAL::GetSampleVolume(int sourceIdx) const
{
	if (m_streams.inRange(sourceIdx))
		return m_streams[sourceIdx].volume;
	return 0.0f;
}

void CEqAudioSourceAL::SetSamplePitch(int sourceIdx, float pitch)
{
	if (sourceIdx == -1)
	{
		for (int i = 0; i < m_streams.numElem(); ++i)
			m_streams[i].pitch = pitch;

		return;
	}

	if (m_streams.inRange(sourceIdx))
		m_streams[sourceIdx].pitch = pitch;
}

float CEqAudioSourceAL::GetSamplePitch(int sourceIdx) const
{
	if (m_streams.inRange(sourceIdx))
		return m_streams[sourceIdx].volume;
	return 0.0f;
}

int	CEqAudioSourceAL::GetSampleCount() const
{
	return m_streams.numElem();
}

void CEqAudioSourceAL::GetParams(Params& params) const
{
	const ALuint thisSource = m_source;

	int sourceState;
	int tempValue;

	if (thisSource == AL_NONE)
		return;

	params.channel = m_channel;

	const bool isStreaming = IsStreamed();

	// get current state of alSource
	alGetSourcefv(thisSource, AL_POSITION, params.position);
	alGetSourcefv(thisSource, AL_VELOCITY, params.velocity);
	params.volume = m_volume;
	params.pitch = m_pitch;
	alGetSourcef(thisSource, AL_REFERENCE_DISTANCE, &params.referenceDistance);
	alGetSourcef(thisSource, AL_ROLLOFF_FACTOR, &params.rolloff);
	alGetSourcef(thisSource, AL_AIR_ABSORPTION_FACTOR, &params.airAbsorption);

	params.looping = m_looping;

	alGetSourcei(thisSource, AL_SOURCE_RELATIVE, &tempValue);
	params.relative = (tempValue == AL_TRUE);

	if (m_filter != AL_NONE)
	{
		GetAlExt().alGetFilterf(m_filter, AL_BANDPASS_GAINLF, &params.bandPass.x);
		GetAlExt().alGetFilterf(m_filter, AL_BANDPASS_GAINLF, &params.bandPass.y);
	}

	if (isStreaming)
	{
		// continuous; use channel state
		params.state = m_state;
	}
	else
	{
		alGetSourcei(thisSource, AL_SOURCE_STATE, &sourceState);

		// use AL state
		if (sourceState == AL_INITIAL || sourceState == AL_STOPPED)
			params.state = STOPPED;
		else if (sourceState == AL_PLAYING)
			params.state = PLAYING;
		else if (sourceState == AL_PAUSED)
			params.state = PAUSED;
	}

	params.releaseOnStop = m_releaseOnStop;
}

void CEqAudioSourceAL::Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback /*= nullptr*/)
{
	Release();
	InitSource();

	m_callback = fnCallback;
	m_channel = chanId;
	m_releaseOnStop = !m_callback;

	SetupSample(sample);
}

void CEqAudioSourceAL::Setup(int chanId, ArrayCRef<const ISoundSource*> samples, UpdateCallback fnCallback /*= nullptr*/)
{
	Release();
	InitSource();

	m_callback = fnCallback;
	m_channel = chanId;
	m_releaseOnStop = !m_callback;

	SetupSamples(samples);
}

bool CEqAudioSourceAL::IsStreamed() const
{
	return m_streams.numElem() > 0 ? m_streams[0].sample->IsStreaming() : false;
}

bool CEqAudioSourceAL::InitSource()
{
	ALuint source;

	// initialize source
	alGenSources(1, &source);
	if (!ALCheckError("gen source"))
	{
		m_source = source;
		return false;
	}

	alSourcei(source, AL_LOOPING, AL_FALSE);
	alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
	alSourcei(source, AL_AUXILIARY_SEND_FILTER_GAIN_AUTO, AL_TRUE);
	alSourcef(source, AL_MIN_GAIN, 0.0f);
	alSourcef(source, AL_MAX_GAIN, 2.0f);
	alSourcef(source, AL_DOPPLER_FACTOR, 1.0f);
	alSourcef(source, AL_MAX_DISTANCE, F_INFINITY);

	// TODO: enable AL_SOURCE_SPATIALIZE_SOFT for non-relative sources by default
	// TODO: add support mixing for different sources using AL_SOFT_callback_buffer ext

	m_source = source;

	// initialize buffers
	alGenBuffers(EQSND_STREAM_BUFFER_COUNT, m_buffers);
	ALCheckError("gen stream buffer");

	return true;
}

void CEqAudioSourceAL::Release()
{
	m_callback = nullptr;
	m_channel = -1;
	m_state = STOPPED;

	if (m_source != AL_NONE)
	{
		EmptyBuffers();

		alDeleteBuffers(EQSND_STREAM_BUFFER_COUNT, m_buffers);
		alDeleteSources(1, &m_source);

		m_source = AL_NONE;
		m_streams.clear();
	}

	if (m_filter != AL_NONE)
	{
		GetAlExt().alDeleteFilters(1, &m_filter);
		m_filter = AL_NONE;
	}
}

// updates channel (in cycle)
bool CEqAudioSourceAL::DoUpdate()
{
	if (!alIsSource(m_source))
	{
		// force destroy invalid source
		Release();
		m_releaseOnStop = true;
		return false;
	}

	// process user callback
	if (m_callback)
	{
		Params params;
		GetParams(params);

		m_callback(this, params);

		// update channel parameters
		UpdateParams(params);
	}
	else
	{
		// monitor mixer state
		const int channel = m_channel;
		if (m_owner->m_mixerChannels.inRange(channel))
		{
			const CEqAudioSystemAL::MixerChannel& mixChannel = m_owner->m_mixerChannels[channel];
			if (mixChannel.updateFlags)
			{
				Params params;
				GetParams(params);
				UpdateParams(params);
			}
		}
	}

	if (!m_streams.numElem())
	{
		return (m_releaseOnStop == false);
	}

	const bool isStreaming = IsStreamed();

	// get source state again
	int sourceState = AL_STOPPED;
	alGetSourcei(m_source, AL_SOURCE_STATE, &sourceState);

	if (isStreaming)
	{
		// always disable internal looping for streams
		alSourcei(m_source, AL_LOOPING, AL_FALSE);

		// update buffers
		if (m_state == PLAYING)
		{
			int	processedBuffers;
			alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processedBuffers);

			while (processedBuffers--)
			{
				// dequeue and get buffer
				ALuint buffer;
				alSourceUnqueueBuffers(m_source, 1, &buffer);

				if (!QueueStreamChannel(buffer))
				{
					m_state = STOPPED;
					break;
				}
			}

			if (sourceState != AL_PLAYING)
				alSourcePlay(m_source);
		}
	}
	else
	{
		if (sourceState == AL_INITIAL || sourceState == AL_STOPPED)
			m_state = STOPPED;
		else if (sourceState == AL_PLAYING)
			m_state = PLAYING;
		else if (sourceState == AL_PAUSED)
			m_state = PAUSED;
	}

	// release channel if stopped
	if (m_releaseOnStop && m_state == STOPPED)
	{
		// drop voice
		Release();
	}

	return true;
}

static ALsizei AL_APIENTRY SoundSourceSampleDataCallback(void* userPtr, void* data, ALsizei size)
{
	CEqAudioSourceAL* audioSrc = reinterpret_cast<CEqAudioSourceAL*>(userPtr);
	return audioSrc->GetSampleBuffer(data, size);
}

ALsizei CEqAudioSourceAL::GetSampleBuffer(void* data, ALsizei size)
{
	const bool looping = m_looping;

	// silence before mix
	memset(data, 0, size);

	int numChannels;
	alGetBufferi(m_buffers[0], AL_CHANNELS, &numChannels);
	
	// We are mixing always into 16 bit no matter what
	const int sizeOfChannels = sizeof(short) * numChannels;
	const int requestedSamples = size / sizeOfChannels;
	int totalMixed = 0;

	constexpr const int MIN_SAMPLES = 2;
	constexpr const int EXTRA_SAMPLES_CORRECTION = 1;

	// we can mix up to 8 samples simultaneously
	for(SourceStream& stream : m_streams)
	{
		const ISoundSource* sample = stream.sample;
		const float sampleVolume = min(stream.volume, 1.0f);
		const float samplePitch = min(stream.pitch, 8.0f);

		const int numSamplesToRead = max(MIN_SAMPLES, requestedSamples * samplePitch + (samplePitch < 1.0f ? EXTRA_SAMPLES_CORRECTION : 0));

		if (sampleVolume <= 0.0f || !stream.mixFunc)
		{
			// update playback progress still but don't mix
			stream.curPos = WrapAroundSampleOffset(stream.curPos + numSamplesToRead, sample, looping);
			totalMixed = max(totalMixed, requestedSamples);
			continue;
		}

		const ISoundSource::Format& fmt = sample->GetFormat();
		const int sampleUnit = (fmt.bitwidth >> 3);

		void* streamSamples = stackalloc(numSamplesToRead * sampleUnit * fmt.channels);
		const int samplesRead = sample->GetSamples(streamSamples, numSamplesToRead, stream.curPos, looping);

		const int mixedSamples = stream.mixFunc(streamSamples, samplesRead, data, requestedSamples, sampleVolume, samplePitch);
		
		stream.curPos = WrapAroundSampleOffset(stream.curPos + samplesRead, sample, looping);
		totalMixed = max(totalMixed, mixedSamples);
	}

	return totalMixed * sizeOfChannels;
}

void CEqAudioSourceAL::SetupStreamMixer(SourceStream& stream) const
{
	stream.mixFunc = nullptr;

	const ISoundSource::Format& fmt = stream.sample->GetFormat();
	const int sampleUnit = (fmt.bitwidth >> 3);
	if (sampleUnit == sizeof(uint8))
	{
		if (fmt.channels == 1)
			stream.mixFunc = Mixer::MixMono8;
		else if (fmt.channels == 2)
			stream.mixFunc = Mixer::MixStereo8;
	}
	else if (sampleUnit == sizeof(uint16))
	{
		if (fmt.channels == 1)
			stream.mixFunc = Mixer::MixMono16;
		else if (fmt.channels == 2)
			stream.mixFunc = Mixer::MixStereo16;
	}

	ASSERT_MSG(stream.mixFunc, "Unsupported audio sample '%s' format (bits=%d, channels=%d)", stream.sample->GetFilename(), fmt.bitwidth, fmt.channels);
}

void CEqAudioSourceAL::SetupSample(const ISoundSource* sample)
{
	ASSERT_MSG(sample, "SetupSample - No samples");

	// setup voice defaults
	SourceStream& stream = m_streams.append();
	stream.sample = const_cast<ISoundSource*>(sample);

	if (sample->IsStreaming())
		return;

	const CSoundSource_OpenALCache* alSource = static_cast<const CSoundSource_OpenALCache*>(sample);
	alSourcei(m_source, AL_BUFFER, alSource->m_alBuffer);
}

void CEqAudioSourceAL::SetupSamples(ArrayCRef<const ISoundSource*> samples)
{
#if !USE_ALSOFT_BUFFER_CALLBACK
	ASSERT_MSG(samples.numElem() == 1, "SetupSample - USE_ALSOFT_BUFFER_CALLBACK required to setup more than one samples");
#endif

	ASSERT_MSG(samples.numElem() > 0, "SetupSample - No samples");
	ASSERT_MSG(samples.numElem() < EQSND_SAMPLE_COUNT, "SetupSamples - exceeding EQSND_SAMPLE_COUNT (%d), required %d", EQSND_SAMPLE_COUNT, samples.numElem());

	// create streams
	for (const ISoundSource* sample : samples)
	{
		SourceStream& stream = m_streams.append();
		stream.sample = const_cast<ISoundSource*>(sample);
	}

	if (samples.front()->IsStreaming())
		return;

#if USE_ALSOFT_BUFFER_CALLBACK // all this possible because of this
	if (GetAlExt().alBufferCallbackSOFT)
	{
		int channels = 1;
		for (SourceStream& stream : m_streams)
		{
			ASSERT(stream.sample);
			if (stream.sample->IsStreaming())
			{
				ASSERT_FAIL("Got streamed sample %s which not yet supported with multi-sample feature", stream.sample->GetFilename());
				continue;
			}
			SetupStreamMixer(stream);
			channels = max(stream.sample->GetFormat().channels, channels);
		}

		// For multi-sample sound we need to specify the best format to work with if they are different
		// So we're mixing always into 16 bit no matter what
		ALenum alFormat;
		if (channels == 1)
			alFormat = AL_FORMAT_MONO16;
		else if (channels == 2)
			alFormat = AL_FORMAT_STEREO16;

		// TODO: choose device frequency?
		const ISoundSource::Format fmt = m_streams.front().sample->GetFormat();

		// set the callback on AL buffer
		// alBufferData will reset this to NULL for us
		GetAlExt().alBufferCallbackSOFT(m_buffers[0], alFormat, fmt.frequency, SoundSourceSampleDataCallback, this);
		alSourcei(m_source, AL_BUFFER, m_buffers[0]);
		return;
	}
#endif // USE_ALSOFT_BUFFER_CALLBACK

	const CSoundSource_OpenALCache* alSource = static_cast<const CSoundSource_OpenALCache*>(samples.front());
	alSourcei(m_source, AL_BUFFER, alSource->m_alBuffer);
}

bool CEqAudioSourceAL::QueueStreamChannel(ALuint buffer)
{
	static ubyte pcmBuffer[EQSND_STREAM_BUFFER_SIZE];

	if (m_streams.numElem() == 0)
		return false;

	SourceStream& mainStream = m_streams.front();
	ISoundSource* sample = mainStream.sample;
	const int streamPos = mainStream.curPos;

	const ISoundSource::Format& fmt = sample->GetFormat();
	ALenum alFormat = GetSoundSourceFormatAsALEnum(fmt);
	const int sampleSize = (fmt.bitwidth >> 3) * fmt.channels;

	// read sample data and update AL buffers
	const int numRead = sample->GetSamples(pcmBuffer, EQSND_STREAM_BUFFER_SIZE / sampleSize, streamPos, m_looping);

	if (numRead > 0)
	{
		mainStream.curPos = WrapAroundSampleOffset(streamPos + numRead, sample, m_looping);

		// upload to specific buffer
		alBufferData(buffer, alFormat, pcmBuffer, numRead * sampleSize, fmt.frequency);

		// queue after uploading
		alSourceQueueBuffers(m_source, 1, &buffer);
	}

	return numRead > 0;
}

// dequeues buffers
void CEqAudioSourceAL::EmptyBuffers()
{
	// stop source
	alSourceStop(m_source);

	int sourceType;
	alGetSourcei(m_source, AL_SOURCE_TYPE, &sourceType);
	if (sourceType == AL_STREAMING)
	{
		// dequeue buffers
		int numQueued = 0;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &numQueued);

		ALuint qbuffer;
		while (numQueued--)
			alSourceUnqueueBuffers(m_source, 1, &qbuffer);

		// make silent buffer (this also removes callback)
		for (int i = 0; i < EQSND_STREAM_BUFFER_COUNT; i++)
			alBufferData(m_buffers[i], AL_FORMAT_MONO16, _silence, elementsOf(_silence), 8000);
	}
	else
	{
		alSourcei(m_source, AL_BUFFER, 0);
	}
}