//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////



#include "eqAudioSystemAL.h"
#include "source/snd_al_source.h"
#include "ConVar.h"
#include "DebugInterface.h"

#include <AL/alext.h>
#include <AL/efx.h>

//---------------------------------------------------------
// AL COMMON

const char* getALCErrorString(int err)
{
	switch (err)
	{
	case ALC_NO_ERROR:
		return "AL_NO_ERROR";
	case ALC_INVALID_DEVICE:
		return "ALC_INVALID_DEVICE";
	case ALC_INVALID_CONTEXT:
		return "ALC_INVALID_CONTEXT";
	case ALC_INVALID_ENUM:
		return "ALC_INVALID_ENUM";
	case ALC_INVALID_VALUE:
		return "ALC_INVALID_VALUE";
	case ALC_OUT_OF_MEMORY:
		return "ALC_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN";
	}
}

const char* getALErrorString(int err)
{
	switch (err)
	{
	case AL_NO_ERROR:
		return "AL_NO_ERROR";
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:
		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:
		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:
		return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:
		return "AL_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN";
	}
}

//---------------------------------------------------------

static ConVar snd_voices("snd_voices", "48", 24, 128, "Audio voice count", CV_ARCHIVE);
static ConVar snd_device("snd_device", "0", nullptr, CV_ARCHIVE);

//---------------------------------------------------------

#define BUFFER_SILENCE_SIZE 128
static const short _silence[BUFFER_SILENCE_SIZE] = { 0 };

//---------------------------------------------------------

CEqAudioSystemAL::CEqAudioSystemAL() : 
	m_ctx(nullptr),
	m_dev(nullptr),
	m_noSound(true)
{

}

CEqAudioSystemAL::~CEqAudioSystemAL()
{
}

// init AL context
bool CEqAudioSystemAL::InitContext()
{
	Msg(" \n--------- EqAudioSystem InitContext --------- \n");

	// Init openAL
	DkList<char*> tempListChars;

	// check devices list
	char* devices = (char*)alcGetString(nullptr, ALC_DEVICE_SPECIFIER);

	// go through device list (each device terminated with a single NULL, list terminated with double NULL)
	while ((*devices) != '\0')
	{
		tempListChars.append(devices);

		Msg("found sound device: %s\n", devices);
		devices += strlen(devices)+1;
	}

	if (snd_device.GetInt() >= tempListChars.numElem())
	{
		MsgWarning("snd_device: Invalid audio device selected, reset to 0\n");
		snd_device.SetInt(0);
	}

	Msg("Audio device: %s\n", tempListChars[snd_device.GetInt()]);
	m_dev = alcOpenDevice((ALCchar*)tempListChars[snd_device.GetInt()]);

	int alErr = AL_NO_ERROR;

	if (!m_dev)
	{
		alErr = alcGetError(nullptr);
		MsgError("alcOpenDevice: NULL DEVICE error: %s\n", getALCErrorString(alErr));
		return false;
	}

	// configure context
	int al_context_params[] =
	{
		ALC_FREQUENCY, 44100,
		ALC_MAX_AUXILIARY_SENDS, 4,
		//ALC_SYNC, ALC_TRUE,
		//ALC_REFRESH, 120,
		0
	};

	m_ctx = alcCreateContext(m_dev, al_context_params);
	alErr = alcGetError(m_dev);

	if (alErr != AL_NO_ERROR)
	{
		MsgError("alcCreateContext error: %s\n", getALCErrorString(alErr));
		return false;
	}

	alcMakeContextCurrent(m_ctx);
	alErr = alcGetError(m_dev);

	if (alErr != AL_NO_ERROR)
	{
		MsgError("alcMakeContextCurrent error: %s\n", getALCErrorString(alErr));
		return false;
	}

	return true;
}

// destroy AL context
void CEqAudioSystemAL::DestroyContext()
{
	// delete voices
	for (int i = 0; i < m_channels.numElem(); i++)
	{
		AudioChannel_t& v = m_channels[i];

		alDeleteBuffers(STREAM_BUFFER_COUNT, v.m_buffers);
		alDeleteSources(1, &v.m_source);
	}

	// destroy context
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(m_ctx);
	alcCloseDevice(m_dev);
}

// Initializes context and voices
void CEqAudioSystemAL::Init()
{
	// init OpenAL
	if (!InitContext())
		return;

	m_noSound = false;

	// init voices
	m_channels.setNum(snd_voices.GetInt());
	memset(m_channels.ptr(), 0, sizeof(AudioChannel_t));

	for (int i = 0; i < m_channels.numElem(); i++)
	{
		AudioChannel_t& v = m_channels[i];
		v.m_id = CHANNEL_INVALID_HANDLE;
		v.m_callback = nullptr;
		v.m_callbackObject = nullptr;

		// initialize source
		alGenSources(1, &v.m_source);
		alSourcei(v.m_source, AL_LOOPING, AL_FALSE);
		alSourcei(v.m_source, AL_SOURCE_RELATIVE, AL_FALSE);
		alSourcei(v.m_source, AL_AUXILIARY_SEND_FILTER_GAIN_AUTO, AL_TRUE);
		alSourcef(v.m_source, AL_MAX_GAIN, 0.9f);
		alSourcef(v.m_source, AL_DOPPLER_FACTOR, 1.0f);

		// initialize buffers
		alGenBuffers(STREAM_BUFFER_COUNT, v.m_buffers);
		
		EmptyBuffers(v);
	}

	// set other properties
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

// Destroys context and vocies
void CEqAudioSystemAL::Shutdown()
{
	m_noSound = true;

	// clear voices
	m_channels.clear();

	// delete sample sources
	while(m_samples.numElem() > 0)
		FreeSample(m_samples[0]);

	// destroy context
	DestroyContext();
}

// loads sample source data
ISoundSource* CEqAudioSystemAL::LoadSample(const char* filename)
{
	ISoundSource* sampleSource = ISoundSource::CreateSound(filename);

	if (sampleSource)
	{
		soundFormat_t* fmt = sampleSource->GetFormat();

		if (fmt->format != 1 || fmt->bitwidth > 16)	// not PCM or 32 bit
		{
			MsgWarning("Sound '%s' has unsupported format!\n", filename);
			ISoundSource::DestroySound(sampleSource);
			return nullptr;
		}
		else if (fmt->channels > 2)
		{
			MsgWarning("Sound '%s' has unsupported channel count (%d)!\n", filename, fmt->channels);
			ISoundSource::DestroySound(sampleSource);
			return nullptr;
		}

		if (!sampleSource->IsStreaming())
		{
			CSoundSource_OpenALCache* alCacheSource = new CSoundSource_OpenALCache(sampleSource);
			ISoundSource::DestroySound(sampleSource);

			sampleSource = alCacheSource;
		}

		m_samples.append(sampleSource);
	}

	return sampleSource;
}

void CEqAudioSystemAL::FreeSample(ISoundSource* sampleSource)
{
	// stop voices using that sample
	SuspendChannelsWithSample(sampleSource);

	// free
	ISoundSource::DestroySound(sampleSource);
	m_samples.fastRemove(sampleSource);
}

//-----------------------------------------------

ChannelHandle_t CEqAudioSystemAL::GetFreeChannel(ISoundSource* sample, void* callbackObject, ChannelUpdateCallback fnCallback)
{
	if(sample == nullptr)
		return CHANNEL_INVALID_HANDLE;

	for (int i = 0; i < m_channels.numElem(); i++)
	{
		AudioChannel_t& v = m_channels[i];

		// both stupid and clever...
		if (v.m_id == -1)
		{
			v.m_callbackObject = callbackObject;
			v.m_callback = fnCallback;

			SetupChannel(v, sample);

			v.m_id = i;
			return i;
		}
	}

	// no free voices left
	return CHANNEL_INVALID_HANDLE;
}

void CEqAudioSystemAL::ReleaseChannel(ChannelHandle_t voice)
{
	if (voice == CHANNEL_INVALID_HANDLE)
		return;

	AudioChannel_t& v = m_channels[voice];

	// both stupid and clever...
	if (v.m_id == voice)
	{
		EmptyBuffers(v);

		v.m_id = CHANNEL_INVALID_HANDLE;
		v.m_callback = nullptr;
		v.m_callbackObject = nullptr;
		v.m_state = CHANNEL_STATE_STOPPED;
	}

}

//-----------------------------------------------

void CEqAudioSystemAL::SuspendChannelsWithSample(ISoundSource* sample)
{
	for (int i = 0; i < m_channels.numElem(); i++)
	{
		AudioChannel_t& v = m_channels[i];

		if (v.m_sample == sample)
		{
			EmptyBuffers(v);
		}
	}
}

//-----------------------------------------------

void CEqAudioSystemAL::SetupChannel(AudioChannel_t& voice, ISoundSource* sample)
{
	// setup voice defaults
	voice.m_sample = sample;
	voice.m_streamPos = 0;
	voice.m_releaseOnStop = (voice.m_callback == nullptr);

	if (!sample->IsStreaming())
	{
		CSoundSource_OpenALCache* alSource = (CSoundSource_OpenALCache*)sample;

		// set the AL buffer
		alSourcei(voice.m_source, AL_BUFFER, alSource->m_alBuffer);
	}
}

bool CEqAudioSystemAL::QueueStreamChannel(AudioChannel_t& voice, ALuint buffer)
{
	static ubyte pcmBuffer[STREAM_BUFFER_SIZE];

	ISoundSource* sample = voice.m_sample;

	soundFormat_t* formatInfo = sample->GetFormat();
	ALenum alFormat;

	int sampleSize = formatInfo->bitwidth / 8 * formatInfo->channels;

	if (formatInfo->bitwidth == 8)
		alFormat = formatInfo->channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	else if (formatInfo->bitwidth == 16)
		alFormat = formatInfo->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	else
		alFormat = AL_FORMAT_MONO16;

	// read sample data and update AL buffers
	int numRead = sample->GetSamples(pcmBuffer, STREAM_BUFFER_SIZE / sampleSize, voice.m_streamPos, voice.m_looping);

	if (numRead > 0)
	{
		voice.m_streamPos += numRead;

		// FIXME: might be invalid
		if (voice.m_looping)
		{
			voice.m_streamPos %= sample->GetSampleCount();
		}

		// upload to specific buffer
		alBufferData(buffer, alFormat, pcmBuffer, numRead * sampleSize, formatInfo->frequency);

		// queue after uploading
		alSourceQueueBuffers(voice.m_source, 1, &buffer);
	}

	return numRead > 0;
}

// dequeues buffers
void CEqAudioSystemAL::EmptyBuffers(AudioChannel_t& channel)
{
	int numQueued;
	int sourceType;
	ALuint qbuffer;

	// stop source
	alSourceStop(channel.m_source);

	alGetSourcei(channel.m_source, AL_SOURCE_TYPE, &sourceType);

	if (sourceType == AL_STREAMING)
	{
		numQueued = 0;
		alGetSourcei(channel.m_source, AL_BUFFERS_QUEUED, &numQueued);

		while (numQueued--)
			alSourceUnqueueBuffers(channel.m_source, 1, &qbuffer);

		// make silent buffer
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
			alBufferData(channel.m_buffers[i], AL_FORMAT_MONO16, (short*)_silence, BUFFER_SILENCE_SIZE, 8000);
	}
	else
	{
		alSourcei(channel.m_source, AL_BUFFER, 0);
	}

	channel.m_sample = nullptr;
	channel.m_streamPos = 0;
}

// updates all channels
void CEqAudioSystemAL::Update()
{
	for (int i = 0; i < m_channels.numElem(); i++)
	{
		DoChannelUpdate(m_channels[i]);
	}
}

// Updates channel with user parameters
void CEqAudioSystemAL::UpdateChannel(ChannelHandle_t handle, channelParams_t params, int mask)
{
	if (handle == CHANNEL_INVALID_HANDLE)
		return;

	if (mask == 0)
		return;

	ALuint qbuffer;
	int tempValue;
	int numQueued;
	bool isStreaming;

	AudioChannel_t& chan = m_channels[handle];

	isStreaming = chan.m_sample->IsStreaming();

	if (mask & AUDIO_CHAN_UPDATE_POSITION)
		alSourcefv(chan.m_source, AL_POSITION, params.position);

	if (mask & AUDIO_CHAN_UPDATE_VELOCITY)
		alSourcefv(chan.m_source, AL_VELOCITY, params.velocity);

	if (mask & AUDIO_CHAN_UPDATE_VOLUME)
		alSourcef(chan.m_source, AL_GAIN, params.volume);

	if (mask & AUDIO_CHAN_UPDATE_PITCH)
		alSourcef(chan.m_source, AL_PITCH, params.pitch);

	if (mask & AUDIO_CHAN_UPDATE_REF_DIST)
		alSourcef(chan.m_source, AL_REFERENCE_DISTANCE, params.referenceDistance);

	if (mask & AUDIO_CHAN_UPDATE_AIRABSORPTION)
		alSourcef(chan.m_source, AL_AIR_ABSORPTION_FACTOR, params.airAbsorption);

	if (mask & AUDIO_CHAN_UPDATE_RELATIVE)
	{
		tempValue = params.relative == true ? AL_TRUE : AL_FALSE;
		alSourcei(chan.m_source, AL_SOURCE_RELATIVE, tempValue);
	}

	if (mask & AUDIO_CHAN_UPDATE_LOOPING)
	{
		chan.m_looping = params.looping;

		if(!isStreaming)
			alSourcei(chan.m_source, AL_LOOPING, chan.m_looping ? AL_TRUE : AL_FALSE);
	}

	if (mask & AUDIO_CHAN_UPDATE_DO_REWIND)
	{
		chan.m_streamPos = 0;
		alSourceRewind(mask);
	}

	if (mask & AUDIO_CHAN_UPDATE_RELEASE_ON_STOP)
		chan.m_releaseOnStop = params.releaseOnStop;

	// change state
	if (mask & AUDIO_CHAN_UPDATE_STATE)
	{
		if (params.state == CHANNEL_STATE_STOPPED)
		{
			alSourceStop(chan.m_source);
		}
		else if (params.state == CHANNEL_STATE_PAUSED)
		{
			alSourcePause(chan.m_source);
		}
		else if (params.state == CHANNEL_STATE_PLAYING)
		{
			// re-queue stream buffers
			if (isStreaming)
			{
				alSourceStop(chan.m_source);

				// first dequeue buffers
				numQueued = 0;
				alGetSourcei(chan.m_source, AL_BUFFERS_QUEUED, &numQueued);

				while (numQueued--)
					alSourceUnqueueBuffers(chan.m_source, 1, &qbuffer);

				for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
				{
					if (!QueueStreamChannel(chan, chan.m_buffers[i]))
						break; // too short
				}
			}

			alSourcePlay(chan.m_source);
		}

		chan.m_state = params.state;
	}
}

void CEqAudioSystemAL::GetChannelParams(ChannelHandle_t handle, channelParams_t& params)
{
	if (handle == CHANNEL_INVALID_HANDLE)
		return;

	int sourceState;
	int tempValue;

	AudioChannel_t& chan = m_channels[handle];

	if (chan.m_id == CHANNEL_INVALID_HANDLE)
		return;

	params.id = chan.m_id;

	bool isStreaming = chan.m_sample->IsStreaming();

	// get current state of alSource
	alGetSourcefv(chan.m_source, AL_POSITION, params.position);
	alGetSourcefv(chan.m_source, AL_VELOCITY, params.velocity);
	alGetSourcef(chan.m_source, AL_GAIN, &params.volume);
	alGetSourcef(chan.m_source, AL_PITCH, &params.pitch);
	alGetSourcef(chan.m_source, AL_REFERENCE_DISTANCE, &params.referenceDistance);
	alGetSourcef(chan.m_source, AL_ROLLOFF_FACTOR, &params.rolloff);
	alGetSourcef(chan.m_source, AL_AIR_ABSORPTION_FACTOR, &params.airAbsorption);

	params.looping = chan.m_looping;

	alGetSourcei(chan.m_source, AL_SOURCE_RELATIVE, &tempValue);
	params.relative = (tempValue == AL_TRUE);

	if (isStreaming)
	{
		// use channel state
		params.state = chan.m_state;
	}
	else
	{
		alGetSourcei(chan.m_source, AL_SOURCE_STATE, &sourceState);

		// use AL state
		if (sourceState == AL_INITIAL || sourceState == AL_STOPPED)
			params.state = CHANNEL_STATE_STOPPED;
		else if (sourceState == AL_PLAYING)
			params.state = CHANNEL_STATE_PLAYING;
		else if (sourceState == AL_PAUSED)
			params.state = CHANNEL_STATE_PAUSED;
	}

	params.releaseOnStop = chan.m_releaseOnStop;
}

// updates channel (in cycle)
void CEqAudioSystemAL::DoChannelUpdate(AudioChannel_t& chan)
{
	if (chan.m_sample == nullptr)
		return;

	int tempValue;
	int sourceState;

	alGetSourcei(chan.m_source, AL_SOURCE_STATE, &sourceState);

	bool isStreaming = chan.m_sample->IsStreaming();

	// process user callback
	if (chan.m_callback)
	{
		channelParams_t params;
		GetChannelParams(chan.m_id, params);

		int mask = chan.m_callback(chan.m_callbackObject, params);

		// update channel parameters
		UpdateChannel(chan.m_id, params, mask);
	}

	// get source state again
	alGetSourcei(chan.m_source, AL_SOURCE_STATE, &sourceState);

	if (isStreaming)
	{
		// always disable internal looping for streams
		alSourcei(chan.m_source, AL_LOOPING, AL_FALSE);

		// update buffers
		if (chan.m_state == CHANNEL_STATE_PLAYING)
		{
			int	processedBuffers;
			alGetSourcei(chan.m_source, AL_BUFFERS_PROCESSED, &processedBuffers);

			while (processedBuffers--)
			{
				// dequeue and get buffer
				ALuint buffer;
				alSourceUnqueueBuffers(chan.m_source, 1, &buffer);

				if (!QueueStreamChannel(chan, buffer))
				{
					chan.m_state = CHANNEL_STATE_STOPPED;
					break;
				}
			}

			if (sourceState != AL_PLAYING)
				alSourcePlay(chan.m_source);
		}
	}
	else
	{
		if (sourceState == AL_INITIAL || sourceState == AL_STOPPED)
			chan.m_state = CHANNEL_STATE_STOPPED;
		else if (sourceState == AL_PLAYING)
			chan.m_state = CHANNEL_STATE_PLAYING;
		else if (sourceState == AL_PAUSED)
			chan.m_state = CHANNEL_STATE_PAUSED;
	}

	// release channel if stopped
	if (chan.m_releaseOnStop && chan.m_state == CHANNEL_STATE_STOPPED)
	{
		// drop voice
		ReleaseChannel(chan.m_id);
	}
}

void CEqAudioSystemAL::SetMasterVolume(float value)
{
	alListenerf(AL_GAIN, value);
}

// sets listener properties
void CEqAudioSystemAL::SetListener(const Vector3D& position,
	const Vector3D& velocity,
	const Vector3D& forwardVec,
	const Vector3D& upVec)
{
	// setup orientation parameters
	float orient[] = { forwardVec.x, forwardVec.y, forwardVec.z, -upVec.x, -upVec.y, -upVec.z };

	alListenerfv(AL_POSITION, position);
	alListenerfv(AL_ORIENTATION, orient);
	alListenerfv(AL_VELOCITY, velocity);
}

// gets listener properties
void CEqAudioSystemAL::GetListener(Vector3D& position, Vector3D& velocity)
{
	alGetListener3f(AL_POSITION, &position.x, &position.y, &position.z);
	alGetListener3f(AL_VELOCITY, &velocity.x, &velocity.y, &velocity.z);
}