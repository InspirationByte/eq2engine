//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IDkCore.h"
#include "utils/KeyValues.h"

#include "eqAudioSystemAL.h"
#include "source/snd_al_source.h"

using namespace Threading;
static CEqMutex s_audioSysMutex;

static CEqAudioSystemAL s_audioSystemAL;
IEqAudioSystem* g_audioSystem = &s_audioSystemAL;

// this allows us to mix between samples in single source
// and also eliminates few reallocations to just copy to single AL buffer
#define USE_ALSOFT_BUFFER_CALLBACK 1

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

bool checkALDeviceForErrors(ALCdevice* dev, const char* stage)
{
	ALCenum alErr = alcGetError(dev);
	if (alErr != AL_NO_ERROR)
	{
		MsgError("%s error: %s\n", stage, getALCErrorString(alErr));
		return false;
	}
	return true;
}

// AL effects prototypes
static LPALGENEFFECTS alGenEffects = nullptr;
static LPALEFFECTI alEffecti = nullptr;
static LPALEFFECTF alEffectf = nullptr;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
static LPALDELETEEFFECTS alDeleteEffects = nullptr;

// AL context prototypes
static LPALCGETSTRINGISOFT alcGetStringiSOFT = nullptr;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT = nullptr;

// AL buffer prototypes
LPALBUFFERCALLBACKSOFT alBufferCallbackSOFT;

#define AL_LOAD_PROC(x, T)		x = (T)alGetProcAddress(#x)
#define ALC_LOAD_PROC(d, T, x)  x = (T)alcGetProcAddress(d, #x)

//---------------------------------------------------------

static ConVar snd_device("snd_device", "0", nullptr, CV_ARCHIVE);
static ConVar snd_hrtf("snd_hrtf", "0", nullptr, CV_ARCHIVE);

//---------------------------------------------------------

#define BUFFER_SILENCE_SIZE		128
static const short _silence[BUFFER_SILENCE_SIZE] = { 0 };

//---------------------------------------------------------

CEqAudioSystemAL::CEqAudioSystemAL()
{
}

CEqAudioSystemAL::~CEqAudioSystemAL()
{
}

// init AL context
bool CEqAudioSystemAL::InitContext()
{
	Msg(" \n--------- AudioSystem Init --------- \n");

	// Init openAL
	Array<const char*> tempListChars(PP_SL);

	// check devices list
	const char* devices = (char*)alcGetString(nullptr, ALC_DEVICE_SPECIFIER);

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

	if (!m_dev)
	{
		checkALDeviceForErrors(nullptr, "alcOpenDevice");
		return false;
	}

	int hrtfIndex = -1;
	if (alcIsExtensionPresent(m_dev, "ALC_SOFT_HRTF"))
	{
		ALC_LOAD_PROC(m_dev, LPALCGETSTRINGISOFT, alcGetStringiSOFT);
		ALC_LOAD_PROC(m_dev, LPALCRESETDEVICESOFT, alcResetDeviceSOFT);

		DevMsg(DEVMSG_SOUND, "Enumerate HRTF modes:\n");
		ALCint numHrtf;
		alcGetIntegerv(m_dev, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &numHrtf);

		for (int i = 0; i < numHrtf; i++)
		{
			const ALCchar* name = alcGetStringiSOFT(m_dev, ALC_HRTF_SPECIFIER_SOFT, i);
			DevMsg(DEVMSG_SOUND, "    %d: %s\n", i, name);

			if (i == 0)
				hrtfIndex = i;
		}
	}
	else
	{
		MsgInfo("EqAudio: HRTF is NOT supported.\n");
	}

	// configure context
	int al_context_params[] =
	{
		ALC_FREQUENCY, 44100,
		ALC_MAX_AUXILIARY_SENDS, EQSND_EFFECT_SLOTS,
		ALC_HRTF_SOFT, snd_hrtf.GetBool(),
		ALC_HRTF_ID_SOFT, hrtfIndex,
		//ALC_SYNC, ALC_TRUE,
		//ALC_REFRESH, 120,
		0
	};

	m_ctx = alcCreateContext(m_dev, al_context_params);
	if (!checkALDeviceForErrors(m_dev, "alcCreateContext"))
		return false;

	alcMakeContextCurrent(m_ctx);
	if (!checkALDeviceForErrors(m_dev, "alcMakeContextCurrent"))
		return false;

	// check HRTF state
	{
		ALCint hrtfState;
		alcGetIntegerv(m_dev, ALC_HRTF_SOFT, 1, &hrtfState);
		if (hrtfState)
		{
			const ALchar* name = alcGetString(m_dev, ALC_HRTF_SPECIFIER_SOFT);
			MsgInfo("EqAudio: HRTF enabled, using %s\n", name);
		}
	}

	// buffer callback is required for multi-source mixing
	if (alIsExtensionPresent("AL_SOFT_callback_buffer"))
	{
		AL_LOAD_PROC(alBufferCallbackSOFT, LPALBUFFERCALLBACKSOFT);
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

	m_mixerChannels.setNum(EQSND_MIXER_CHANNELS);
	InitEffects();

	m_noSound = false;

	// set other properties
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
}

void CEqAudioSystemAL::InitEffects()
{
	if (!alcIsExtensionPresent(m_dev, ALC_EXT_EFX_NAME))
	{
		MsgWarning("Sound effects are NOT supported!\n");
		return;
	}

	AL_LOAD_PROC(alGenEffects, LPALGENEFFECTS);
	AL_LOAD_PROC(alEffecti, LPALEFFECTI);
	AL_LOAD_PROC(alEffectf, LPALEFFECTF);
	AL_LOAD_PROC(alGenAuxiliaryEffectSlots, LPALGENAUXILIARYEFFECTSLOTS);
	AL_LOAD_PROC(alAuxiliaryEffectSloti, LPALAUXILIARYEFFECTSLOTI);
	AL_LOAD_PROC(alDeleteAuxiliaryEffectSlots, LPALDELETEAUXILIARYEFFECTSLOTS);
	AL_LOAD_PROC(alDeleteEffects, LPALDELETEEFFECTS);

	int maxEffectSlots = 0;
	alcGetIntegerv(m_dev, ALC_MAX_AUXILIARY_SENDS, 1, &maxEffectSlots);
	m_effectSlots.setNum(maxEffectSlots);

	alGenAuxiliaryEffectSlots(maxEffectSlots, m_effectSlots.ptr());

	//
	// Load effect presets from file
	//
	KVSection* soundSettings = g_eqCore->GetConfig()->FindSection("Sound");

	const char* effectFilePath = soundSettings ? KV_GetValueString(soundSettings->FindSection("EFXScript"), 0, nullptr) : nullptr;

	if (effectFilePath == nullptr)
	{
		MsgError("InitEFX: EQCONFIG missing Sound:EFXScript !\n");
		return;
	}

	KeyValues kv;
	if (!kv.LoadFromFile(effectFilePath))
	{
		MsgError("InitEFX: Can't init EFX from '%s'\n", effectFilePath);
		return;
	}

	for (int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
	{
		KVSection* pEffectSection = kv.GetRootSection()->keys[i];
		const int nameHash = StringToHash(pEffectSection->name, true);

		sndEffect_t effect;
		strcpy(effect.name, pEffectSection->name);

		KVSection* pPair = pEffectSection->FindSection("type");

		if (pPair)
		{
			if (!CreateALEffect(KV_GetValueString(pPair), pEffectSection, effect))
			{
				MsgError("SOUND: Cannot create effect '%s' with type %s!\n", effect.name, KV_GetValueString(pPair));
				continue;
			}
		}
		else
		{
			MsgError("SOUND: Effect '%s' doesn't have type!\n", effect.name);
			continue;
		}

		DevMsg(DEVMSG_SOUND, "registering sound effect '%s'\n", effect.name);

		m_effects.insert(nameHash, effect);
	}
}

bool CEqAudioSystemAL::CreateALEffect(const char* pszName, KVSection* pSection, sndEffect_t& effect)
{
#define PARAM_VALUE(type, name, str_name)  AL_##type##_##name, clamp(KV_GetValueFloat(pSection->FindSection(str_name), 0, AL_##type##_DEFAULT_##name), AL_##type##_MIN_##name, AL_##type##_MAX_##name)


	if (!stricmp(pszName, "reverb"))
	{
		alGenEffects(1, &effect.nAlEffect);

		alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, GAIN, "gain"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, GAINHF, "gain_hf"));

		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DECAY_TIME, "decay_time"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DECAY_HFRATIO, "decay_hf"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, REFLECTIONS_DELAY, "reflection_delay"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, REFLECTIONS_GAIN, "reflection_gain"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DIFFUSION, "diffusion"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DENSITY, "density"));
		alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, AIR_ABSORPTION_GAINHF, "airabsorption_gain"));

		return true;
	}
	else if (!stricmp(pszName, "echo"))
	{
		alGenEffects(1, &effect.nAlEffect);

		alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

		return true;
	}

#undef PARAM_VALUE

	return false;
}

void CEqAudioSystemAL::DestroyEffects()
{
	for (auto it = m_effects.begin(); it != m_effects.end(); ++it)
		alDeleteEffects(1, &it.value().nAlEffect);

	alDeleteAuxiliaryEffectSlots(m_effectSlots.numElem(), m_effectSlots.ptr());
	m_effectSlots.clear();
	m_effects.clear();
}

// Destroys context and vocies
void CEqAudioSystemAL::Shutdown()
{
	StopAllSounds();
	DestroyEffects();

	// clear voices
	m_sources.clear();

	// delete sample sources
	for (auto it = m_samples.begin(); it != m_samples.end(); ++it)
	{
		ISoundSource::DestroySound(*it);
		m_samples.remove(it);
	}

	// destroy context
	DestroyContext();

	m_noSound = true;
}

IEqAudioSource* CEqAudioSystemAL::CreateSource()
{
	CScopedMutex m(s_audioSysMutex);

	const int index = m_sources.append(CRefPtr_new(CEqAudioSourceAL, this));
	return m_sources[index].Ptr();
}

void CEqAudioSystemAL::DestroySource(IEqAudioSource* source)
{
	if (!source)
		return;

	CEqAudioSourceAL* src = (CEqAudioSourceAL*)source;

	src->m_releaseOnStop = true;
	src->m_forceStop = true;
}

void CEqAudioSystemAL::StopAllSounds(int chanId /*= -1*/, void* callbackObject /*= nullptr*/)
{
	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = m_sources[i].Ptr();
		if (chanId == -1 || source->m_channel == chanId && source->m_callbackObject == callbackObject)
		{
			source->m_forceStop = true;
		}
	}
}

void CEqAudioSystemAL::PauseAllSounds(int chanId /*= -1*/, void* callbackObject /*= nullptr*/)
{
	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::PAUSED);

	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = m_sources[i].Ptr();
		if (chanId == -1 || source->m_channel == chanId && source->m_callbackObject == callbackObject)
			source->UpdateParams(param);
	}
}

void CEqAudioSystemAL::ResumeAllSounds(int chanId /*= -1*/, void* callbackObject /*= nullptr*/)
{
	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::PLAYING);

	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = m_sources[i].Ptr();
		if (chanId == -1 || source->m_channel == chanId && source->m_callbackObject == callbackObject)
			source->UpdateParams(param);
	}
}

void CEqAudioSystemAL::SetChannelVolume(int chanType, float value)
{
	if (!m_mixerChannels.inRange(chanType))
		return;

	MixerChannel_t& channel = m_mixerChannels[chanType];
	channel.volume = value;
	channel.updateFlags |= IEqAudioSource::UPDATE_VOLUME;
}

void CEqAudioSystemAL::SetChannelPitch(int chanType, float value)
{
	if (!m_mixerChannels.inRange(chanType))
		return;

	MixerChannel_t& channel = m_mixerChannels[chanType];
	channel.pitch = value;
	channel.updateFlags |= IEqAudioSource::UPDATE_PITCH;
}

// loads sample source data
ISoundSource* CEqAudioSystemAL::LoadSample(const char* filename)
{
	const int nameHash = StringToHash(filename, true);
	ISoundSource* sampleSource = ISoundSource::CreateSound(filename);

	if (sampleSource)
	{
		const ISoundSource::Format& fmt = sampleSource->GetFormat();

		if (fmt.dataFormat != ISoundSource::FORMAT_PCM || fmt.bitwidth > 16)	// not PCM or 32 bit
		{
			MsgWarning("Sound '%s' has unsupported format!\n", filename);
			ISoundSource::DestroySound(sampleSource);
			return nullptr;
		}
		else if (fmt.channels > 2)
		{
			MsgWarning("Sound '%s' has unsupported channel count (%d)!\n", filename, fmt.channels);
			ISoundSource::DestroySound(sampleSource);
			return nullptr;
		}

#if !USE_ALSOFT_BUFFER_CALLBACK
		if (!sampleSource->IsStreaming())
		{
			// Set memory to OpenAL and destroy original source (as it's not needed anymore)
			CSoundSource_OpenALCache* alCacheSource = PPNew CSoundSource_OpenALCache(sampleSource);
			ISoundSource::DestroySound(sampleSource);

			sampleSource = alCacheSource;
		}
#endif
		{
			CScopedMutex m(s_audioSysMutex);
			m_samples.insert(nameHash, sampleSource);
		}
		
	}

	return sampleSource;
}

void CEqAudioSystemAL::FreeSample(ISoundSource* sampleSource)
{
	// stop voices using that sample
	SuspendSourcesWithSample(sampleSource);

	// free
	{
		CScopedMutex m(s_audioSysMutex);
		for (auto it = m_samples.begin(); it != m_samples.end(); ++it)
		{
			if (*it != sampleSource)
				continue;

			ISoundSource::DestroySound(*it);
			m_samples.remove(it);
			break;
		}
	}
}

// finds the effect. May return EFFECTID_INVALID
effectId_t CEqAudioSystemAL::FindEffect(const char* name) const
{
	const int nameHash = StringToHash(name, true);
	auto it = m_effects.find(nameHash);

	if (it != m_effects.end())
		return it.value().nAlEffect;

	return EFFECT_ID_NONE;
}

// sets the new effect
void CEqAudioSystemAL::SetEffect(int slot, effectId_t effect)
{
	// used directly
	alAuxiliaryEffectSloti(m_effectSlots[slot], AL_EFFECTSLOT_EFFECT, effect);
}

//-----------------------------------------------

void CEqAudioSystemAL::SuspendSourcesWithSample(ISoundSource* sample)
{
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* src = m_sources[i].Ptr();

		if (src->m_sample == sample)
		{
			src->Release();
		}
	}
}

// updates all channels
void CEqAudioSystemAL::BeginUpdate()
{
	ASSERT(m_begunUpdate == false);
	m_begunUpdate = true;
	alcSuspendContext(m_ctx);
}

void CEqAudioSystemAL::EndUpdate()
{
	ASSERT(m_begunUpdate);

	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* src = m_sources[i].Ptr();

		if (src->m_forceStop)
		{
			src->Release();
			src->m_forceStop = false;
		}

		if (!src->DoUpdate())
		{
			if (src->m_releaseOnStop)
			{
				CScopedMutex m(s_audioSysMutex);
				m_sources.fastRemoveIndex(i);
				i--;
			}
		}
	}

	alcProcessContext(m_ctx);
	m_begunUpdate = false;
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

CEqAudioSourceAL::CEqAudioSourceAL(CEqAudioSystemAL* owner) 
	: m_owner(owner)
{
}

CEqAudioSourceAL::CEqAudioSourceAL(int typeId, ISoundSource* sample, UpdateCallback fnCallback, void* callbackObject)
{
	Setup(typeId, sample, fnCallback, callbackObject);
}

CEqAudioSourceAL::~CEqAudioSourceAL()
{
	Release();
}

// Updates channel with user parameters
void CEqAudioSourceAL::UpdateParams(const Params& params, int mask)
{
	mask |= params.updateFlags;

	// apply update flags from mixer
	if (mask & UPDATE_CHANNEL)
	{
		m_channel = params.channel;
		mask &= ~UPDATE_CHANNEL;
	}

	CEqAudioSystemAL::MixerChannel_t mixChannel;

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
	bool isStreaming;

	isStreaming = m_sample ? m_sample->IsStreaming() : false;


	if (mask & UPDATE_POSITION)
		alSourcefv(thisSource, AL_POSITION, params.position);

	if (mask & UPDATE_VELOCITY)
		alSourcefv(thisSource, AL_VELOCITY, params.velocity);

	if (mask & UPDATE_VOLUME)
	{
		m_volume = params.volume;
		alSourcef(thisSource, AL_GAIN, params.volume * mixChannel.volume);
	}

	if (mask & UPDATE_PITCH)
	{
		m_pitch = params.pitch;
		alSourcef(thisSource, AL_PITCH, params.pitch * mixChannel.pitch);
	}

	if (mask & UPDATE_REF_DIST)
		alSourcef(thisSource, AL_REFERENCE_DISTANCE, params.referenceDistance);

	if (mask & UPDATE_AIRABSORPTION)
		alSourcef(thisSource, AL_AIR_ABSORPTION_FACTOR, params.airAbsorption);

	if (mask & UPDATE_EFFECTSLOT)
	{
		if (params.effectSlot < 0)
			alSource3i(thisSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
		else
			alSource3i(thisSource, AL_AUXILIARY_SEND_FILTER, m_owner->m_effectSlots[params.effectSlot], 0, AL_FILTER_NULL);
	}

	// TODO: source orientation and cone angles (AL_ORIENTATION, AL_CONE_INNER_ANGLE, AL_CONE_OUTER_ANGLE)

	// TODO: source radius with AL_SOURCE_RADIUS

	// TODO: direct filters with AL_DIRECT_FILTER
	//		filter is created per source with params like lowPass and highPass, in combination making bandPass
	//		AL_FILTER_LOWPASS
	//		AL_FILTER_HIGHPASS
	//		AL_FILTER_BANDPASS
	//		
	//		alGenFilters(1, &filter);
	//		alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
	//		alFilterf(filter, AL_LOWPASS_GAIN, 0.0f);

	if (mask & UPDATE_RELATIVE)
	{
		const int tempValue = params.relative == true ? AL_TRUE : AL_FALSE;
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

		if (!isStreaming)
			alSourceRewind(thisSource);
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
			}

			alSourcePlay(thisSource);
		}

		m_state = params.state;
	}
}

void CEqAudioSourceAL::GetParams(Params& params) const
{
	const ALuint thisSource = m_source;

	int sourceState;
	int tempValue;

	if (thisSource == AL_NONE)
		return;

	params.channel = m_channel;

	bool isStreaming = m_sample ? m_sample->IsStreaming() : false;

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

void CEqAudioSourceAL::Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback /*= nullptr*/, void* callbackObject /*= nullptr*/)
{
	Release();
	InitSource();

	m_callbackObject = callbackObject;
	m_callback = fnCallback;

	m_channel = chanId;

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

	// TODO: enable AL_SOURCE_SPATIALIZE_SOFT for non-relative sources by default
	// TODO: add support mixing for different sources using AL_SOFT_callback_buffer ext

	m_source = source;

	// initialize buffers
	alGenBuffers(EQSND_STREAM_BUFFER_COUNT, m_buffers);
}

void CEqAudioSourceAL::Release()
{
	if (m_source == AL_NONE)
		return;

	EmptyBuffers();

	m_channel = -1;
	m_callback = nullptr;
	m_callbackObject = nullptr;
	m_state = STOPPED;

	alDeleteBuffers(EQSND_STREAM_BUFFER_COUNT, m_buffers);
	alDeleteSources(1, &m_source);

	m_source = AL_NONE;
}

// updates channel (in cycle)
bool CEqAudioSourceAL::DoUpdate()
{
	// process user callback
	if (m_callback)
	{
		Params params;
		GetParams(params);

		m_callback(m_callbackObject, params);

		// update channel parameters
		UpdateParams(params);
	}
	else
	{
		// monitor mixer state
		const int channel = m_channel;
		if (m_owner->m_mixerChannels.inRange(channel))
		{
			const CEqAudioSystemAL::MixerChannel_t& mixChannel = m_owner->m_mixerChannels[channel];
			if (mixChannel.updateFlags)
			{
				Params params;
				GetParams(params);
				UpdateParams(params);
			}
		}
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

static ALenum GetSoundSourceFormatAsALEnum(const ISoundSource::Format& fmt)
{
	ALenum alFormat;

	if (fmt.bitwidth == 8)
		alFormat = fmt.channels == 2 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
	else if (fmt.bitwidth == 16)
		alFormat = fmt.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	else
		alFormat = AL_FORMAT_MONO16;

	return alFormat;
}

static ALsizei AL_APIENTRY SoundSourceSampleDataCallback(void* userPtr, void* data, ALsizei size)
{
	CEqAudioSourceAL* audioSrc = reinterpret_cast<CEqAudioSourceAL*>(userPtr);
	return audioSrc->GetSampleBuffer(data, size);
}

ALsizei CEqAudioSourceAL::GetSampleBuffer(void* data, ALsizei size)
{
	ISoundSource* sample = m_sample;

	const ISoundSource::Format& fmt = sample->GetFormat();
	const int sampleSize = fmt.bitwidth / 8 * fmt.channels;

	// This also allows us to mix multiple samples at once
	const int numRead = sample->GetSamples((ubyte*)data, size / sampleSize, m_streamPos, m_looping);
	m_streamPos += numRead;

	if (m_looping)
		m_streamPos %= sample->GetSampleCount();

	return numRead;
}

void CEqAudioSourceAL::SetupSample(const ISoundSource* sample)
{
	// setup voice defaults
	m_sample = const_cast<ISoundSource*>(sample);
	m_streamPos = 0;
	m_releaseOnStop = !m_callback;

	if (sample && !sample->IsStreaming())
	{
#if USE_ALSOFT_BUFFER_CALLBACK
		// set the callback on AL buffer
		// alBufferData will reset this to NULL for us
		const ISoundSource::Format& fmt = sample->GetFormat();
		ALenum alFormat = GetSoundSourceFormatAsALEnum(fmt);

		alBufferCallbackSOFT(m_buffers[0], alFormat, fmt.frequency, SoundSourceSampleDataCallback, this);
		alSourcei(m_source, AL_BUFFER, m_buffers[0]);
#else
		CSoundSource_OpenALCache* alSource = (CSoundSource_OpenALCache*)sample;
		alSourcei(m_source, AL_BUFFER, alSource->m_alBuffer);
#endif
	}
}

bool CEqAudioSourceAL::QueueStreamChannel(ALuint buffer)
{
	static ubyte pcmBuffer[EQSND_STREAM_BUFFER_SIZE];

	ISoundSource* sample = m_sample;

	const ISoundSource::Format& fmt = sample->GetFormat();
	ALenum alFormat = GetSoundSourceFormatAsALEnum(fmt);
	const int sampleSize = fmt.bitwidth / 8 * fmt.channels;

	// read sample data and update AL buffers
	const int numRead = sample->GetSamples(pcmBuffer, EQSND_STREAM_BUFFER_SIZE / sampleSize, m_streamPos, m_looping);

	if (numRead > 0)
	{
		m_streamPos += numRead;

		// FIXME: might be invalid
		if (m_looping)
			m_streamPos %= sample->GetSampleCount();

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
		for (int i = 0; i < EQSND_STREAM_BUFFER_COUNT; i++)
			alBufferData(m_buffers[i], AL_FORMAT_MONO16, (short*)_silence, BUFFER_SILENCE_SIZE, 8000);
	}
	else
	{
		alSourcei(m_source, AL_BUFFER, 0);
	}

	m_sample = nullptr;
	m_streamPos = 0;
}