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
	for (int i = 0; i < m_voices.numElem(); i++)
	{
		AudioVoice_t& v = m_voices[i];

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
	m_voices.setNum(snd_voices.GetInt());
	memset(m_voices.ptr(), 0, sizeof(AudioVoice_t));

	for (int i = 0; i < m_voices.numElem(); i++)
	{
		AudioVoice_t& v = m_voices[i];
		v.m_id = VOICE_INVALID_HANDLE;
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
	m_voices.clear();

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
	SuspendVoicesWithSample(sampleSource);

	// free
	ISoundSource::DestroySound(sampleSource);
	m_samples.fastRemove(sampleSource);
}

//-----------------------------------------------

VoiceHandle_t CEqAudioSystemAL::GetFreeVoice(ISoundSource* sample, void* callbackObject, VoiceUpdateCallback fnCallback)
{
	if(sample == nullptr)
		return VOICE_INVALID_HANDLE;

	for (int i = 0; i < m_voices.numElem(); i++)
	{
		AudioVoice_t& v = m_voices[i];

		// both stupid and clever...
		if (v.m_id == -1)
		{
			v.m_callbackObject = callbackObject;
			v.m_callback = fnCallback;

			SetupVoice(v, sample);

			v.m_id = i;
			return i;
		}
	}

	// no free voices left
	return VOICE_INVALID_HANDLE;
}

void CEqAudioSystemAL::ReleaseVoice(VoiceHandle_t voice)
{
	if (voice == VOICE_INVALID_HANDLE)
		return;

	AudioVoice_t& v = m_voices[voice];

	// both stupid and clever...
	if (v.m_id == voice)
	{
		EmptyBuffers(v);

		v.m_id = VOICE_INVALID_HANDLE;
		v.m_callback = nullptr;
		v.m_callbackObject = nullptr;
		v.m_state = VOICE_STATE_STOPPED;
	}

}

//-----------------------------------------------

void CEqAudioSystemAL::SuspendVoicesWithSample(ISoundSource* sample)
{
	for (int i = 0; i < m_voices.numElem(); i++)
	{
		AudioVoice_t& v = m_voices[i];

		if (v.m_sample == sample)
		{
			EmptyBuffers(v);
		}
	}
}

//-----------------------------------------------

void CEqAudioSystemAL::SetupVoice(AudioVoice_t& voice, ISoundSource* sample)
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

bool CEqAudioSystemAL::QueueStreamVoice(AudioVoice_t& voice, ALuint buffer)
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
			voice.m_streamPos = voice.m_streamPos % sample->GetSampleCount();
		}

		// upload to specific buffer
		alBufferData(buffer, alFormat, pcmBuffer, numRead * sampleSize, formatInfo->frequency);

		// queue after uploading
		alSourceQueueBuffers(voice.m_source, 1, &buffer);
	}

	return numRead > 0;
}

// dequeues buffers
void CEqAudioSystemAL::EmptyBuffers(AudioVoice_t& voice)
{
	int numQueued;
	int sourceType;
	ALuint qbuffer;

	// stop source
	alSourceStop(voice.m_source);

	alGetSourcei(voice.m_source, AL_SOURCE_TYPE, &sourceType);

	if (sourceType == AL_STREAMING)
	{
		numQueued = 0;
		alGetSourcei(voice.m_source, AL_BUFFERS_QUEUED, &numQueued);

		while (numQueued--)
			alSourceUnqueueBuffers(voice.m_source, 1, &qbuffer);

		// make silent buffer
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
			alBufferData(voice.m_buffers[i], AL_FORMAT_MONO16, (short*)_silence, BUFFER_SILENCE_SIZE, 8000);
	}
	else
	{
		alSourcei(voice.m_source, AL_BUFFER, 0);
	}

	voice.m_sample = nullptr;
	voice.m_streamPos = 0;
}

// updates all voices
void CEqAudioSystemAL::Update()
{
	for (int i = 0; i < m_voices.numElem(); i++)
	{
		DoVoiceUpdate(m_voices[i]);
	}
}

// Updates voice with user parameters
void CEqAudioSystemAL::UpdateVoice(VoiceHandle_t handle, voiceParams_t params, int mask)
{
	if (handle == VOICE_INVALID_HANDLE)
		return;

	if (mask == 0)
		return;

	ALuint qbuffer;
	int tempValue;
	int numQueued;
	bool isStreaming;

	AudioVoice_t& voice = m_voices[handle];

	isStreaming = voice.m_sample->IsStreaming();

	if (mask & VOICE_UPDATE_POSITION)
		alSourcefv(voice.m_source, AL_POSITION, params.position);

	if (mask & VOICE_UPDATE_VELOCITY)
		alSourcefv(voice.m_source, AL_VELOCITY, params.velocity);

	if (mask & VOICE_UPDATE_VOLUME)
		alSourcef(voice.m_source, AL_GAIN, params.volume);

	if (mask & VOICE_UPDATE_PITCH)
		alSourcef(voice.m_source, AL_PITCH, params.pitch);

	if (mask & VOICE_UPDATE_REF_DIST)
		alSourcef(voice.m_source, AL_REFERENCE_DISTANCE, params.referenceDistance);

	if (mask & VOICE_UPDATE_AIRABSORPTION)
		alSourcef(voice.m_source, AL_AIR_ABSORPTION_FACTOR, params.airAbsorption);

	if (mask & VOICE_UPDATE_RELATIVE)
	{
		tempValue = params.relative == true ? AL_TRUE : AL_FALSE;
		alSourcei(voice.m_source, AL_SOURCE_RELATIVE, tempValue);
	}

	if (mask & VOICE_UPDATE_LOOPING)
	{
		voice.m_looping = params.looping;

		if(!isStreaming)
			alSourcei(voice.m_source, AL_LOOPING, voice.m_looping ? AL_TRUE : AL_FALSE);
	}

	if (mask & VOICE_UPDATE_DO_REWIND)
	{
		voice.m_streamPos = 0;
		alSourceRewind(mask);
	}

	if (mask & VOICE_UPDATE_RELEASE_ON_STOP)
		voice.m_releaseOnStop = params.releaseOnStop;

	// change state
	if (mask & VOICE_UPDATE_STATE)
	{
		if (params.state == VOICE_STATE_STOPPED)
		{
			alSourceStop(voice.m_source);
		}
		else if (params.state == VOICE_STATE_PAUSED)
		{
			alSourcePause(voice.m_source);
		}
		else if (params.state == VOICE_STATE_PLAYING)
		{
			// re-queue stream buffers
			if (isStreaming)
			{
				alSourceStop(voice.m_source);

				// first dequeue buffers
				numQueued = 0;
				alGetSourcei(voice.m_source, AL_BUFFERS_QUEUED, &numQueued);

				while (numQueued--)
					alSourceUnqueueBuffers(voice.m_source, 1, &qbuffer);

				for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
				{
					if (!QueueStreamVoice(voice, voice.m_buffers[i]))
						break; // too short
				}
			}

			alSourcePlay(voice.m_source);
		}

		voice.m_state = params.state;
	}
}

void CEqAudioSystemAL::GetVoiceParams(VoiceHandle_t handle, voiceParams_t& params)
{
	if (handle == VOICE_INVALID_HANDLE)
		return;

	int sourceState;
	int tempValue;

	AudioVoice_t& voice = m_voices[handle];

	if (voice.m_id == VOICE_INVALID_HANDLE)
		return;

	params.id = voice.m_id;

	bool isStreaming = voice.m_sample->IsStreaming();

	// get current state of alSource
	alGetSourcefv(voice.m_source, AL_POSITION, params.position);
	alGetSourcefv(voice.m_source, AL_VELOCITY, params.velocity);
	alGetSourcef(voice.m_source, AL_GAIN, &params.volume);
	alGetSourcef(voice.m_source, AL_PITCH, &params.pitch);
	alGetSourcef(voice.m_source, AL_REFERENCE_DISTANCE, &params.referenceDistance);
	alGetSourcef(voice.m_source, AL_ROLLOFF_FACTOR, &params.rolloff);
	alGetSourcef(voice.m_source, AL_AIR_ABSORPTION_FACTOR, &params.airAbsorption);

	params.looping = voice.m_looping;

	alGetSourcei(voice.m_source, AL_SOURCE_RELATIVE, &tempValue);
	params.relative = (tempValue == AL_TRUE);

	if (isStreaming)
	{
		// use voice state
		params.state = voice.m_state;
	}
	else
	{
		alGetSourcei(voice.m_source, AL_SOURCE_STATE, &sourceState);

		// use AL state
		if (sourceState == AL_INITIAL || sourceState == AL_STOPPED)
			params.state = VOICE_STATE_STOPPED;
		else if (sourceState == AL_PLAYING)
			params.state = VOICE_STATE_PLAYING;
		else if (sourceState == AL_PAUSED)
			params.state = VOICE_STATE_PAUSED;
	}

	params.releaseOnStop = voice.m_releaseOnStop;
}

// updates voice (in cycle)
void CEqAudioSystemAL::DoVoiceUpdate(AudioVoice_t& voice)
{
	if (voice.m_sample == nullptr)
		return;

	int tempValue;
	int sourceState;

	alGetSourcei(voice.m_source, AL_SOURCE_STATE, &sourceState);

	bool isStreaming = voice.m_sample->IsStreaming();

	// process user callback
	if (voice.m_callback)
	{
		voiceParams_t params;
		GetVoiceParams(voice.m_id, params);

		int mask = voice.m_callback(voice.m_callbackObject, params);

		// update voice parameters
		UpdateVoice(voice.m_id, params, mask);
	}

	// get source state again
	alGetSourcei(voice.m_source, AL_SOURCE_STATE, &sourceState);

	if (isStreaming)
	{
		// always disable internal looping for streams
		alSourcei(voice.m_source, AL_LOOPING, AL_FALSE);

		// update buffers
		if (voice.m_state == VOICE_STATE_PLAYING)
		{
			int	processedBuffers;
			alGetSourcei(voice.m_source, AL_BUFFERS_PROCESSED, &processedBuffers);

			while (processedBuffers--)
			{
				// dequeue and get buffer
				ALuint buffer;
				alSourceUnqueueBuffers(voice.m_source, 1, &buffer);

				if (!QueueStreamVoice(voice, buffer))
				{
					voice.m_state = VOICE_STATE_STOPPED;
					break;
				}
			}

			if (sourceState != AL_PLAYING)
				alSourcePlay(voice.m_source);
		}
	}
	else
	{
		if (sourceState == AL_INITIAL || sourceState == AL_STOPPED)
			voice.m_state = VOICE_STATE_STOPPED;
		else if (sourceState == AL_PLAYING)
			voice.m_state = VOICE_STATE_PLAYING;
		else if (sourceState == AL_PAUSED)
			voice.m_state = VOICE_STATE_PAUSED;
	}

	// release voice if stopped
	if (voice.m_releaseOnStop && voice.m_state == VOICE_STATE_STOPPED)
	{
		// drop voice
		ReleaseVoice(voice.m_id);
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