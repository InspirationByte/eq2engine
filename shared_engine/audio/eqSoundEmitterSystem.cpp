//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IEqParallelJobs.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"

#include "render/IDebugOverlay.h"

#include "utils/KeyValues.h"
#include "math/Random.h"

#include "source/snd_source.h"
#include "eqSoundEmitterSystem.h"

#define SOUND_DEFAULT_PATH		"sounds/"

using namespace Threading;
static CEqMutex s_soundEmitterSystemMutex;

static CSoundEmitterSystem s_ses;
CSoundEmitterSystem* g_sounds = &s_ses;

static const char* s_soundChannelNames[CHAN_COUNT] =
{
	"CHAN_STATIC",
	"CHAN_VOICE",
	"CHAN_ITEM",
	"CHAN_BODY",
	"CHAN_WEAPON",
	"CHAN_SIGNAL",
	"CHAN_STREAM",
};

static int s_soundChannelMaxEmitters[CHAN_COUNT] =
{
	16, // CHAN_STATIC
	1,	// CHAN_VOICE,
	3,	// CHAN_ITEM
	16,	// CHAN_BODY
	1,	// CHAN_WEAPON
	1,	// CHAN_SIGNAL
	1	// CHAN_STREAM
};

static ESoundChannelType ChannelTypeByName(const char* str)
{
	for (int i = 0; i < CHAN_COUNT; i++)
	{
		if (!stricmp(str, s_soundChannelNames[i]))
			return (ESoundChannelType)i;
	}

	return CHAN_INVALID;
}

struct soundScriptDesc_t
{
	EqString				name;

	Array<ISoundSource*>	samples{ PP_SL };
	Array<EqString>			soundFileNames{ PP_SL };

	ESoundChannelType		channelType{ CHAN_INVALID };

	float		volume{ 1.0f };
	float		atten{ 1.0f };
	float		rolloff{ 1.0f };
	float		pitch{ 1.0f };
	float		airAbsorption{ 0.0f };
	float		maxDistance{ 1.0f };

	bool		extraStreaming : 1;
	bool		loop : 1;
	bool		stopLoop : 1;
	bool		is2d : 1;
};

struct SoundEmitterData
{
	EmitParams					emitParams;					// parameters which used to start this sound
	IEqAudioSource::Params		sourceParams;

	CRefPtr<IEqAudioSource>		soundSource;				// virtual when NULL
	const soundScriptDesc_t*	script{ nullptr };			// sound script which used to start this sound
	CSoundingObject*			soundingObj{ nullptr };
	
};

DECLARE_CMD_VARIANTS(snd_test_scriptsound, "Test the scripted sound", CSoundEmitterSystem::cmd_vars_sounds_list, 0)
{
	if(CMD_ARGC > 0)
	{
		g_sounds->PrecacheSound( CMD_ARGV(0).ToCString() );

		EmitParams ep;
		ep.flags = (EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D);
		ep.pitch = 1.0;
		ep.radiusMultiplier = 1.0f;
		ep.volume = 1.0f;
		ep.origin = Vector3D(0);
		ep.name = (char*)CMD_ARGV(0).ToCString();
		ep.object = nullptr;

		if(g_sounds->EmitSound( &ep ) == CHAN_INVALID)
			MsgError("Cannot play - not valid sound '%s'\n", CMD_ARGV(0).ToCString());
	}
}

ConVar emitsound_debug("scriptsound_debug", "0", nullptr, CV_CHEAT);

ConVar snd_effectsvolume("snd_effectsvolume", "1.0", nullptr, CV_UNREGISTERED);
ConVar snd_musicvolume("snd_musicvolume", "0.5", nullptr, CV_ARCHIVE);
ConVar snd_voicevolume("snd_voicevolume", "0.5", nullptr, CV_ARCHIVE);

CSoundingObject::CSoundingObject()
{
}

CSoundingObject::~CSoundingObject()
{
	g_sounds->InvalidateSoundChannelObject(this);
}

void CSoundingObject::EmitSound(EmitParams* ep)
{
	ep->object = this;

	int channelType = g_sounds->EmitSound(ep);

	if(channelType == CHAN_INVALID)
		return;

	m_numChannelSounds[channelType]++;
}

int CSoundingObject::GetChannelSoundCount(ESoundChannelType chan)
{
	return m_numChannelSounds[chan];
}

int CSoundingObject::FirstEmitterIdxByChannelType(ESoundChannelType chan) const
{
	if (chan == CHAN_INVALID)
		return -1;

	CScopedMutex m(s_soundEmitterSystemMutex);
	for (int i = 0; i < m_emitters.numElem(); i++)
	{
		if (m_emitters[i]->emitParams.channelType == chan)
			return i;
	}

	return -1;
}

void CSoundingObject::StopFirstEmitter(ESoundChannelType chan)
{
	if (chan == CHAN_INVALID)
		return;

	// find currently playing sound index
	const int firstPlayingSound = FirstEmitterIdxByChannelType(chan);

	if (firstPlayingSound != -1)
	{
		SoundEmitterData* emitter = m_emitters[firstPlayingSound];
		if (emitter->soundSource)
		{
			emitter->soundSource->Release();
			emitter->soundSource = nullptr;
		}
		delete emitter;
		m_emitters.fastRemoveIndex(firstPlayingSound);

		--m_numChannelSounds[chan];
	}
}


//----------------------------------------------------------------------------
//
//    SOUND EMITTER SYSTEM
//
//----------------------------------------------------------------------------

void CSoundEmitterSystem::cmd_vars_sounds_list(const ConCommandBase* base, Array<EqString>& list, const char* query)
{
	for (auto it = s_ses.m_allSounds.begin(); it != s_ses.m_allSounds.end(); ++it)
	{
		list.append(it.value()->name);
	}
}

CSoundEmitterSystem::CSoundEmitterSystem()
{

}

CSoundEmitterSystem::~CSoundEmitterSystem()
{
	
}

void CSoundEmitterSystem::Init(float maxDistance)
{
	if(m_isInit)
		return;

	m_defaultMaxDistance = maxDistance;

	KVSection* soundSettings = g_eqCore->GetConfig()->FindSection("Sound");

	const char* baseScriptFilePath = soundSettings ? KV_GetValueString(soundSettings->FindSection("EmitterScripts"), 0, nullptr) : nullptr;

	if(baseScriptFilePath == nullptr)
	{
		MsgError("InitEFX: EQCONFIG missing Sound:EmitterScripts !\n");
		return;
	}

	LoadScriptSoundFile(baseScriptFilePath);

	m_isPaused = false;
	m_isInit = true;
}

void CSoundEmitterSystem::Shutdown()
{
	m_isPaused = false;

	StopAllSounds();

	for (auto it = m_allSounds.begin(); it != m_allSounds.end(); ++it)
	{
		soundScriptDesc_t* script = *it;
		for(int j = 0; j < script->samples.numElem(); j++)
			g_audioSystem->FreeSample(script->samples[j] );

		delete script;
	}
	m_allSounds.clear();

	m_isInit = false;
}

void CSoundEmitterSystem::SetPaused(bool paused)
{
	m_isPaused = paused;
}

bool CSoundEmitterSystem::IsPaused()
{
	return m_isPaused;
}

void CSoundEmitterSystem::InvalidateSoundChannelObject(CSoundingObject* pEnt)
{
	ASSERT_FAIL("UNIMPLEMENTED");
#if 0
	for (int i = 0; i < m_emitters.numElem(); i++)
	{
		if (m_emitters[i]->channelObj == pEnt)
			m_emitters[i]->channelObj = nullptr;
	}
#endif
}

void CSoundEmitterSystem::PrecacheSound(const char* pszName)
{
	// find the present sound file
	soundScriptDesc_t* pSound = FindSound(pszName);

	if(!pSound)
		return;

	if(pSound->samples.numElem() > 0)
		return;

	CScopedMutex m(s_soundEmitterSystemMutex);

	for(int i = 0; i < pSound->soundFileNames.numElem(); i++)
	{
		ISoundSource* pCachedSample = g_audioSystem->LoadSample(SOUND_DEFAULT_PATH + pSound->soundFileNames[i]);

		if (pCachedSample)
			pSound->samples.append(pCachedSample);
	}
}

soundScriptDesc_t* CSoundEmitterSystem::FindSound(const char* soundName) const
{
	EqString name(soundName);
	name = name.LowerCase();

	const int namehash = StringToHash(name.ToCString(), true );

	auto it = m_allSounds.find(namehash);
	if (it == m_allSounds.end())
		return nullptr;

	return *it;
}

// simple sound emitter
int CSoundEmitterSystem::EmitSound(EmitParams* ep)
{
	ASSERT(ep);

	if((ep->flags & EMITSOUND_FLAG_START_ON_UPDATE) || m_isPaused)
	{
		CScopedMutex m(s_soundEmitterSystemMutex);

		EmitParams newEmit = (*ep);
		newEmit.flags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		newEmit.flags |= EMITSOUND_FLAG_PENDING;

		m_pendingStartSounds.append( newEmit );
		return CHAN_INVALID;
	}

	const soundScriptDesc_t* script = FindSound(ep->name.ToCString());

	if (!script)
	{
		if (emitsound_debug.GetBool())
			MsgError("EmitSound: unknown sound '%s'\n", ep->name.ToCString());

		return CHAN_INVALID;
	}

	if(script->samples.numElem() == 0 && (ep->flags & EMITSOUND_FLAG_FORCE_CACHED))
	{
		CScopedMutex m(s_soundEmitterSystemMutex);
		MsgWarning("Warning! use of EMITSOUND_FLAG_FORCE_CACHED flag!\n");
		PrecacheSound(ep->name.ToCString());
	}

	if (!script->samples.numElem())
	{
		MsgWarning("WARNING! Script sound '%s' is not cached!\n", script->name.ToCString());
		return CHAN_INVALID;
	}

	Vector3D listenerPos, listenerVel;
	g_audioSystem->GetListener(listenerPos, listenerVel);

	const float distToSound = length(ep->origin - listenerPos);
	const bool isAudibleToStart = script->is2d || (distToSound < script->maxDistance);

	if (!isAudibleToStart && !script->loop)
	{
		return CHAN_INVALID;
	}

	SoundEmitterData tmpEmit;

	CSoundingObject* soundingObj = ep->object;
	SoundEmitterData* edata = &tmpEmit;
	if(soundingObj)
	{
		const int usedSounds = soundingObj->GetChannelSoundCount(script->channelType);

		// if entity reached the maximum sound count for self
		// at specific channel, we stop first sound
		if(usedSounds >= s_soundChannelMaxEmitters[script->channelType])
			soundingObj->StopFirstEmitter(script->channelType);

		edata = PPNew SoundEmitterData();
		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			m_soundingObjects.insert(soundingObj);
			soundingObj->m_emitters.append(edata);
		}
	}
	
	// fill in start params
	// TODO: move to separate func as we'll need a reuse
	IEqAudioSource::Params& startParams = edata->sourceParams;

	startParams.set_volume(script->volume * ep->volume);

	if (!script->loop && (ep->flags & EMITSOUND_FLAG_RANDOM_PITCH))
		startParams.set_pitch(ep->pitch * script->pitch + RandomFloat(-0.05f, 0.05f));
	else
		startParams.set_pitch(ep->pitch * script->pitch);

	startParams.set_looping(script->loop);		// TODO: auto loop if repeat marker
	// TODO: startParams.set_stopLooping(script->stopLoop); given by the loop points which are already included
	// TODO?: startParams.maxDistance = script->maxDistance;
	startParams.set_referenceDistance(script->atten * ep->radiusMultiplier);
	startParams.set_rolloff(script->rolloff);
	startParams.set_airAbsorption(script->airAbsorption);
	startParams.set_relative(script->is2d);
	startParams.set_position(ep->origin);
	startParams.set_channel((ep->channelType != CHAN_INVALID) ? ep->channelType : script->channelType);
	startParams.set_releaseOnStop(true);
	startParams.set_state(IEqAudioSource::PLAYING);

	ep->channelType = (ESoundChannelType)startParams.channel;

	edata->script = script;
	edata->soundingObj = soundingObj;
	edata->emitParams = *ep;

	// try start sound
	// TODO: EMITSOUND_FLAG_STARTSILENT handling here?
	if (!soundingObj || !(ep->flags & EMITSOUND_FLAG_STARTSILENT))
	{
		SwitchSourceState(edata, !isAudibleToStart);
	}

	return ep->channelType;
}

bool CSoundEmitterSystem::SwitchSourceState(SoundEmitterData* emit, bool isVirtual)
{
	const EmitParams& ep = emit->emitParams;

	// start the real sound
	if (!isVirtual && !emit->soundSource)
	{
		const ISoundSource* bestSample = GetBestSample(emit->script, ep.sampleId);

		if (!bestSample)
			return false;

		// sound parameters to initialize SoundEmitter
		const IEqAudioSource::Params& startParams = emit->sourceParams;

		IEqAudioSource* sndSource;

		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			sndSource = g_audioSystem->CreateSource();
			emit->soundSource = sndSource;
		}

		if (!emit->soundingObj)
		{
			// no sounding object
			// set looping sound to self destruct when outside max distance
			sndSource->Setup(startParams.channel, bestSample, emit->script->loop ? LoopSourceUpdateCallback : nullptr, const_cast<soundScriptDesc_t*>(emit->script));
		}
		else
		{
			sndSource->Setup(startParams.channel, bestSample, EmitterUpdateCallback, emit);
		}

		// start sound
		sndSource->UpdateParams(startParams, 0);

		if (emitsound_debug.GetBool())
		{
			const float boxsize = startParams.referenceDistance;

			DbgBox()
				.CenterSize(startParams.position, boxsize)
				.Color(color_white)
				.Time(1.0f);

			MsgInfo("started sound '%s' ref=%g max=%g\n", emit->script->name.ToCString(), startParams.referenceDistance);
		}

		return true;
	}
	
	// stop and drop the sound
	if (isVirtual && emit->soundSource)
	{
		emit->soundSource->Release();
		emit->soundSource = nullptr;

		return true;
	}

	return false;
}

void CSoundEmitterSystem::StopAllSounds()
{
	StopAllEmitters();
}

void CSoundEmitterSystem::StopAllEmitters()
{
	CScopedMutex m(s_soundEmitterSystemMutex);

	// remove pending sounds
	m_pendingStartSounds.clear();

	ASSERT_FAIL("UNIMPLEMENTED");

	m_soundingObjects.clear();
}


int CSoundEmitterSystem::EmitterUpdateCallback(void* obj, IEqAudioSource::Params& params)
{
	SoundEmitterData* emitter = (SoundEmitterData*)obj;
	const IEqAudioSource::Params& startParams = emitter->sourceParams;
	const soundScriptDesc_t* script = emitter->script;
	const CSoundingObject* soundingObj = emitter->soundingObj;

	params.set_volume(startParams.volume * soundingObj->GetSoundVolumeScale());

	if (!params.relative)
	{
		Vector3D listenerPos, listenerVel;
		g_audioSystem->GetListener(listenerPos, listenerVel);

		const float distToSound = lengthSqr(params.position - listenerPos);
		const float maxDistSqr = M_SQR(script->maxDistance);

		// switch emitter between virtual and real here
		g_sounds->SwitchSourceState(emitter, distToSound > maxDistSqr);
	}

	return 0;
}

int CSoundEmitterSystem::LoopSourceUpdateCallback(void* obj, IEqAudioSource::Params& params)
{
	const soundScriptDesc_t* soundScript = (const soundScriptDesc_t*)obj;

	Vector3D listenerPos, listenerVel;
	g_audioSystem->GetListener(listenerPos, listenerVel);

	const float distToSound = lengthSqr(params.position - listenerPos);
	const float maxDistSqr = M_SQR(soundScript->maxDistance);
	if (distToSound > maxDistSqr)
	{
		params.set_state(IEqAudioSource::STOPPED);
	}
	return 0;
}

//
// Updates all emitters and sound system itself
//
void CSoundEmitterSystem::Update(float pitchScale, bool force)
{
	PROF_EVENT("Sound Emitter System Update");

	// start all pending sounds we accumulated during sound pause
	if (!m_isPaused)
	{
		if (m_pendingStartSounds.numElem())
		{
			CScopedMutex m(s_soundEmitterSystemMutex);

			// play sounds
			for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
				EmitSound(&m_pendingStartSounds[i]);

			// release
			m_pendingStartSounds.clear();
		}
	}

	Vector3D listenerPos, listenerVel;
	g_audioSystem->GetListener(listenerPos, listenerVel);

	{
		// CScopedMutex m(s_soundEmitterSystemMutex);
		for (auto it = m_soundingObjects.begin(); it != m_soundingObjects.end(); ++it)
		{
			CSoundingObject* obj = it.key();
			if (!obj->m_emitters.numElem())
				m_soundingObjects.remove(it);

			// update emitters manually if they are in virtual state
			Array<SoundEmitterData*>& emitters = obj->m_emitters;
			for (int i = 0; i < emitters.numElem(); ++i)
			{
				if (emitters[i]->soundSource != nullptr)
					continue;

				IEqAudioSource::Params& params = emitters[i]->sourceParams;
				const soundScriptDesc_t* script = emitters[i]->script;

				if (script->loop)
				{
					const float distToSound = lengthSqr(params.position - listenerPos);
					const float maxDistSqr = M_SQR(script->maxDistance);

					// switch emitter between virtual and real here
					g_sounds->SwitchSourceState(emitters[i], distToSound > maxDistSqr);
				}
				else
				{
					const int chanType = emitters[i]->emitParams.channelType;

					delete emitters[i];
					emitters.fastRemoveIndex(i--);
					
					if(chanType != CHAN_INVALID)
						--obj->m_numChannelSounds[chanType];
				}

			}
		}
	}

	{
		// FIXME: this is silly and lazy
		CScopedMutex m(s_soundEmitterSystemMutex);
		g_audioSystem->Update();
	}
}

//
// Loads sound scripts
//
void CSoundEmitterSystem::LoadScriptSoundFile(const char* fileName)
{
	KeyValues kv;
	if(!kv.LoadFromFile(fileName))
	{
		MsgError("*** Error! Failed to open script sound file '%s'!\n", fileName);
		return;
	}

	DevMsg(DEVMSG_SOUND, "Loading sound script file '%s'\n", fileName);

	for(int i = 0; i <  kv.GetRootSection()->keys.numElem(); i++)
	{
		KVSection* kb = kv.GetRootSection()->keys[i];

		if(!stricmp( kb->GetName(), "include"))
			LoadScriptSoundFile( KV_GetValueString(kb) );
	}

	for(int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
	{
		KVSection* curSec = kv.GetRootSection()->keys[i];

		if(!stricmp(curSec->name, "include"))
			continue;

		CreateSoundScript(curSec);
	}
}

void CSoundEmitterSystem::CreateSoundScript(const KVSection* scriptSection)
{
	if (!scriptSection)
		return;

	EqString soundName(_Es(scriptSection->name).LowerCase());

	const int namehash = StringToHash(soundName, true);
	if (m_allSounds.find(namehash) != m_allSounds.end())
	{
		ASSERT_FAIL("Sound '%s' is already registered, please change name and references", soundName.ToCString());
		return;
	}

	soundScriptDesc_t* newSound = PPNew soundScriptDesc_t;
	newSound->name = soundName;

	newSound->volume = KV_GetValueFloat(scriptSection->FindSection("volume"), 0, 1.0f);

	newSound->atten = KV_GetValueFloat(scriptSection->FindSection("distance"), 0, m_defaultMaxDistance * 0.35f);
	newSound->maxDistance = KV_GetValueFloat(scriptSection->FindSection("maxdistance"), 0, m_defaultMaxDistance);

	newSound->pitch = KV_GetValueFloat(scriptSection->FindSection("pitch"), 0, 1.0f);
	newSound->rolloff = KV_GetValueFloat(scriptSection->FindSection("rolloff"), 0, 1.0f);
	newSound->airAbsorption = KV_GetValueFloat(scriptSection->FindSection("airabsorption"), 0, 0.0f);

	newSound->loop = KV_GetValueBool(scriptSection->FindSection("loop"), 0, false);
	newSound->stopLoop = KV_GetValueBool(scriptSection->FindSection("stopLoop"), 0, false);

	newSound->extraStreaming = KV_GetValueBool(scriptSection->FindSection("streamed"), 0, false);
	newSound->is2d = KV_GetValueBool(scriptSection->FindSection("is2d"), 0, false);

	KVSection* pKey = scriptSection->FindSection("channel");

	if (pKey)
	{
		newSound->channelType = ChannelTypeByName(KV_GetValueString(pKey));

		if (newSound->channelType == CHAN_INVALID)
		{
			Msg("Invalid channel '%s' for sound %s\n", KV_GetValueString(pKey), newSound->name.ToCString());
			newSound->channelType = CHAN_STATIC;
		}
	}
	else
		newSound->channelType = CHAN_STATIC;

	// pick 'rndwave' or 'wave' sections for lists
	pKey = scriptSection->FindSection("rndwave", KV_FLAG_SECTION);

	if (!pKey)
		pKey = scriptSection->FindSection("wave", KV_FLAG_SECTION);

	if (pKey)
	{
		for (int j = 0; j < pKey->keys.numElem(); j++)
		{
			KVSection* ent = pKey->keys[j];

			if (stricmp(ent->name, "wave"))
				continue;

			newSound->soundFileNames.append(KV_GetValueString(ent));
		}
	}
	else
	{
		pKey = scriptSection->FindSection("wave");

		if (pKey)
			newSound->soundFileNames.append(KV_GetValueString(pKey));
	}

	if (newSound->soundFileNames.numElem() == 0)
		MsgWarning("empty sound script '%s'!\n", newSound->name.ToCString());

	m_allSounds.insert(namehash, newSound);
}

//
// picks best sample randomly.
//
const ISoundSource* CSoundEmitterSystem::GetBestSample(const soundScriptDesc_t* script, int sampleId /*= -1*/) const
{
	const Array<ISoundSource*>& samples = script->samples;
	const int numSamples = samples.numElem();

	if (!numSamples)
		return nullptr;

	if (!samples.inRange(sampleId))	// if it is out of range, randomize
		sampleId = -1;

	if (sampleId < 0)
	{
		if (numSamples == 1)
			return samples[0];
		else
			return samples[RandomInt(0, numSamples - 1)];
	}
	else
		return samples[sampleId];
}