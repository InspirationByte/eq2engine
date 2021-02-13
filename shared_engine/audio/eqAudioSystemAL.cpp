//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

#include "eqAudioSystemAL.h"

#include "core/ConVar.h"
#include "core/DebugInterface.h"

#include "source/snd_al_source.h"

static CEqAudioSystemAL s_audioSystemAL;
IEqAudioSystem* g_audioSystem = &s_audioSystemAL;

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
		devices += strlen(devices) + 1;
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

	// set other properties
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

// Destroys context and vocies
void CEqAudioSystemAL::Shutdown()
{
	StopAllSounds();

	m_noSound = true;

	// clear voices
	m_sources.clear();

	// delete sample sources
	while (m_samples.numElem() > 0)
		FreeSample(m_samples[0]);

	// destroy context
	DestroyContext();
}

IEqAudioSource* CEqAudioSystemAL::CreateSource()
{
	int index = m_sources.append(new CEqAudioSourceAL());
	return m_sources[index];
}

void CEqAudioSystemAL::DestroySource(IEqAudioSource* source)
{
	if (!source)
		return;

	source->Release();

	if (m_sources.fastRemove(source))
	{
		delete source;
	}
}

void CEqAudioSystemAL::StopAllSounds(int chanType /*= -1*/, void* callbackObject /*= nullptr*/)
{
	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = (CEqAudioSourceAL*)m_sources[i];
		if (chanType == -1 || source->m_chanType == chanType && source->m_callbackObject == callbackObject)
		{
			m_sources[i]->Release();
			delete m_sources[i];
		}
	}
}

void CEqAudioSystemAL::PauseAllSounds(int chanType /*= -1*/, void* callbackObject /*= nullptr*/)
{
	IEqAudioSource::params_t param;
	param.state = IEqAudioSource::PAUSED;

	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = (CEqAudioSourceAL*)m_sources[i];
		if (chanType == -1 || source->m_chanType == chanType && source->m_callbackObject == callbackObject)
			source->UpdateParams(param, IEqAudioSource::UPDATE_STATE);
	}
}

void CEqAudioSystemAL::ResumeAllSounds(int chanType /*= -1*/, void* callbackObject /*= nullptr*/)
{
	IEqAudioSource::params_t param;
	param.state = IEqAudioSource::PLAYING;

	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = (CEqAudioSourceAL*)m_sources[i];
		if (chanType == -1 || source->m_chanType == chanType && source->m_callbackObject == callbackObject)
			source->UpdateParams(param, IEqAudioSource::UPDATE_STATE);
	}
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
			// Set memory to OpenAL and destroy original source (as it's not needed anymore)
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
	SuspendSourcesWithSample(sampleSource);

	// free
	ISoundSource::DestroySound(sampleSource);
	m_samples.fastRemove(sampleSource);
}

//-----------------------------------------------

void CEqAudioSystemAL::SuspendSourcesWithSample(ISoundSource* sample)
{
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* src = (CEqAudioSourceAL*)m_sources[i];

		if (src->m_sample == sample)
		{
			src->Release();
		}
	}
}

// updates all channels
void CEqAudioSystemAL::Update()
{
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* src = (CEqAudioSourceAL*)m_sources[i];
		if (!src->DoUpdate())
		{
			if (src->m_releaseOnStop)
			{
				delete src;
				m_sources.fastRemoveIndex(i);
				i--;
			}
		}
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

//----------------------------------------------------------------------------------------------
// Sound source
//----------------------------------------------------------------------------------------------

CEqAudioSourceAL::CEqAudioSourceAL() :
	m_sample(nullptr),
	m_callback(nullptr),
	m_callbackObject(nullptr),
	m_source(AL_NONE),
	m_streamPos(0),
	m_state(STOPPED),
	m_chanType(-1),
	m_releaseOnStop(true),
	m_looping(false)
{
	memset(m_buffers, 0, sizeof(m_buffers));
}

CEqAudioSourceAL::CEqAudioSourceAL(int typeId, ISoundSource* sample, UpdateCallback fnCallback, void* callbackObject)
{
	Setup(typeId, sample, fnCallback, callbackObject);
}

// Updates channel with user parameters
void CEqAudioSourceAL::UpdateParams(params_t params, int mask)
{
	if (mask == 0)
		return;

	const ALuint thisSource = m_source;

	// is that source needs setup again?
	if (thisSource == 0)
		return;

	ALuint qbuffer;
	int tempValue;
	int numQueued;
	bool isStreaming;

	isStreaming = m_sample->IsStreaming();

	if (mask & UPDATE_POSITION)
		alSourcefv(thisSource, AL_POSITION, params.position);

	if (mask & UPDATE_VELOCITY)
		alSourcefv(thisSource, AL_VELOCITY, params.velocity);

	if (mask & UPDATE_VOLUME)
		alSourcef(thisSource, AL_GAIN, params.volume);

	if (mask & UPDATE_PITCH)
		alSourcef(thisSource, AL_PITCH, params.pitch);

	if (mask & UPDATE_REF_DIST)
		alSourcef(thisSource, AL_REFERENCE_DISTANCE, params.referenceDistance);

	if (mask & UPDATE_AIRABSORPTION)
		alSourcef(thisSource, AL_AIR_ABSORPTION_FACTOR, params.airAbsorption);

	if (mask & UPDATE_RELATIVE)
	{
		tempValue = params.relative == true ? AL_TRUE : AL_FALSE;
		alSourcei(thisSource, AL_SOURCE_RELATIVE, tempValue);
	}

	if (mask & UPDATE_LOOPING)
	{
		m_looping = params.looping;

		if (!isStreaming)
			alSourcei(thisSource, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);
	}

	if (mask & UPDATE_DO_REWIND)
	{
		m_streamPos = 0;
		alSourceRewind(mask);
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

				for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
				{
					if (!QueueStreamChannel(m_buffers[i]))
						break; // too short
				}
			}

			alSourcePlay(thisSource);
		}

		m_state = params.state;
	}
}

void CEqAudioSourceAL::GetParams(params_t& params)
{
	const ALuint thisSource = m_source;

	int sourceState;
	int tempValue;

	if (thisSource == AL_NONE)
		return;

	params.id = m_chanType;

	bool isStreaming = m_sample->IsStreaming();

	// get current state of alSource
	alGetSourcefv(thisSource, AL_POSITION, params.position);
	alGetSourcefv(thisSource, AL_VELOCITY, params.velocity);
	alGetSourcef(thisSource, AL_GAIN, &params.volume);
	alGetSourcef(thisSource, AL_PITCH, &params.pitch);
	alGetSourcef(thisSource, AL_REFERENCE_DISTANCE, &params.referenceDistance);
	alGetSourcef(thisSource, AL_ROLLOFF_FACTOR, &params.rolloff);
	alGetSourcef(thisSource, AL_AIR_ABSORPTION_FACTOR, &params.airAbsorption);

	params.looping = m_looping;

	alGetSourcei(thisSource, AL_SOURCE_RELATIVE, &tempValue);
	params.relative = (tempValue == AL_TRUE);

	if (isStreaming)
	{
		// use channel state
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

void CEqAudioSourceAL::Setup(int typeId, ISoundSource* sample, UpdateCallback fnCallback /*= nullptr*/, void* callbackObject /*= nullptr*/)
{
	InitSource();

	EmptyBuffers();

	m_callbackObject = callbackObject;
	m_callback = fnCallback;

	m_chanType = typeId;

	SetupSample(sample);
}

void CEqAudioSourceAL::InitSource()
{
	ALuint source;

	// initialize source
	alGenSources(1, &source);

	alSourcei(source, AL_LOOPING, AL_FALSE);
	alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
	alSourcei(source, AL_AUXILIARY_SEND_FILTER_GAIN_AUTO, AL_TRUE);
	alSourcef(source, AL_MAX_GAIN, 0.9f);
	alSourcef(source, AL_DOPPLER_FACTOR, 1.0f);
	m_source = source;

	// initialize buffers
	alGenBuffers(STREAM_BUFFER_COUNT, m_buffers);
}

void CEqAudioSourceAL::Release()
{
	if (m_source == AL_NONE)
		return;

	EmptyBuffers();

	m_chanType = -1;
	m_callback = nullptr;
	m_callbackObject = nullptr;
	m_state = STOPPED;

	alDeleteBuffers(STREAM_BUFFER_COUNT, m_buffers);
	alDeleteSources(1, &m_source);

	m_source = AL_NONE;
}

// updates channel (in cycle)
bool CEqAudioSourceAL::DoUpdate()
{
	// process user callback
	if (m_callback)
	{
		params_t params;
		GetParams(params);

		int mask = m_callback(m_callbackObject, params);

		// update channel parameters
		UpdateParams(params, mask);
	}

	if (m_sample == nullptr)
		return m_releaseOnStop == false;

	bool isStreaming = m_sample->IsStreaming();

	// get source state again
	int sourceState;
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

void CEqAudioSourceAL::SetupSample(ISoundSource* sample)
{
	// setup voice defaults
	m_sample = sample;
	m_streamPos = 0;
	m_releaseOnStop = (m_callback == nullptr);

	if (!sample->IsStreaming())
	{
		CSoundSource_OpenALCache* alSource = (CSoundSource_OpenALCache*)sample;

		// set the AL buffer
		alSourcei(m_source, AL_BUFFER, alSource->m_alBuffer);
	}
}

bool CEqAudioSourceAL::QueueStreamChannel(ALuint buffer)
{
	static ubyte pcmBuffer[STREAM_BUFFER_SIZE];

	ISoundSource* sample = m_sample;

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
	int numRead = sample->GetSamples(pcmBuffer, STREAM_BUFFER_SIZE / sampleSize, m_streamPos, m_looping);

	if (numRead > 0)
	{
		m_streamPos += numRead;

		// FIXME: might be invalid
		if (m_looping)
		{
			m_streamPos %= sample->GetSampleCount();
		}

		// upload to specific buffer
		alBufferData(buffer, alFormat, pcmBuffer, numRead * sampleSize, formatInfo->frequency);

		// queue after uploading
		alSourceQueueBuffers(m_source, 1, &buffer);
	}

	return numRead > 0;
}

// dequeues buffers
void CEqAudioSourceAL::EmptyBuffers()
{
	int numQueued;
	int sourceType;
	ALuint qbuffer;

	// stop source
	alSourceStop(m_source);

	alGetSourcei(m_source, AL_SOURCE_TYPE, &sourceType);

	if (sourceType == AL_STREAMING)
	{
		numQueued = 0;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &numQueued);

		while (numQueued--)
			alSourceUnqueueBuffers(m_source, 1, &qbuffer);

		// make silent buffer
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
			alBufferData(m_buffers[i], AL_FORMAT_MONO16, (short*)_silence, BUFFER_SILENCE_SIZE, 8000);
	}
	else
	{
		alSourcei(m_source, AL_BUFFER, 0);
	}

	m_sample = nullptr;
	m_streamPos = 0;
}