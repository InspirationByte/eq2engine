//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound system
//////////////////////////////////////////////////////////////////////////////////

#define AL_ALEXT_PROTOTYPES

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <vorbis/vorbisfile.h>

#include "alsound_local.h"

#include "ISoundSystem.h"
#include "DebugInterface.h"
#include "ConCommand.h"
#include "ConVar.h"
#include "utils/KeyValues.h"
#include "eqParallelJobs.h"

#include "IDebugOverlay.h"
#include "IDkCore.h"

#define MAX_AMBIENT_STREAMS 8

static DkSoundSystemLocal	s_soundSystem;
ISoundSystem* soundsystem = &s_soundSystem;

void channels_callback(ConVar* pVar,char const* pszOldValue)
{
	if(stricmp(pVar->GetString(),pszOldValue))
	{
		if(soundsystem->IsInitialized())
			MsgWarning("You need to restart game to take changes\n");
	}
}

static ConVar snd_mastervolume("snd_mastervolume", "1.0", 0.0f, 1.0f, nullptr, CV_ARCHIVE);

static ConVar snd_device("snd_device","0", nullptr, CV_ARCHIVE);
static ConVar snd_3dchannels("snd_3dchannels","32", channels_callback, nullptr, CV_ARCHIVE);
static ConVar snd_samplerate("snd_samplerate", "44100", nullptr, CV_ARCHIVE);
static ConVar snd_outputchannels("snd_outputchannels", "2", "Output channels. 2 is headphones, 4 - quad surround system, 5 - 5.1 surround", CV_ARCHIVE);
static ConVar snd_effect("snd_effect", "-1", "Test sound effects", CV_CHEAT);
static ConVar snd_debug("snd_debug", "0", "Print debug info about sound", CV_CHEAT);
static ConVar snd_dopplerFac("snd_doppler_factor", "2.0f", "Doppler factor (could crash on biggest values)", CV_ARCHIVE);
static ConVar snd_doppler_soundSpeed("snd_doppler_soundSpeed", "800.0f", "Doppler speed sound (could crash on biggest values)", CV_ARCHIVE);
static ConVar job_soundLoader("job_soundLoader", "1", "Multi-threaded sound sample loading", CV_ARCHIVE);

DECLARE_CMD(snd_reloadeffects,"Play a sound",0)
{
	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	pSoundSystem->ReloadEFX();
}

DECLARE_CMD(snd_info, "Print info about sound system", 0)
{
	((DkSoundSystemLocal*)soundsystem)->PrintInfo();
}

static int out_channel_formats[][2] =
{
	{AL_FORMAT_MONO16,		AL_FORMAT_MONO_FLOAT32},		// 1
	{AL_FORMAT_STEREO16,	AL_FORMAT_STEREO_FLOAT32},		// 2
	{AL_FORMAT_STEREO16,	AL_FORMAT_STEREO_FLOAT32},		// 3, keep as 2.1
	{AL_FORMAT_QUAD16,		AL_FORMAT_QUAD32},				// 4
	{AL_FORMAT_51CHN16,		AL_FORMAT_51CHN32},				// 5.1
	{AL_FORMAT_61CHN16,		AL_FORMAT_61CHN32},				// 6.1
	{AL_FORMAT_71CHN16,		AL_FORMAT_71CHN32}				// 6.1
};

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

DkSoundSystemLocal::DkSoundSystemLocal()
{
	m_defaultParams.referenceDistance = 1.0f;
	m_defaultParams.maxDistance = 128000;

	m_defaultParams.rolloff = 2.1f;
	m_defaultParams.volume = 1.0f;
	m_defaultParams.pitch = 1.0f;
	m_defaultParams.airAbsorption = 0.0f;
	m_defaultParams.is2D = false;

	m_init = false;
	m_pauseState = false;

	m_masterVolume = 1.0f;

	m_dev = nullptr;
	m_ctx = nullptr;
}

void DkSoundSystemLocal::SetPauseState(bool pause)
{
	m_pauseState = pause;
}

bool DkSoundSystemLocal::GetPauseState()
{
	return m_pauseState;
}

bool DkSoundSystemLocal::IsInitialized()
{
	return m_init;
}

bool DkSoundSystemLocal::InitContext()
{
	Msg(" \n--------- InitSound --------- \n");

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

	// out_channel_formats snd_outputchannels
	int al_context_params[] =
	{
		ALC_FREQUENCY, snd_samplerate.GetInt(),
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

void DkSoundSystemLocal::ShutdownContext()
{
	// destroy context
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(m_ctx);
	alcCloseDevice(m_dev);
}

void DkSoundSystemLocal::Init()
{
	if (!InitContext())
	{
		MsgError("Unable to initialize sound!\n");
	}

	//Set Gain
	alListenerf(AL_GAIN, snd_mastervolume.GetFloat());
	alListenerf(AL_PITCH, 1.0f);

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	int max_sends = 0;
	alcGetIntegerv(m_dev, ALC_MAX_AUXILIARY_SENDS, 1, &max_sends);
	DevMsg(DEVMSG_SOUND,"Sound: max effect slots is: %d\n", max_sends);

	if(max_sends >= SOUND_EFX_SLOTS)
		InitEFX();

	for (int i = 0; i < MAX_AMBIENT_STREAMS; i++)
		m_ambientStreams.append(new DkSoundAmbient);

	// create channels
	for(int i = 0; i < snd_3dchannels.GetInt(); i++)
	{
		sndChannel_t *c = new sndChannel_t;
		alGenSources(1, &c->alSource);

		alSourcei(c->alSource, AL_LOOPING,AL_FALSE);
		alSourcei(c->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
		alSourcei(c->alSource, AL_AUXILIARY_SEND_FILTER_GAIN_AUTO, AL_TRUE);
		alSourcef(c->alSource, AL_MAX_GAIN, 0.9f);
		alSourcef(c->alSource, AL_DOPPLER_FACTOR, 1.0f);

		m_channels.append(c);
	}

	//Activate soundsystem
	m_init = true;
}

void DkSoundSystemLocal::InitEFX()
{
	m_currEffect = nullptr;
	m_currEffectSlotIdx = 0;
	alGenAuxiliaryEffectSlots(SOUND_EFX_SLOTS, m_effectSlots);

	// add default effect
	sndEffect_t no_eff;
	strcpy( no_eff.name, "no_effect" );
	no_eff.nAlEffect = AL_EFFECT_NULL;

	m_effects.append(no_eff);

	//
	// Load effect presets from file
	//
	kvkeybase_t* soundSettings = GetCore()->GetConfig()->FindKeyBase("Sound");

	const char* effectFilePath = soundSettings ? KV_GetValueString(soundSettings->FindKeyBase("EFXScript"), 0, nullptr) : nullptr;

	if(effectFilePath == nullptr)
	{
		MsgError("InitEFX: EQCONFIG missing Sound:EFXScript !\n");
		return;
	}

	KeyValues kv;
	if(!kv.LoadFromFile(effectFilePath))
	{
		MsgError("InitEFX: Can't init EFX from '%s'\n", effectFilePath);
		return;
	}

	for(int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* pEffectSection = kv.GetRootSection()->keys[i];

		sndEffect_t effect;
		strcpy( effect.name, pEffectSection->name );

		kvkeybase_t* pPair = pEffectSection->FindKeyBase("type");

		if(pPair)
		{
			if(!CreateALEffect(KV_GetValueString( pPair ), pEffectSection, effect))
			{
				MsgError("SOUND: Cannot create effect '%s' with type %s!\n", effect.name, KV_GetValueString( pPair ));
				continue;
			}
		}
		else
		{
			MsgError("SOUND: Effect '%s' doesn't have type!\n", effect.name);
			continue;
		}

		DevMsg(DEVMSG_SOUND,"registering sound effect '%s'\n", effect.name);

		m_effects.append(effect);
	}
}

#define AL_SAMPLE_OFFSET 0x1025

void DkSoundSystemLocal::ReloadEFX()
{
	ReleaseEffects();

	InitEFX();
}

void DkSoundSystemLocal::Update()
{
	if(!m_init)
		return;

	alDopplerFactor( snd_dopplerFac.GetFloat() );
	alSpeedOfSound( snd_doppler_soundSpeed.GetFloat() );

	// Update mixer volume and time scale
	alListenerf(AL_GAIN, snd_mastervolume.GetFloat());

	int nStreamsInUse = 0;
	int nChannelsInUse = 0;
	int nEmittersUsesChannels = 0;
	int nVirtualEmitters = 0;

	//Ambient Channels
	for (int i = 0; i < m_ambientStreams.numElem(); i++)
	{
		DkSoundAmbient* str = m_ambientStreams[i];
		str->Update();

		if (str->GetState() == SOUND_STATE_PLAYING)
			nStreamsInUse++;
	}

	if(snd_effect.GetInt() != -1)
	{
		if(snd_effect.GetInt() < m_effects.numElem())
			m_currEffect = &m_effects[snd_effect.GetInt()];

		alAuxiliaryEffectSloti(m_effectSlots[m_currEffectSlotIdx], AL_EFFECTSLOT_EFFECT, m_effects[snd_effect.GetInt()].nAlEffect);
	}

	if( !m_pauseState )
	{
		for(int i = 0; i < m_emitters.numElem(); i++)
		{
			DkSoundEmitterLocal* pEmitter = (DkSoundEmitterLocal*)m_emitters[i];

			// don't update emitters without samples
			if(!pEmitter->m_sample)
				continue;

			if(pEmitter->m_nChannel != -1)
				nEmittersUsesChannels++;

			pEmitter->Update();

			if(pEmitter->m_virtual)
				nVirtualEmitters++;
		}
	}

	// update channels
	for( int i = 0; i < m_channels.numElem(); i++ )
	{
		sndChannel_t* chnl = m_channels[i];

		if(!chnl->emitter)
		{
			alSourceRewind(chnl->alSource);
			continue;
		}

		nChannelsInUse++;

		if(m_pauseState)
		{
			if( !chnl->onGamePause && chnl->alState == AL_PLAYING )
			{
				chnl->onGamePause = true;

				alGetSourcei(chnl->alSource, AL_SAMPLE_OFFSET, &chnl->lastPauseOffsetBytes);
				alSourcePause(chnl->alSource);
			}
		}
		else if( !m_pauseState )
		{
			if( chnl->onGamePause && chnl->alState == AL_PLAYING )
			{
				chnl->onGamePause = false;

				alSourcePlay(chnl->alSource);
				alSourcei(chnl->alSource, AL_SAMPLE_OFFSET, chnl->lastPauseOffsetBytes);
			}

			// update source state
			alGetSourcei(chnl->alSource, AL_SOURCE_STATE, &chnl->alState);

			// and if it was stopped it drops channel
			if(chnl->alState == AL_STOPPED)
			{
				alSourceRewind(chnl->alSource);

				chnl->alState = AL_STOPPED;
				chnl->emitter->m_nChannel = -1;
				chnl->emitter = nullptr;
			}
		}
	}

	if(snd_debug.GetBool())
	{
		// print sound infos

		debugoverlay->Text(ColorRGBA(1,1,0,1), "-----sound debug info------");

		debugoverlay->Text(ColorRGBA(1,1,1,1), "channels in use: %d/%d (emitters use %d)", nChannelsInUse, m_channels.numElem(), nEmittersUsesChannels);
		debugoverlay->Text(ColorRGBA(1,1,1,1), "streams in use: %d/%d", m_ambientStreams.numElem(), MAX_AMBIENT_STREAMS);
		debugoverlay->Text(ColorRGBA(1,1,1,1), "emitters: %d (%d virtual)", m_emitters.numElem(), nVirtualEmitters);
		debugoverlay->Text(ColorRGBA(1,1,1,1), "samples cached: %d", m_samples.numElem());
		debugoverlay->Text(ColorRGBA(1,1,1,1), "DSP effect: %s", m_currEffect ? m_currEffect->name : "none");
		debugoverlay->Text(ColorRGBA(1,1,1,1), "effect types: %d", m_effects.numElem());
	}
}

sndEffect_t* DkSoundSystemLocal::FindEffect(const char* pszName)
{
	for(int i = 0; i < m_effects.numElem(); i++)
	{
		if(!stricmp(m_effects[i].name, pszName))
			return &m_effects[i];
	}

	return nullptr;
}

void DkSoundSystemLocal::ReleaseEffects()
{
	for (int i = 0; i < m_effects.numElem(); i++)
		alDeleteEffects(1, &m_effects[i].nAlEffect);

	alDeleteAuxiliaryEffectSlots(2, m_effectSlots);

	m_effects.clear();
}

void DkSoundSystemLocal::Shutdown()
{
	if(!m_init)
		return;

	m_init = false;

	Msg("SoundSystem shutdown...\n");

	// release all channels
	for(int i = 0; i < m_channels.numElem(); i++)
	{
		alDeleteSources(1, &m_channels[i]->alSource);
		delete m_channels[i];
	}

	m_channels.clear();

	for(int i = 0; i < m_ambientStreams.numElem(); i++)
		delete m_ambientStreams[i];

	m_ambientStreams.clear();

	ReleaseEmitters();
	ReleaseEffects();
	ReleaseSamples();

	ShutdownContext();
}

void DkSoundSystemLocal::SetListener( const Vector3D &position, const Vector3D &forwardVec, const Vector3D &upVec, const Vector3D& velocity, sndEffect_t* pEffect)
{
	if(!m_init)
		return;

	m_listenerOrigin = position;

	// set zero effect if nothing
	if(pEffect == nullptr)
		pEffect = &m_effects[0];

	if(m_currEffect != pEffect)
	{
		m_currEffectSlotIdx = !m_currEffectSlotIdx;

		if(snd_effect.GetInt() == -1)
			m_currEffect = pEffect;

		if( m_currEffect )
		{
			alAuxiliaryEffectSloti(m_effectSlots[m_currEffectSlotIdx], AL_EFFECTSLOT_EFFECT, m_currEffect->nAlEffect);
		}
		else
		{
			m_currEffectSlotIdx = 0;
			alAuxiliaryEffectSloti(m_effectSlots[0], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
			alAuxiliaryEffectSloti(m_effectSlots[1], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
		}

		// apply effect slot change to channels
		for(register int i=0; i < m_channels.numElem(); i++)
		{
			// set current effect slot
			if( m_currEffect /* && channels[i]->emitter->m_bShouldUseEffects */ )
				alSource3i(m_channels[i]->alSource, AL_AUXILIARY_SEND_FILTER, m_effectSlots[m_currEffectSlotIdx], 0, AL_FILTER_NULL);
			else
				alSource3i(m_channels[i]->alSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
		}
	}

	// setup orientation parameters
	float orient[] = { forwardVec.x, forwardVec.y, forwardVec.z, -upVec.x, -upVec.y, -upVec.z };

	alListenerfv(AL_POSITION, m_listenerOrigin);
	alListenerfv(AL_ORIENTATION, orient);
	alListenerfv(AL_VELOCITY, velocity);

	//alDopplerVelocity(length(velocity));
}

const Vector3D& DkSoundSystemLocal::GetListenerPosition() const
{
	return m_listenerOrigin;
}

// removes the sound sample
void DkSoundSystemLocal::ReleaseSample(ISoundSample* pSample)
{
	if(pSample == nullptr)
		return;

	if(pSample == &zeroSample)
		return;

	// channels
	for(int i = 0; i < m_channels.numElem(); i++)
	{
		DkSoundEmitterLocal* emitter = m_channels[i]->emitter;

		if (emitter != nullptr && emitter->m_sample == pSample)	// drop channel
			emitter->SetSample(nullptr);
	}

	// ambient
	for(int i = 0; i < m_ambientStreams.numElem(); i++)
	{
		DkSoundAmbient* str = m_ambientStreams[i];

		if(str->m_sample == pSample)
			str->SetSample(nullptr);
	}

	// Remove sample
	if(m_samples.fastRemove( pSample ))
	{
		// deallocate sample
		DkSoundSampleLocal* pSamp = (DkSoundSampleLocal*)pSample;
		pSamp->WaitForLoad();

		delete pSamp;
	}
}

// frees the sound emitter
void DkSoundSystemLocal::FreeEmitter(ISoundEmitter* pEmitter)
{
	if(pEmitter == nullptr)
		return;

	if(pEmitter->GetState() != SOUND_STATE_STOPPED)
		pEmitter->Stop();

	// Remove sample
	m_emitters.fastRemove(pEmitter);

	DkSoundEmitterLocal* pEmit = (DkSoundEmitterLocal*)pEmitter;

	delete pEmit;
}

bool DkSoundSystemLocal::IsValidEmitter(ISoundEmitter* pEmitter) const
{
	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		if(m_emitters[i] == pEmitter)
			return true;
	}

	return false;
}

void DkSoundSystemLocal::ReleaseEmitters()
{
	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		m_emitters[i]->Stop();
		delete ((DkSoundEmitterLocal*)m_emitters[i]);
	}

	m_emitters.clear();

	m_currEffect = nullptr;
	m_currEffectSlotIdx = 0;
}

// releases all sound samples
void DkSoundSystemLocal::ReleaseSamples()
{
	for(int i = 0; i < m_samples.numElem(); i++)
	{
		delete ((DkSoundSampleLocal*)m_samples[i]);
	}

	m_samples.clear();
}

ISoundEmitter* DkSoundSystemLocal::AllocEmitter()
{
	if(m_init)
	{
		int index = m_emitters.append( new DkSoundEmitterLocal );

		return m_emitters[index];
	}

	return nullptr;
}

ISoundSample* DkSoundSystemLocal::LoadSample(const char *name, int nFlags)
{
	if(!m_init)
		return (ISoundSample*) &zeroSample;

	ISoundSample *pSample = FindSampleByName(name);

	if(pSample)
		return pSample;

	if(!g_fileSystem->FileExist((_Es(SOUND_DEFAULT_PATH) + name).GetData()))
	{
		MsgError("Can't open sound file '%s', file is probably missing on disk\n",name);
		return nullptr;
	}

	DkSoundSampleLocal* pNewSample = new DkSoundSampleLocal();
	pNewSample->Init(name, nFlags);

	if(job_soundLoader.GetBool())
	{
		g_parallelJobs->AddJob( DkSoundSampleLocal::SampleLoaderJob, pNewSample );
		g_parallelJobs->Submit();
	}
	else
	{
		pNewSample->Load();
	}

	m_samples.append( pNewSample );

	return pNewSample;
}

ISoundSample* DkSoundSystemLocal::FindSampleByName(const char *name)
{
	for(int i = 0; i < m_samples.numElem();i++)
	{
		if( !stricmp(((DkSoundSampleLocal*)m_samples[i])->GetName(), name))
			return m_samples[i];
	}

	return nullptr;
}

ISoundPlayable* DkSoundSystemLocal::GetStaticStreamChannel( int channel )
{
	if(channel > -1 && channel < m_ambientStreams.numElem())
	{
		return m_ambientStreams[channel];
	}

	return nullptr;
}

int DkSoundSystemLocal::GetNumStaticStreamChannels()
{
	return m_ambientStreams.numElem();
}

int	DkSoundSystemLocal::RequestChannel(DkSoundEmitterLocal *emitter)
{
	if(m_pauseState)
		return -1;

	//Check if we are in sphere of sound
	if( (length(emitter->vPosition-m_listenerOrigin)) > emitter->m_params.maxDistance)
		return -1;

	for(int i = 0; i < m_channels.numElem(); i++)
	{
		if(m_channels[i]->emitter == nullptr)
			return i;
	}

	return -1;
}

bool DkSoundSystemLocal::CreateALEffect(const char* pszName, kvkeybase_t* pSection, sndEffect_t& effect)
{
	if (!stricmp(pszName, "reverb"))
	{
		alGenEffects(1, &effect.nAlEffect);

		alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		alEffectf(effect.nAlEffect, AL_REVERB_GAIN, KV_GetValueFloat(pSection->FindKeyBase("gain"), 0, 0.5f));
		alEffectf(effect.nAlEffect, AL_REVERB_GAINHF, KV_GetValueFloat(pSection->FindKeyBase("gain_hf"), 0, 0.5f));

		alEffectf(effect.nAlEffect, AL_REVERB_DECAY_TIME, KV_GetValueFloat(pSection->FindKeyBase("decay_time"), 0, 10.0f));
		alEffectf(effect.nAlEffect, AL_REVERB_DECAY_HFRATIO, KV_GetValueFloat(pSection->FindKeyBase("decay_hf"), 0, 0.5f));
		alEffectf(effect.nAlEffect, AL_REVERB_REFLECTIONS_DELAY, KV_GetValueFloat(pSection->FindKeyBase("reflection_delay"), 0, 0.0f));
		alEffectf(effect.nAlEffect, AL_REVERB_REFLECTIONS_GAIN, KV_GetValueFloat(pSection->FindKeyBase("reflection_gain"), 0, 0.5f));
		alEffectf(effect.nAlEffect, AL_REVERB_DIFFUSION, KV_GetValueFloat(pSection->FindKeyBase("diffusion"), 0, 0.5f));
		alEffectf(effect.nAlEffect, AL_REVERB_DENSITY, KV_GetValueFloat(pSection->FindKeyBase("density"), 0, 0.5f));
		alEffectf(effect.nAlEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, KV_GetValueFloat(pSection->FindKeyBase("airabsorption_gain"), 0, 0.5f));

		return true;
	}
	else if (!stricmp(pszName, "echo"))
	{
		alGenEffects(1, &effect.nAlEffect);

		alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

		return true;
	}
	else if (!stricmp(pszName, "none"))
	{
		effect.nAlEffect = AL_EFFECT_NULL;
		return true;
	}
	else
		return false;
}


void DkSoundSystemLocal::PrintInfo()
{
	Msg("---- sound system info ----\n");
	MsgInfo("    3d channels: %d\n", m_channels.numElem());
	MsgInfo("    stream channels: %d\n", m_ambientStreams.numElem());
	MsgInfo("    registered effects: %d\n", m_effects.numElem());
	MsgInfo("    emitters: %d\n", m_emitters.numElem());
	MsgInfo("    samples loaded: %d\n", m_samples.numElem());
}