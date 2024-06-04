//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IDkCore.h"
#include "utils/KeyValues.h"

#include "render/IDebugOverlay.h"

#include "eqSoundCommonAL.h"
#include "eqAudioSystemAL.h"
#include "eqAudioSourceAL.h"
#include "source/snd_al_source.h"

using namespace Threading;
static CEqMutex s_audioSysMutex;

static CEqAudioSystemAL s_audioSystemAL;
IEqAudioSystem* g_audioSystem = &s_audioSystemAL;

// AL context functions
static LPALCGETSTRINGISOFT alcGetStringiSOFT = nullptr;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT = nullptr;

#define AL_LOAD_PROC(T, x)		x = (T)alGetProcAddress(#x)
#define ALC_LOAD_PROC(d, T, x)  x = (T)alcGetProcAddress(d, #x)

//---------------------------------------------------------

static void snd_hrtf_changed(ConVar* pVar, char const* pszOldValue)
{
	if(!g_audioSystem)
		return;

	((CEqAudioSystemAL*)g_audioSystem)->UpdateDeviceHRTF();
}

DECLARE_CVAR(snd_device, "0", nullptr, CV_ARCHIVE);
DECLARE_CVAR_CHANGE(snd_hrtf, "0", snd_hrtf_changed, nullptr, CV_ARCHIVE);
DECLARE_CVAR(snd_debug, "0", nullptr, CV_CHEAT);

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
		ALCheckDeviceForErrors(nullptr, "alcOpenDevice");
		return false;
	}

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
			DevMsg(DEVMSG_SOUND, "    %d: %s\n", i+1, name);
		}
	}
	else
	{
		MsgInfo("EqAudio: HRTF is NOT supported.\n");
	}

	// configure context
	ContextParamsList al_context_params;
	GetContextParams(al_context_params);

	m_ctx = alcCreateContext(m_dev, al_context_params.ptr());
	if (!ALCheckDeviceForErrors(m_dev, "alcCreateContext"))
		return false;

	alcMakeContextCurrent(m_ctx);
	if (!ALCheckDeviceForErrors(m_dev, "alcMakeContextCurrent"))
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
#if USE_ALSOFT_BUFFER_CALLBACK
	if (alIsExtensionPresent("AL_SOFT_callback_buffer"))
	{
		GetAlExt().AL_LOAD_PROC(LPALBUFFERCALLBACKSOFT, alBufferCallbackSOFT);
	}
	else
		ErrorMsg("AL_SOFT_callback_buffer is not supported, OpenAL-soft needs to be updated\n");
#endif


	return true;
}

void CEqAudioSystemAL::GetContextParams(ContextParamsList& paramsList) const
{
	const int frequency[] = { ALC_FREQUENCY, 44100 };
	const int effectSlots[] = { ALC_MAX_AUXILIARY_SENDS, EQSND_EFFECT_SLOTS };
	const int hrtfOn[] = { 
		ALC_HRTF_SOFT, snd_hrtf.GetBool(),
		ALC_HRTF_ID_SOFT, snd_hrtf.GetInt()-1,
	};

	paramsList.append(frequency, elementsOf(frequency));
	paramsList.append(effectSlots, elementsOf(effectSlots));
	paramsList.append(hrtfOn, elementsOf(hrtfOn));

	// must be always last
	const int terminator[] = {0, 0};
	paramsList.append(terminator, elementsOf(terminator));
}

void CEqAudioSystemAL::UpdateDeviceHRTF()
{
	if(!m_ctx || !alcResetDeviceSOFT)
		return;

	ContextParamsList al_context_params;
	GetContextParams(al_context_params);

	alcResetDeviceSOFT(m_dev, al_context_params.ptr());
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
	GetAlExt().AL_LOAD_PROC(LPALGENFILTERS, alGenFilters);
	GetAlExt().AL_LOAD_PROC(LPALDELETEFILTERS, alDeleteFilters);
	GetAlExt().AL_LOAD_PROC(LPALISFILTER, alIsFilter);
	GetAlExt().AL_LOAD_PROC(LPALFILTERI, alFilteri);
	GetAlExt().AL_LOAD_PROC(LPALFILTERIV, alFilteriv);
	GetAlExt().AL_LOAD_PROC(LPALFILTERF, alFilterf);
	GetAlExt().AL_LOAD_PROC(LPALFILTERFV, alFilterfv);
	GetAlExt().AL_LOAD_PROC(LPALGETFILTERI, alGetFilteri);
	GetAlExt().AL_LOAD_PROC(LPALGETFILTERIV, alGetFilteriv);
	GetAlExt().AL_LOAD_PROC(LPALGETFILTERF, alGetFilterf);
	GetAlExt().AL_LOAD_PROC(LPALGETFILTERFV, alGetFilterfv);

	if (!alcIsExtensionPresent(m_dev, ALC_EXT_EFX_NAME))
	{
		MsgWarning("Sound effects are NOT supported!\n");
		return;
	}

	GetAlExt().AL_LOAD_PROC(LPALGENEFFECTS, alGenEffects);
	GetAlExt().AL_LOAD_PROC(LPALDELETEEFFECTS, alDeleteEffects);
	GetAlExt().AL_LOAD_PROC(LPALISEFFECT, alIsEffect);
	GetAlExt().AL_LOAD_PROC(LPALEFFECTI, alEffecti);
	GetAlExt().AL_LOAD_PROC(LPALEFFECTIV, alEffectiv);
	GetAlExt().AL_LOAD_PROC(LPALEFFECTF, alEffectf);
	GetAlExt().AL_LOAD_PROC(LPALEFFECTFV, alEffectfv);
	GetAlExt().AL_LOAD_PROC(LPALGETEFFECTI, alGetEffecti);
	GetAlExt().AL_LOAD_PROC(LPALGETEFFECTIV, alGetEffectiv);
	GetAlExt().AL_LOAD_PROC(LPALGETEFFECTF, alGetEffectf);
	GetAlExt().AL_LOAD_PROC(LPALGETEFFECTFV, alGetEffectfv);

	GetAlExt().AL_LOAD_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
	GetAlExt().AL_LOAD_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
	GetAlExt().AL_LOAD_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
	GetAlExt().AL_LOAD_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
	GetAlExt().AL_LOAD_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
	GetAlExt().AL_LOAD_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
	GetAlExt().AL_LOAD_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);
	GetAlExt().AL_LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti);
	GetAlExt().AL_LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv);
	GetAlExt().AL_LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf);
	GetAlExt().AL_LOAD_PROC(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv);

	int maxEffectSlots = 0;
	alcGetIntegerv(m_dev, ALC_MAX_AUXILIARY_SENDS, 1, &maxEffectSlots);
	m_effectSlots.setNum(maxEffectSlots);

	GetAlExt().alGenAuxiliaryEffectSlots(maxEffectSlots, m_effectSlots.ptr());

	//
	// Load effect presets from file
	//
	const KVSection* soundSettings = g_eqCore->GetConfig()->FindSection("Sound");

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

		EffectInfo effect;
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

bool CEqAudioSystemAL::CreateALEffect(const char* pszName, KVSection* pSection, EffectInfo& effect)
{
#define PARAM_VALUE(type, name, str_name)  AL_##type##_##name, clamp(KV_GetValueFloat(pSection->FindSection(str_name), 0, AL_##type##_DEFAULT_##name), AL_##type##_MIN_##name, AL_##type##_MAX_##name)


	if (!CString::CompareCaseIns(pszName, "reverb"))
	{
		GetAlExt().alGenEffects(1, &effect.nAlEffect);
		if (!ALCheckError("gen buffers"))
			return false;

		GetAlExt().alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, GAIN, "gain"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, GAINHF, "gain_hf"));

		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DECAY_TIME, "decay_time"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DECAY_HFRATIO, "decay_hf"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, REFLECTIONS_DELAY, "reflection_delay"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, REFLECTIONS_GAIN, "reflection_gain"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DIFFUSION, "diffusion"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, DENSITY, "density"));
		GetAlExt().alEffectf(effect.nAlEffect, PARAM_VALUE(REVERB, AIR_ABSORPTION_GAINHF, "airabsorption_gain"));

		return true;
	}
	else if (!CString::CompareCaseIns(pszName, "echo"))
	{
		GetAlExt().alGenEffects(1, &effect.nAlEffect);
		if (!ALCheckError("gen buffers"))
			return false;

		GetAlExt().alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

		return true;
	}

#undef PARAM_VALUE

	return false;
}

void CEqAudioSystemAL::DestroyEffects()
{
	for (auto it = m_effects.begin(); !it.atEnd(); ++it)
		GetAlExt().alDeleteEffects(1, &it.value().nAlEffect);

	GetAlExt().alDeleteAuxiliaryEffectSlots(m_effectSlots.numElem(), m_effectSlots.ptr());
	m_effectSlots.clear(true);
	m_effects.clear(true);
}

// Destroys context and vocies
void CEqAudioSystemAL::Shutdown()
{
	StopAllSounds();
	DestroyEffects();

	// clear voices
	m_sources.clear(true);

	// delete sample sources
	m_samples.clear(true);

	// destroy context
	DestroyContext();

	m_noSound = true;
}

CRefPtr<IEqAudioSource> CEqAudioSystemAL::CreateSource()
{
	CScopedMutex m(s_audioSysMutex);
	const int index = m_sources.append(CRefPtr_new(CEqAudioSourceAL, this));

	return static_cast<CRefPtr<IEqAudioSource>>(m_sources[index]);
}

void CEqAudioSystemAL::DestroySource(IEqAudioSource* source)
{
	if (!source)
		return;

	CEqAudioSourceAL* src = static_cast<CEqAudioSourceAL*>(source);

	src->m_releaseOnStop = true;
	src->m_forceStop = true;
}

void CEqAudioSystemAL::StopAllSounds(int chanId /*= -1*/)
{
	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = m_sources[i].Ptr();
		if (chanId == -1 || source->m_channel == chanId)
		{
			source->m_forceStop = true;
		}
	}
}

void CEqAudioSystemAL::PauseAllSounds(int chanId /*= -1*/)
{
	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::PAUSED);

	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = m_sources[i].Ptr();
		if (chanId == -1 || source->m_channel == chanId)
			source->UpdateParams(param);
	}
}

void CEqAudioSystemAL::ResumeAllSounds(int chanId /*= -1*/)
{
	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::PLAYING);

	// suspend all sources
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* source = m_sources[i].Ptr();
		if (chanId == -1 || source->m_channel == chanId)
			source->UpdateParams(param);
	}
}

void CEqAudioSystemAL::ResetMixer(int chanId)
{
	if (!m_mixerChannels.inRange(chanId))
		return;

	m_mixerChannels[chanId] = MixerChannel{};
}

void CEqAudioSystemAL::SetChannelVolume(int chanType, float value)
{
	if (!m_mixerChannels.inRange(chanType))
		return;

	MixerChannel& channel = m_mixerChannels[chanType];
	channel.volume = value;
	channel.updateFlags |= IEqAudioSource::UPDATE_VOLUME;
}

void CEqAudioSystemAL::SetChannelPitch(int chanType, float value)
{
	if (!m_mixerChannels.inRange(chanType))
		return;

	MixerChannel& channel = m_mixerChannels[chanType];
	channel.pitch = value;
	channel.updateFlags |= IEqAudioSource::UPDATE_PITCH;
}

// loads sample source data
ISoundSourcePtr CEqAudioSystemAL::GetSample(const char* filename)
{
	{
		const int nameHash = StringToHash(filename, true);
		CScopedMutex m(s_audioSysMutex);
		auto it = m_samples.find(nameHash);
		if (!it.atEnd())
			return ISoundSourcePtr(*it);
	}

	ISoundSourcePtr sampleSource = ISoundSource::CreateSound(filename);

	if (sampleSource)
	{
		const ISoundSource::Format& fmt = sampleSource->GetFormat();

		if (fmt.dataFormat != ISoundSource::FORMAT_PCM || fmt.bitwidth > 16)	// not PCM or 32 bit
		{
			MsgWarning("Sound '%s' has unsupported format!\n", filename);
			return nullptr;
		}
		else if (fmt.channels > 2)
		{
			MsgWarning("Sound '%s' has unsupported channel count (%d)!\n", filename, fmt.channels);
			return nullptr;
		}

#if USE_ALSOFT_BUFFER_CALLBACK
		if (!GetAlExt().alBufferCallbackSOFT && !sampleSource->IsStreaming())
#else
		if (!sampleSource->IsStreaming())
#endif
		{
			// Set memory to OpenAL and destroy original source (as it's not needed anymore)
			sampleSource = ISoundSourcePtr(CRefPtr_new(CSoundSource_OpenALCache, sampleSource));
		}

		AddSample(sampleSource);
	}

	return sampleSource;
}

void CEqAudioSystemAL::AddSample(ISoundSource* sample)
{
	const int nameHash = sample->GetNameHash();

	{
		CScopedMutex m(s_audioSysMutex);
		auto it = m_samples.find(nameHash);
		ASSERT_MSG(it.atEnd(), "Audio sample '%s' is already registered\n", sample->GetFilename());
	}

	CScopedMutex m(s_audioSysMutex);
	m_samples.insert(nameHash, sample);
}

void CEqAudioSystemAL::OnSampleDeleted(ISoundSource* sampleSource)
{
	if (!sampleSource)
		return;

	// stop voices using that sample
	SuspendSourcesWithSample(sampleSource);

	DevMsg(DEVMSG_SOUND, "freeing sample %s\n", sampleSource->GetFilename());

	// remove from list
	{
		CScopedMutex m(s_audioSysMutex);
		m_samples.remove(sampleSource->GetNameHash());
	}
}

// finds the effect. May return EFFECTID_INVALID
AudioEffectId CEqAudioSystemAL::FindEffect(const char* name) const
{
	const int nameHash = StringToHash(name, true);
	auto it = m_effects.find(nameHash);

	if (!it.atEnd())
		return it.value().nAlEffect;

	return EFFECT_ID_NONE;
}

// sets the new effect
void CEqAudioSystemAL::SetEffect(int slot, AudioEffectId effect)
{
	// used directly
	GetAlExt().alAuxiliaryEffectSloti(m_effectSlots[slot], AL_EFFECTSLOT_EFFECT, effect);
}

int	CEqAudioSystemAL::GetEffectSlotCount() const
{
	return m_effectSlots.numElem();
}

//-----------------------------------------------

void CEqAudioSystemAL::SuspendSourcesWithSample(ISoundSource* sample)
{
	for (int i = 0; i < m_sources.numElem(); i++)
	{
		CEqAudioSourceAL* src = m_sources[i].Ptr();

		for (int j = 0; j < src->m_streams.numElem(); ++j)
		{
			if (src->m_streams[j].sample == sample)
			{
				// sadly, entire sound source has to be stopped
				src->Release();
				break;
			}
		}
	}
}

// updates all channels
void CEqAudioSystemAL::BeginUpdate()
{
	if (m_begunUpdate)
		return;

	m_begunUpdate = true;
	alcSuspendContext(m_ctx);
}

void CEqAudioSystemAL::EndUpdate()
{
	if (!m_begunUpdate)
		return;

	m_begunUpdate = false;

	PROF_EVENT("AudioSystemAL EndUpdate");

	CScopedMutex m(s_audioSysMutex);
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
				m_sources.fastRemoveIndex(i);
				i--;
			}
		}
	}

	// setup orientation parameters
	const float orient[] = { m_listener.orientF.x,  m_listener.orientF.y,  m_listener.orientF.z, -m_listener.orientU.x, -m_listener.orientU.y, -m_listener.orientU.z };

	alListenerfv(AL_POSITION, m_listener.position);
	alListenerfv(AL_VELOCITY, m_listener.velocity);
	alListenerfv(AL_ORIENTATION, orient);

	alcProcessContext(m_ctx);

	for (int i = 0; i < m_mixerChannels.numElem(); ++i)
		m_mixerChannels[i].updateFlags = 0;

#ifdef ENABLE_DEBUG_DRAWING
	if (snd_debug.GetBool())
	{
		uint sampleMem = 0;
		for (auto it = m_samples.begin(); !it.atEnd(); ++it)
		{
			const ISoundSource* sample = *it;
			if (sample->IsStreaming())
				continue;

			const ISoundSource::Format& fmt = sample->GetFormat();
			const int sampleUnit = (fmt.bitwidth >> 3);
			const int sampleSize = sampleUnit * fmt.channels;

			sampleMem += sample->GetSampleCount() * sampleSize;
		}

		uint playing = 0;
		for (int i = 0; i < m_sources.numElem(); i++)
		{
			CEqAudioSourceAL* src = m_sources[i].Ptr();
			playing += (src->GetState() == IEqAudioSource::PLAYING);
		}

		debugoverlay->Text(color_white, "-----SOUND STATISTICS-----");
		debugoverlay->Text(color_white, "  sources: %d, (%d allocated)", playing, m_sources.numElem());
		debugoverlay->Text(color_white, "  samples: %d, mem: %d kbytes (non-streamed)", m_samples.size(), sampleMem / 1024);
	}
#endif // ENABLE_DEBUG_DRAWING
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
	m_listener.position = position;
	m_listener.velocity = velocity;
	m_listener.orientF = forwardVec;
	m_listener.orientU = upVec;
}

// gets listener properties
const Vector3D& CEqAudioSystemAL::GetListenerPosition() const
{
	return m_listener.position;
}
