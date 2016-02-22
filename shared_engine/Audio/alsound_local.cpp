//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound system
//////////////////////////////////////////////////////////////////////////////////

//Local sound system

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

#ifndef NO_ENGINE
#include "IEngineGame.h"
#include "IEngineHost.h"
#include "coord.h"
#endif

static DkSoundSystemLocal	s_soundSystem;
ISoundSystem* soundsystem = &s_soundSystem;

DECLARE_CMD(snd_play,"Play a sound",0)
{
	if(!args)
		return;

	if(args->numElem() <= 0)
		return;

	ISoundSample* pSample = soundsystem->LoadSample(args->ptr()[0].GetData(),false,true,SAMPLE_FLAG_REMOVEWHENSTOPPED);
	ISoundPlayable* staticChannel = soundsystem->GetStaticStreamChannel(0);

	staticChannel->SetSample(pSample);
	staticChannel->Play();
}

void channels_callback(ConVar* pVar,char const* pszOldValue)
{
	if(stricmp(pVar->GetString(),pszOldValue))
	{
		if(soundsystem->IsInitialized())
			MsgWarning("You need to restart game to take changes\n");
	}
}

//ConVar snd_timescale("snd_timescale","1.0",0.5f,2.0f,"Sound pitch for mixer. May be combined with the sys_timescale value",CV_ARCHIVE);
//ConVar snd_timescale("snd_timescale","1.0","Sound pitch for mixer. May be combined with the sys_timescale value",CV_ARCHIVE);
extern ConVar sys_timescale;
static ConVar snd_channels("snd_channels","32",channels_callback,"Maximum allowed channels for sound system",CV_ARCHIVE);
static ConVar snd_volume("snd_volume","1.0",0.0f,1.0f,"Sound mixing volume",CV_ARCHIVE);
static ConVar snd_devicetype("snd_devicetype","0","Sound device index", CV_ARCHIVE);
static ConVar snd_samplerate("snd_samplerate","44100","Sound sample rate", CV_ARCHIVE);

static ConVar snd_outputchannels("snd_outputchannels","2","Sound output channels", CV_ARCHIVE);

DECLARE_CMD(snd_reloadeffects,"Play a sound",0)
{
	DkSoundSystemLocal* pSoundSystem = static_cast<DkSoundSystemLocal*>(soundsystem);

	pSoundSystem->ReloadEffects();
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

DkSoundSystemLocal::DkSoundSystemLocal()
{
#ifndef NO_ENGINE
	m_defaultParams.referenceDistance = 300.0f;
	m_defaultParams.maxDistance = MAX_COORD_UNITS;
#else
	m_defaultParams.referenceDistance = 1.0f;
	m_defaultParams.maxDistance = 128000;
#endif
	m_defaultParams.rolloff = 2.0f;
	m_defaultParams.volume = 1.0f;
	m_defaultParams.pitch = 1.0f;
	m_defaultParams.airAbsorption = 0.0f;
	m_defaultParams.is2D = false;

	m_bSoundInit = false;
	m_pauseState = false;

	m_fVolumeScale = 1.0f;

	m_dev = NULL;
	m_ctx = NULL;
}

void DkSoundSystemLocal::SetVolumeScale(float vol_scale)
{
	m_fVolumeScale = vol_scale;
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
	return m_bSoundInit;
}

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

void STA_CheckError(bool checkFatal = false)
{
	int alErr = alGetError();

	if(alErr == AL_NO_ERROR)
		return;

	if(checkFatal)
		MsgError("AUDIO ERROR: %s\n", getALErrorString(alErr));
	else
		DevMsg(DEVMSG_SOUND, "AUDIO ERROR: %s\n", getALErrorString(alErr));
}

void DkSoundSystemLocal::Init()
{
	Msg(" \n--------- InitSound --------- \n");

	// Init openAL

	DkList<char*> tempListChars;

	// check devices list
	char* devices = (char*)alcGetString(NULL, ALC_DEVICE_SPECIFIER);

	// go through device list (each device terminated with a single NULL, list terminated with double NULL)
	while ((*devices) != '\0')
	{
		tempListChars.append(devices);

		Msg("found sound device: %s\n", devices);

		devices += strlen(devices) + 1;
	}

	if(snd_devicetype.GetInt() >= tempListChars.numElem())
	{
		MsgError("snd_devicetype: Invalid audio device specifier\n");
		snd_devicetype.SetInt(0);
	}

	Msg("Audio device: %s\n", tempListChars[snd_devicetype.GetInt()]);

	m_dev = alcOpenDevice((ALCchar*)tempListChars[snd_devicetype.GetInt()]);

	int alErr = AL_NO_ERROR;

	if(!m_dev)
	{
		alErr = alcGetError(NULL);
		MsgError("alcOpenDevice: NULL DEVICE error: %s\n", getALCErrorString(alErr));
	}

	// out_channel_formats snd_outputchannels

	int al_context_params[] =
	{
		ALC_FREQUENCY, snd_samplerate.GetInt(),
		ALC_MAX_AUXILIARY_SENDS, 32,
		//ALC_SYNC, ALC_TRUE,
		//ALC_REFRESH, 120,
		0
	};

	m_ctx = alcCreateContext(m_dev, al_context_params);

	alErr = alcGetError(m_dev);

	if(alErr != AL_NO_ERROR)
	{
		MsgError("alcCreateContext error: %s\n", getALCErrorString(alErr));
	}

	alcMakeContextCurrent(m_ctx);

	alErr = alcGetError(m_dev);

	if(alErr != AL_NO_ERROR)
	{
		MsgError("alcMakeContextCurrent error: %s\n", getALCErrorString(alErr));
	}

	STA_CheckError(true);

	// Clear Error Code (so we can catch any new errors)
	STA_CheckError(true);

	//Set Gain
	alListenerf(AL_GAIN, snd_volume.GetFloat());
	//alListenerf(AL_METERS_PER_UNIT, METERS_PER_UNIT);

#ifndef NO_ENGINE
	if(gpGlobals)
		alListenerf(AL_PITCH, sys_timescale.GetFloat() * gpGlobals->timescale);
	else
		alListenerf(AL_PITCH, sys_timescale.GetFloat());
#else
	alListenerf(AL_PITCH, 1.0f);
#endif

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	//alDistanceModel(AL_EXPONENT_DISTANCE);

	// Create 32 ambient channels TODO: remove
	for(int i = 0; i < 8; i++)
		m_pAmbients.append(new DkSoundAmbient);

	int max_sends = 0;
	alcGetIntegerv(m_dev, ALC_MAX_AUXILIARY_SENDS, 1, &max_sends);
	DevMsg(DEVMSG_SOUND,"Sound: max effect slots is: %d\n", max_sends);

	InitEffects();

	//Create channels
	for(int i = 0; i < snd_channels.GetInt(); i++)
	{
		sndChannel_t *c = new sndChannel_t;
		alGenSources(1, &c->alSource);

		alSourcei(c->alSource, AL_LOOPING,AL_FALSE);
		alSourcei(c->alSource, AL_SOURCE_RELATIVE, AL_FALSE);
		//alSourcef(c->alSource, AL_AIR_ABSORPTION_FACTOR, 0.2f);
		//alSourcef(c->alSource, AL_ROOM_ROLLOFF_FACTOR, 0.0f);
		alSourcei(c->alSource, AL_AUXILIARY_SEND_FILTER_GAIN_AUTO, AL_TRUE);

		alSourcef(c->alSource, AL_DOPPLER_FACTOR, 1.0f);

		m_pChannels.append(c);
	}

	STA_CheckError();

	//Activate soundsystem
	m_bSoundInit = true;
}

bool CreateALEffect(const char* pszName, kvkeybase_t* pSection, sndEffect_t& effect)
{
	if(!stricmp(pszName, "reverb"))
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
	else if(!stricmp(pszName, "echo"))
	{
		alGenEffects(1, &effect.nAlEffect);

		alEffecti(effect.nAlEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

		return true;
	}
	else if(!stricmp(pszName, "none"))
	{
		effect.nAlEffect = AL_EFFECT_NULL;
		return true;
	}
	else
		return false;
}

void DkSoundSystemLocal::InitEffects()
{
	m_pCurrentEffect = NULL;
	m_nCurrentSlot = 0;
	alGenAuxiliaryEffectSlots( 2, m_nEffectSlots);

	// add default effect
	sndEffect_t no_eff;
	strcpy( no_eff.name, "no_effect" );
	no_eff.nAlEffect = AL_EFFECT_NULL;

	m_effects.append(no_eff);

	KeyValues kv;
	if(kv.LoadFromFile("scripts/soundeffects.txt"))
	{
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
	else
	{
		MsgError("DkSoundSystem::InitEffects(): Can't init effect, 'scripts/soundeffects.txt' missing\n");
	}
}

#define AL_SAMPLE_OFFSET 0x1025

void DkSoundSystemLocal::ReloadEffects()
{
	for(int i = 0; i < m_effects.numElem(); i++)
	{
		alDeleteEffects(1, &m_effects[i].nAlEffect);
	}

	alDeleteAuxiliaryEffectSlots(1, m_nEffectSlots);

	m_effects.clear();

	InitEffects();
}

static ConVar snd_effect("snd_effect", "-1", "Test sound effects", CV_CHEAT);
ConVar snd_debug("snd_debug", "0", "Print debug info about sound", CV_CHEAT);
ConVar snd_dopplerFac("snd_doppler_factor", "2.0f", "Doppler factor (could crash on biggest values)", CV_ARCHIVE);
ConVar snd_doppler_soundSpeed("snd_doppler_soundSpeed", "800.0f", "Doppler speed sound (could crash on biggest values)", CV_ARCHIVE);

void DkSoundSystemLocal::Update()
{
	if(!m_bSoundInit)
		return;

	alDopplerFactor( snd_dopplerFac.GetFloat() );
	alSpeedOfSound( snd_doppler_soundSpeed.GetFloat() );

	// Update mixer volume and time scale
	alListenerf(AL_GAIN, snd_volume.GetFloat() * m_fVolumeScale);

	//Ambient Channels
	for(int i = 0; i < m_pAmbients.numElem(); i++)
		m_pAmbients[i]->Update();

	if(snd_effect.GetInt() != -1)
	{
		if(snd_effect.GetInt() < m_effects.numElem())
			m_pCurrentEffect = &m_effects[snd_effect.GetInt()];

		alAuxiliaryEffectSloti(m_nEffectSlots[m_nCurrentSlot], AL_EFFECTSLOT_EFFECT, m_effects[snd_effect.GetInt()].nAlEffect);
	}

	int nChannelsInUse = 0;
	int nEmittersUsesChannels = 0;
	int nVirtualEmitters = 0;

	if( !m_pauseState )
	{
		for(int i = 0; i < m_pSoundEmitters.numElem(); i++)
		{
			DkSoundEmitterLocal* pEmitter = (DkSoundEmitterLocal*)m_pSoundEmitters[i];

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


	//Channels
	for( int i = 0; i < m_pChannels.numElem(); i++ )
	{
		if(!m_pChannels[i]->emitter)
			continue;

		nChannelsInUse++;

		if(m_pauseState)
		{
			if( !m_pChannels[i]->onGamePause && m_pChannels[i]->alState == AL_PLAYING )
			{
				m_pChannels[i]->onGamePause = true;

				alGetSourcei(m_pChannels[i]->alSource, AL_SAMPLE_OFFSET, &m_pChannels[i]->lastPauseOffsetBytes);
				alSourcePause(m_pChannels[i]->alSource);
			}
		}
		else if( !m_pauseState )
		{
			if( m_pChannels[i]->onGamePause && m_pChannels[i]->alState == AL_PLAYING )
			{
				m_pChannels[i]->onGamePause = false;

				alSourcePlay(m_pChannels[i]->alSource);
				alSourcei(m_pChannels[i]->alSource, AL_SAMPLE_OFFSET, m_pChannels[i]->lastPauseOffsetBytes);
			}

			// update sound state
			alGetSourcei(m_pChannels[i]->alSource, AL_SOURCE_STATE, &m_pChannels[i]->alState);

			if(m_pChannels[i]->alState == AL_STOPPED && m_pChannels[i]->emitter)
			{
				m_pChannels[i]->emitter->m_nChannel = -1;
				m_pChannels[i]->emitter = NULL; //Free channel
			}
		}
	}

	if(snd_debug.GetBool())
	{
		// print sound infos

		debugoverlay->Text(ColorRGBA(1,1,0,1), "-----sound debug info------");

		debugoverlay->Text(ColorRGBA(1,1,1,1), "channels in use: %d/%d (emitters use %d)", nChannelsInUse, m_pChannels.numElem(), nEmittersUsesChannels);
		debugoverlay->Text(ColorRGBA(1,1,1,1), "emitters: %d (%d virtual)", m_pSoundEmitters.numElem(), nVirtualEmitters);
		debugoverlay->Text(ColorRGBA(1,1,1,1), "samples cached: %d", m_pSoundSamples.numElem());
		debugoverlay->Text(ColorRGBA(1,1,1,1), "DSP effect: %s", m_pCurrentEffect ? m_pCurrentEffect->name : "none");
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

	return NULL;
}

void DkSoundSystemLocal::Shutdown()
{
	if(!m_bSoundInit)
		return;

	Msg("SoundSystem shutdown...\n");

	// deallocate channels
	for(int i = 0; i < m_pChannels.numElem(); i++)
	{
		alDeleteSources(1, &m_pChannels[i]->alSource);
		delete m_pChannels[i];
	}

	m_pChannels.clear();

	// Create 32 ambient channels TODO: remove
	for(int i = 0; i < m_pAmbients.numElem(); i++)
		delete m_pAmbients[i];

	m_pAmbients.clear();

	ReleaseEmitters();

	for(int i = 0; i < m_effects.numElem(); i++)
	{
		alDeleteEffects(1, &m_effects[i].nAlEffect);
	}

	alDeleteAuxiliaryEffectSlots(2, m_nEffectSlots);

	m_effects.clear();

	//Delete sound samples
	ReleaseSamples();

	m_pSoundSamples.clear();

	//Deactivate
	alcMakeContextCurrent(NULL);
	alcDestroyContext(m_ctx);
	alcCloseDevice(m_dev);
}

void DkSoundSystemLocal::SetListener( const Vector3D &position, const Vector3D &forwardVec, const Vector3D &upVec, const Vector3D& velocity, sndEffect_t* pEffect)
{
	if(!m_bSoundInit)
		return;

	m_listenerOrigin = position;

	// set zero effect if nothing
	if(pEffect == NULL)
		pEffect = &m_effects[0];

	if(m_pCurrentEffect != pEffect)
	{
		m_nCurrentSlot = !m_nCurrentSlot;

		if(snd_effect.GetInt() == -1)
			m_pCurrentEffect = pEffect;

		if( m_pCurrentEffect )
		{
			alAuxiliaryEffectSloti(m_nEffectSlots[m_nCurrentSlot], AL_EFFECTSLOT_EFFECT, m_pCurrentEffect->nAlEffect);
		}
		else
		{
			m_nCurrentSlot = 0;
			alAuxiliaryEffectSloti(m_nEffectSlots[0], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
			alAuxiliaryEffectSloti(m_nEffectSlots[1], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
		}

		// apply effect slot change to channels
		for(register int i=0; i < m_pChannels.numElem(); i++)
		{
			// set current effect slot
			if( m_pCurrentEffect /* && channels[i]->emitter->m_bShouldUseEffects */ )
				alSource3i(m_pChannels[i]->alSource, AL_AUXILIARY_SEND_FILTER, m_nEffectSlots[m_nCurrentSlot], 0, AL_FILTER_NULL);
			else
				alSource3i(m_pChannels[i]->alSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
		}
	}

	// setup orientation parameters
	float orient[] = { forwardVec.x, forwardVec.y, forwardVec.z, -upVec.x, -upVec.y, -upVec.z };

	alListenerfv(AL_POSITION, m_listenerOrigin);
	alListenerfv(AL_ORIENTATION, orient);
	alListenerfv(AL_VELOCITY, velocity);

	//alDopplerVelocity(length(velocity));
}

Vector3D DkSoundSystemLocal::GetListenerPosition()
{
	return m_listenerOrigin;
}

// removes the sound sample
void DkSoundSystemLocal::ReleaseSample(ISoundSample *pSample)
{
	if(pSample == NULL)
		return;

	if(pSample == &zeroSample)
		return;

	// channels
	for(int i = 0; i < m_pChannels.numElem(); i++)
	{
		if(m_pChannels[i]->emitter != NULL)
		{
			// Break if we found used sample
			if(m_pChannels[i]->emitter->m_sample == pSample)
			{
				MsgError("Programming error! You need to free sound emitter first!\n");
				return;
			}
		}
	}

	// ambient
	for(int i = 0; i < m_pAmbients.numElem(); i++)
	{
		if(m_pAmbients[i]->m_sample == pSample)
		{
			m_pAmbients[i]->Stop();
			m_pAmbients[i]->m_sample = NULL;
		}
	}

	// Remove sample
	if(m_pSoundSamples.remove( pSample ))
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
	if(pEmitter == NULL)
		return;

	if(!pEmitter->GetState() != SOUND_STATE_STOPPED)
		pEmitter->Stop();

	//Look for a channel
	for(int i = 0; i < m_pChannels.numElem(); i++)
	{
		if(m_pChannels[i]->emitter == pEmitter)
		{
			m_pChannels[i]->emitter = NULL;
		}
	}

	// Remove sample
	m_pSoundEmitters.remove(pEmitter);
	DkSoundEmitterLocal* pEmit = (DkSoundEmitterLocal*)pEmitter;

	delete pEmit;
}

bool DkSoundSystemLocal::IsValidEmitter(ISoundEmitter* pEmitter) const
{
	for(int i = 0; i < m_pSoundEmitters.numElem(); i++)
	{
		if(m_pSoundEmitters[i] == pEmitter)
			return true;
	}

	return false;
}

void DkSoundSystemLocal::ReleaseEmitters()
{
	for(int i = 0; i < m_pSoundEmitters.numElem(); i++)
	{
		delete ((DkSoundEmitterLocal*)m_pSoundEmitters[i]);
	}

	m_pSoundEmitters.clear();

	m_pCurrentEffect = NULL;
	m_nCurrentSlot = 0;
}

// releases all sound samples
void DkSoundSystemLocal::ReleaseSamples()
{
	for(int i = 0; i < m_pSoundSamples.numElem(); i++)
	{
		delete ((DkSoundSampleLocal*)m_pSoundSamples[i]);
	}

	m_pSoundSamples.clear();
}

ISoundEmitter* DkSoundSystemLocal::AllocEmitter()
{
	if(m_bSoundInit)
	{
		int index = m_pSoundEmitters.append( new DkSoundEmitterLocal );

		return m_pSoundEmitters[index];
	}

	return NULL;
}

ISoundSample* DkSoundSystemLocal::LoadSample(const char *name, bool streaming, bool looping, int nFlags)
{
	if(!m_bSoundInit)
		return (ISoundSample*) &zeroSample;

	ISoundSample *pSample = FindSampleByName(name);

	if(pSample)
		return pSample;

	if(!g_fileSystem->FileExist((_Es(SOUND_DEFAULT_PATH) + name).GetData()))
	{
		MsgError("Can't open sound file '%s', file is probably missing on disk\n",name);
		return (ISoundSample*) &zeroSample;
	}

#ifndef NO_ENGINE
	g_pEngineHost->EnterResourceLoading();
#endif

	DkSoundSampleLocal* pNewSample = new DkSoundSampleLocal();
	pNewSample->Init(name, streaming, looping, nFlags);

	g_parallelJobs->AddJob( DkSoundSampleLocal::SampleLoaderJob, pNewSample );
	g_parallelJobs->Submit();

	m_pSoundSamples.append( pNewSample );

	STA_CheckError();

#ifndef NO_ENGINE
	g_pEngineHost->EndResourceLoading();
#endif

	return pNewSample;
}

ISoundSample* DkSoundSystemLocal::FindSampleByName(const char *name)
{
	for(int i = 0; i < m_pSoundSamples.numElem();i++)
	{
		if( !stricmp(((DkSoundSampleLocal*)m_pSoundSamples[i])->GetName(), name))
			return m_pSoundSamples[i];
	}

	return NULL;
}

ISoundPlayable* DkSoundSystemLocal::GetStaticStreamChannel( int channel )
{
	if(channel > -1 && channel < m_pAmbients.numElem())
	{
		return m_pAmbients[channel];
	}

	return NULL;
}

int DkSoundSystemLocal::GetNumStaticStreamChannels()
{
	return m_pAmbients.numElem();
}

int	DkSoundSystemLocal::RequestChannel(DkSoundEmitterLocal *emitter)
{
	if(m_pauseState)
		return -1;

	//Check if we are in sphere of sound
	if( (length(emitter->vPosition-m_listenerOrigin)) > emitter->m_params.maxDistance)
		return -1;

	//Look for a channel
	for(int i = 0; i < m_pChannels.numElem(); i++)
	{
		if(!m_pChannels[i]->emitter)
		{
			m_pChannels[i]->emitter = emitter;
			return i;
		}
	}

	return -1;
}
