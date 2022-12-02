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

DECLARE_CMD_VARIANTS(snd_test_scriptsound, "Test the scripted sound", CSoundEmitterSystem::cmd_vars_sounds_list, 0)
{
	if (CMD_ARGC > 0)
	{
		g_sounds->PrecacheSound(CMD_ARGV(0).ToCString());

		EmitParams ep;
		ep.flags = (EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D);
		ep.name = (char*)CMD_ARGV(0).ToCString();

		if (g_sounds->EmitSound(&ep) == CHAN_INVALID)
			MsgError("Cannot play - not valid sound '%s'\n", CMD_ARGV(0).ToCString());
	}
}

ConVar snd_scriptsound_debug("snd_scriptsound_debug", "0", nullptr, CV_CHEAT);

struct SoundScriptDesc
{
	EqString				name;

	Array<ISoundSource*>	samples{ PP_SL };
	Array<EqString>			soundFileNames{ PP_SL };

	int						channelType{ CHAN_INVALID };

	float		volume{ 1.0f };
	float		atten{ 1.0f };
	float		rolloff{ 1.0f };
	float		pitch{ 1.0f };
	float		airAbsorption{ 0.0f };
	float		maxDistance{ 1.0f };

	bool		loop : 1;
	bool		is2d : 1;
	bool		randomSample : 1;

	const ISoundSource* GetBestSample(int sampleId /*= -1*/) const;
};

struct SoundEmitterData
{
	IEqAudioSource::Params	startParams;
	IEqAudioSource::Params	virtualParams;
	CRefPtr<IEqAudioSource>	soundSource;			// NULL when virtual 
	const SoundScriptDesc*	script{ nullptr };		// sound script which used to start this sound
	CSoundingObject*		soundingObj{ nullptr };
	int						channelType{ CHAN_INVALID };
	int						sampleId{ -1 };			// when randomSample and sampleId == -1, it's random
};

//-------------------------------------------

const ISoundSource* SoundScriptDesc::GetBestSample(int sampleId /*= -1*/) const
{
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

//-------------------------------------------

CSoundingObject::~CSoundingObject()
{
	g_sounds->RemoveSoundingObject(this);

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		SoundEmitterData* emitter = *it;
		if (emitter->soundSource)
		{
			emitter->soundSource->Release();
			emitter->soundSource = nullptr;
		}

		delete emitter;
	}
}

int CSoundingObject::EmitSound(int iniqueId, EmitParams* ep)
{
	if (iniqueId == -1)
		iniqueId = RandomInt(0, StringHashMask);

	const int channelType = g_sounds->EmitSound(ep, this, iniqueId & StringHashMask);

	if(channelType == CHAN_INVALID)
		return CHAN_INVALID;

	return channelType;
}

bool CSoundingObject::UpdateEmitters(const Vector3D& listenerPos)
{
	// update emitters manually if they are in virtual state
	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		bool needDelete = false;
		SoundEmitterData* emitter = *it;
		if (emitter->soundSource != nullptr)
		{
			needDelete = (emitter->soundSource->GetState() == IEqAudioSource::STOPPED);
		}
		else
		{
			IEqAudioSource::Params& params = emitter->virtualParams;
			const SoundScriptDesc* script = emitter->script;

			if (script->loop)
			{
				const float distToSound = lengthSqr(params.position - listenerPos);
				const float maxDistSqr = M_SQR(script->maxDistance);

				// switch emitter between virtual and real here
				g_sounds->SwitchSourceState(emitter, distToSound > maxDistSqr);
			}
			else
			{
				needDelete = true;
			}
		}

		if(needDelete)
		{
			const int chanType = emitter->channelType;

			delete emitter;
			m_emitters.remove(it);

			if (chanType != CHAN_INVALID)
				--m_numChannelSounds[chanType];
		}
	}

	return m_emitters.size() > 0;
}

void CSoundingObject::StopFirstEmitterByChannel(int chan)
{
	if (chan == CHAN_INVALID)
		return;

	CScopedMutex m(s_soundEmitterSystemMutex);

	// find first sound with the specific channel and kill it
	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		SoundEmitterData* emitter = *it;
		if (emitter->channelType == chan)
		{
			if (emitter->soundSource)
			{
				emitter->soundSource->Release();
				emitter->soundSource = nullptr;
			}
			delete emitter;

			m_emitters.remove(it);
			--m_numChannelSounds[chan];
			break;
		}
	}
}

#if 0
int CSoundingObject::DecodeId(int idWaveId, int& waveId)
{
	if (waveId & 0x80000000) // needs a 'SET' flag
		waveId = idWaveId >> StringHashBits & 127;

	return idWaveId & StringHashMask;
}

int CSoundingObject::EncodeId(int id, int waveId)
{
	if (waveId == -1)
		return id;

	return (id & StringHashMask) | (waveId << StringHashBits) | 0x80000000;
}
#endif

void CSoundingObject::StopEmitter(int uniqueId)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			StopEmitter(*it);
			m_emitters.remove(it);
		}
		return;
	}

	{
		CScopedMutex m(s_soundEmitterSystemMutex);
		for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
			StopEmitter(*it);
		m_emitters.clear();
	}
}

void CSoundingObject::PauseEmitter(int uniqueId)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		PauseEmitter(*it);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		PauseEmitter(*it);
}

void CSoundingObject::PlayEmitter(int uniqueId, bool rewind /*= false*/)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		PlayEmitter(*it, rewind);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		PlayEmitter(*it, rewind);
}

void CSoundingObject::StopLoop(int uniqueId)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		StopLoop(*it);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		StopLoop(*it);
}

void CSoundingObject::SetPosition(int uniqueId, const Vector3D& position)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetPosition(*it, position);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetPosition(*it, position);
}

void CSoundingObject::SetVelocity(int uniqueId, const Vector3D& velocity)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetVelocity(*it, velocity);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetVelocity(*it, velocity);
}

void CSoundingObject::SetPitch(int uniqueId, float pitch)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetPitch(*it, pitch);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetPitch(*it, pitch);
}

void CSoundingObject::SetVolume(int uniqueId, float volume)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetVolume(*it, volume);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetVolume(*it, volume);
}

void CSoundingObject::SetSampleVolume(int uniqueId, int waveId, float volume)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetSampleVolume(*it, waveId, volume);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetSampleVolume(*it, waveId, volume);
}

void CSoundingObject::SetParams(int uniqueId, IEqAudioSource::Params& params)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetParams(*it, params);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetParams(*it, params);
}

void CSoundingObject::StopEmitter(SoundEmitterData* emitter)
{
	--m_numChannelSounds[emitter->channelType];

	if (emitter->soundSource)
	{
		emitter->soundSource->Release();
		emitter->soundSource = nullptr;
	}
	delete emitter;
}

void CSoundingObject::PauseEmitter(SoundEmitterData* emitter)
{
	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::PAUSED);
	SetParams(emitter, param);
}

void CSoundingObject::PlayEmitter(SoundEmitterData* emitter, bool rewind)
{
	// TODO: check if not playing already

	// update virtual params
	emitter->virtualParams.set_state(IEqAudioSource::PLAYING);

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource::Params param;
		param.set_state(IEqAudioSource::PLAYING);

		if(rewind)
			param.updateFlags |= IEqAudioSource::UPDATE_DO_REWIND;

		emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::StopLoop(SoundEmitterData* emitter)
{
	IEqAudioSource::Params param;
	param.set_looping(false);
	SetParams(emitter, param);
}

void CSoundingObject::SetPosition(SoundEmitterData* emitter, const Vector3D& position)
{
	IEqAudioSource::Params param;
	param.set_position(position);
	SetParams(emitter, param);
}

void CSoundingObject::SetVelocity(SoundEmitterData* emitter, const Vector3D& velocity)
{
	IEqAudioSource::Params param;
	param.set_velocity(velocity);
	SetParams(emitter, param);
}

void CSoundingObject::SetPitch(SoundEmitterData* emitter, float pitch)
{
	// update virtual params
	emitter->virtualParams.set_pitch(emitter->startParams.pitch * pitch);

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource::Params param;
		param.set_pitch(emitter->startParams.pitch * pitch);

		emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::SetVolume(SoundEmitterData* emitter, float volume)
{
	// update virtual params
	emitter->virtualParams.set_volume(emitter->startParams.volume * volume);

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource::Params param;
		param.set_volume(emitter->startParams.volume * volume);

		emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::SetSampleVolume(SoundEmitterData* emitter, int waveId, float volume)
{
	// TODO: update virtual params

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource* source = emitter->soundSource;
		source->SetSampleVolume(waveId, volume);
	}
}

void CSoundingObject::SetParams(SoundEmitterData* emitter, IEqAudioSource::Params& params)
{
	// update virtual params
	emitter->virtualParams |= params;

	// update actual params
	if (emitter->soundSource)
		emitter->soundSource->UpdateParams(params);
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

void CSoundEmitterSystem::Init(float defaultMaxDistance, ChannelDef* channelDefs, int numChannels)
{
	if(m_isInit)
		return;

	m_defaultMaxDistance = defaultMaxDistance;

	for (int i = 0; i < numChannels; ++i)
	{
		m_channelTypes.append(channelDefs[i]);
		g_audioSystem->ResetMixer(channelDefs[i].id);
	}

	KVSection* soundSettings = g_eqCore->GetConfig()->FindSection("Sound");

	const char* baseScriptFilePath = soundSettings ? KV_GetValueString(soundSettings->FindSection("EmitterScripts"), 0, nullptr) : nullptr;

	if(baseScriptFilePath == nullptr)
	{
		MsgError("InitEFX: EQCONFIG missing Sound:EmitterScripts !\n");
		return;
	}

	LoadScriptSoundFile(baseScriptFilePath);

	m_isInit = true;
}

void CSoundEmitterSystem::Shutdown()
{
	StopAllSounds();

	for (auto it = m_allSounds.begin(); it != m_allSounds.end(); ++it)
	{
		SoundScriptDesc* script = *it;
		for(int j = 0; j < script->samples.numElem(); j++)
			g_audioSystem->FreeSample(script->samples[j] );

		delete script;
	}
	m_allSounds.clear();
	m_isInit = false;
	m_channelTypes.clear();
}

void CSoundEmitterSystem::PrecacheSound(const char* pszName)
{
	// find the present sound file
	SoundScriptDesc* pSound = FindSound(pszName);

	if(!pSound)
		return;

	if(pSound->samples.numElem() > 0)
		return;

	for(int i = 0; i < pSound->soundFileNames.numElem(); i++)
	{
		ISoundSource* pCachedSample = g_audioSystem->LoadSample(SOUND_DEFAULT_PATH + pSound->soundFileNames[i]);

		if (pCachedSample)
		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			pSound->samples.append(pCachedSample);
		}
	}
}

SoundScriptDesc* CSoundEmitterSystem::FindSound(const char* soundName) const
{
	const int namehash = StringToHash(soundName, true );

	auto it = m_allSounds.find(namehash);
	if (it == m_allSounds.end())
		return nullptr;

	return *it;
}

int CSoundEmitterSystem::EmitSound(EmitParams* ep)
{
	return EmitSound(ep, nullptr, -1);
}

// simple sound emitter
int CSoundEmitterSystem::EmitSound(EmitParams* ep, CSoundingObject* soundingObj, int objUniqueId)
{
	ASSERT(ep);

	if(ep->flags & EMITSOUND_FLAG_START_ON_UPDATE)
	{
		EmitParams newEmit = (*ep);
		newEmit.flags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		newEmit.flags |= EMITSOUND_FLAG_PENDING;

		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			m_pendingStartSounds.append(newEmit);
		}
		return CHAN_INVALID;
	}

	const SoundScriptDesc* script = FindSound(ep->name.ToCString());

	if (!script)
	{
		if (snd_scriptsound_debug.GetBool())
			MsgError("EmitSound: unknown sound '%s'\n", ep->name.ToCString());

		return CHAN_INVALID;
	}

	if(script->samples.numElem() == 0 && (ep->flags & EMITSOUND_FLAG_FORCE_CACHED))
	{
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
	SoundEmitterData* edata = &tmpEmit;
	if(soundingObj)
	{
		// stop the sound if it has been already started
		soundingObj->StopEmitter(objUniqueId);

		const int usedSounds = soundingObj->GetChannelSoundCount(script->channelType);

		// if entity reached the maximum sound count for self
		// at specific channel, we stop first sound
		if(usedSounds >= m_channelTypes[script->channelType].limit)
			soundingObj->StopFirstEmitterByChannel(script->channelType);

		edata = PPNew SoundEmitterData();
		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			m_soundingObjects.insert(soundingObj);
			soundingObj->m_emitters.insert(objUniqueId, edata);
		}
	}
	
	// fill in start params
	// TODO: move to separate func as we'll need a reuse
	IEqAudioSource::Params& startParams = edata->startParams;

	startParams.set_volume(script->volume);
	startParams.set_pitch(script->pitch);

	startParams.set_looping(script->loop);		// TODO: auto loop if repeat marker
	startParams.set_referenceDistance(script->atten * ep->radiusMultiplier);
	startParams.set_rolloff(script->rolloff);
	startParams.set_airAbsorption(script->airAbsorption);
	startParams.set_relative(script->is2d);
	startParams.set_position(ep->origin);
	startParams.set_channel((ep->channelType != CHAN_INVALID) ? ep->channelType : script->channelType);
	startParams.set_state(IEqAudioSource::PLAYING);
	startParams.set_releaseOnStop(true);

	ep->channelType = startParams.channel;

	const float randPitch = (ep->flags & EMITSOUND_FLAG_RANDOM_PITCH) ? RandomFloat(-0.05f, 0.05f) : 0.0f;

	edata->virtualParams = edata->startParams;
	edata->virtualParams.set_volume(edata->startParams.volume * ep->volume);
	edata->virtualParams.set_pitch(edata->startParams.pitch * ep->pitch + randPitch);
	edata->script = script;
	edata->soundingObj = soundingObj;
	edata->channelType = ep->channelType;
	edata->sampleId = ep->sampleId;

	if (soundingObj && ep->channelType != CHAN_INVALID)
		++soundingObj->m_numChannelSounds[ep->channelType];

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
	// start the real sound
	if (!isVirtual && !emit->soundSource)
	{
		const SoundScriptDesc* script = emit->script;
		FixedArray<const ISoundSource*, 16> samples;

		if (script->randomSample )
		{
			const ISoundSource* bestSample = script->GetBestSample(emit->sampleId);
			ASSERT(bestSample);	// shouldn't really happen

			samples.append(bestSample);
		}
		else
		{
			// put all samples
			samples.append(script->samples);
		}

		// sound parameters to initialize SoundEmitter
		const IEqAudioSource::Params& virtualParams = emit->virtualParams;

		IEqAudioSource* sndSource = g_audioSystem->CreateSource();

		if (!emit->soundingObj)
		{
			// no sounding object
			// set looping sound to self destruct when outside max distance
			sndSource->Setup(virtualParams.channel, samples, script->loop ? LoopSourceUpdateCallback : nullptr, const_cast<SoundScriptDesc*>(script));
		}
		else
		{
			sndSource->Setup(virtualParams.channel, samples, EmitterUpdateCallback, emit);
		}

		// start sound
		sndSource->UpdateParams(virtualParams);
		emit->soundSource = sndSource;

		if (snd_scriptsound_debug.GetBool())
		{
			const float boxsize = virtualParams.referenceDistance;

			DbgBox()
				.CenterSize(virtualParams.position, boxsize)
				.Color(color_white)
				.Time(1.0f);

			MsgInfo("started sound '%s' ref=%g max=%g\n", script->name.ToCString(), virtualParams.referenceDistance);
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
	CScopedMutex m(s_soundEmitterSystemMutex);

	// remove pending sounds
	m_pendingStartSounds.clear();

	for (auto it = m_soundingObjects.begin(); it != m_soundingObjects.end(); ++it)
	{
		CSoundingObject* obj = it.key();
		obj->StopEmitter(CSoundingObject::ID_ALL);
	}

	m_soundingObjects.clear();
}

int CSoundEmitterSystem::EmitterUpdateCallback(void* obj, IEqAudioSource::Params& params)
{
	SoundEmitterData* emitter = (SoundEmitterData*)obj;
	const IEqAudioSource::Params& virtualParams = emitter->virtualParams;
	const SoundScriptDesc* script = emitter->script;
	const CSoundingObject* soundingObj = emitter->soundingObj;

	params.set_volume(virtualParams.volume * soundingObj->GetSoundVolumeScale());

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
	const SoundScriptDesc* soundScript = (const SoundScriptDesc*)obj;

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
void CSoundEmitterSystem::Update()
{
	PROF_EVENT("Sound Emitter System Update");

	// FIXME: run in job thread?

	g_audioSystem->BeginUpdate();

	// start all pending sounds we accumulated during sound pause
	if (m_pendingStartSounds.numElem())
	{
		CScopedMutex m(s_soundEmitterSystemMutex);

		// play sounds
		for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
			EmitSound(&m_pendingStartSounds[i]);

		// release
		m_pendingStartSounds.clear();
	}

	Vector3D listenerPos, listenerVel;
	g_audioSystem->GetListener(listenerPos, listenerVel);

	{
		CScopedMutex m(s_soundEmitterSystemMutex);
		for (auto it = m_soundingObjects.begin(); it != m_soundingObjects.end(); ++it)
		{
			CSoundingObject* obj = it.key();

			if(!obj->UpdateEmitters(listenerPos))
				m_soundingObjects.remove(it);
		}
	}

	g_audioSystem->EndUpdate();
}

void CSoundEmitterSystem::RemoveSoundingObject(CSoundingObject* obj)
{
	CScopedMutex m(s_soundEmitterSystemMutex);
	m_soundingObjects.remove(obj);
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

	SoundScriptDesc* newSound = PPNew SoundScriptDesc;
	newSound->name = soundName;

	newSound->volume = KV_GetValueFloat(scriptSection->FindSection("volume"), 0, 1.0f);
	newSound->pitch = KV_GetValueFloat(scriptSection->FindSection("pitch"), 0, 1.0f);
	newSound->rolloff = KV_GetValueFloat(scriptSection->FindSection("rollOff"), 0, 1.0f);
	newSound->airAbsorption = KV_GetValueFloat(scriptSection->FindSection("airAbsorption"), 0, 0.0f);

	newSound->atten = KV_GetValueFloat(scriptSection->FindSection("distance"), 0, m_defaultMaxDistance * 0.35f);
	newSound->maxDistance = KV_GetValueFloat(scriptSection->FindSection("maxDistance"), 0, m_defaultMaxDistance);

	newSound->loop = KV_GetValueBool(scriptSection->FindSection("loop"), 0, false);
	newSound->is2d = KV_GetValueBool(scriptSection->FindSection("is2D"), 0, false);
	newSound->randomSample = false;

	KVSection* chanKey = scriptSection->FindSection("channel");

	if (chanKey)
	{
		const char* chanName = KV_GetValueString(chanKey);
		newSound->channelType = ChannelTypeByName(chanName);

		if (newSound->channelType == CHAN_INVALID)
		{
			Msg("Invalid channel '%s' for sound %s\n", chanName, newSound->name.ToCString());
			newSound->channelType = 0;
		}
	}
	else
		newSound->channelType = 0;

	// pick 'rndwave' or 'wave' sections for lists
	KVSection* waveKey = waveKey = scriptSection->FindSection("wave", KV_FLAG_SECTION); 

	if (!waveKey)
	{
		waveKey = scriptSection->FindSection("rndwave", KV_FLAG_SECTION);
		newSound->randomSample = true;
	}

	if (waveKey)
	{
		for (int j = 0; j < waveKey->keys.numElem(); j++)
		{
			const KVSection* ent = waveKey->keys[j];

			if (stricmp(ent->name, "wave"))
				continue;

			newSound->soundFileNames.append(KV_GetValueString(ent));
		}
	}
	else
	{
		waveKey = scriptSection->FindSection("wave");

		if (waveKey)
			newSound->soundFileNames.append(KV_GetValueString(waveKey));
	}

	if (newSound->soundFileNames.numElem() == 0)
		MsgWarning("empty sound script '%s'!\n", newSound->name.ToCString());

	m_allSounds.insert(namehash, newSound);
}

int CSoundEmitterSystem::ChannelTypeByName(const char* str) const
{
	for (int i = 0; i < m_channelTypes.numElem(); i++)
	{
		if (!stricmp(str, m_channelTypes[i].name))
			return m_channelTypes[i].id;
	}

	return CHAN_INVALID;
}