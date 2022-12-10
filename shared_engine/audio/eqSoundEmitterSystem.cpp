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
#include "eqSoundEmitterPrivateTypes.h"
#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterSystem.h"

#define SOUND_DEFAULT_PATH		"sounds/"

static CSoundEmitterSystem s_ses;
CSoundEmitterSystem* g_sounds = &s_ses;

using namespace Threading;

static void cmd_vars_sounds_list(const ConCommandBase* base, Array<EqString>& list, const char* query)
{
	Array<SoundScriptDesc*> sndList(PP_SL);
	g_sounds->GetAllSoundsList(sndList);
	for (int i = 0; i < sndList.numElem(); ++i)
		list.append(sndList[i]->name);
}

DECLARE_CMD_VARIANTS(snd_test_scriptsound, "Test the scripted sound", cmd_vars_sounds_list, 0)
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

//----------------------------------------------------------------------------
//
//    SOUND EMITTER SYSTEM
//
//----------------------------------------------------------------------------

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

	m_updateDone.Raise();

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
int CSoundEmitterSystem::EmitSound(EmitParams* ep, CSoundingObject* soundingObj, int objUniqueId, bool releaseOnStop)
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

	const bool is2Dsound = script->is2d || (ep->flags & EMITSOUND_FLAG_FORCE_2D);
	const bool startSilent = (ep->flags & EMITSOUND_FLAG_STARTSILENT);
	bool isAudibleToStart = !startSilent;
	if (!is2Dsound)
	{
		const float distToSound = length(ep->origin - listenerPos);
		isAudibleToStart = !startSilent && (distToSound < script->maxDistance);
	}
	
	if (!isAudibleToStart && releaseOnStop)
	{
		return CHAN_INVALID;
	}

	const int channelType = (ep->channelType != CHAN_INVALID) ? ep->channelType : script->channelType;

	SoundEmitterData tmpEmit;
	SoundEmitterData* edata = &tmpEmit;
	if(soundingObj)
	{
		// stop the sound if it has been already started
		soundingObj->StopEmitter(objUniqueId, true);

		const int usedSounds = soundingObj->GetChannelSoundCount(channelType);

		// if entity reached the maximum sound count for self
		// at specific channel, we stop first sound
		if(usedSounds >= m_channelTypes[channelType].limit)
			soundingObj->StopFirstEmitterByChannel(channelType);

		edata = PPNew SoundEmitterData();
		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			m_soundingObjects.insert(soundingObj);
			soundingObj->m_emitters.insert(objUniqueId, edata);
		}
	}

	// fill in start params

	edata->script = script;
	edata->soundingObj = soundingObj;
	edata->channelType = channelType;
	edata->sampleId = ep->sampleId;

	const float randPitch = (ep->flags & EMITSOUND_FLAG_RANDOM_PITCH) ? RandomFloat(-0.05f, 0.05f) : 0.0f;
	edata->epPitch = ep->pitch + randPitch;
	edata->epVolume = ep->volume;
	edata->epRadiusMultiplier = ep->radiusMultiplier;

	edata->CreateNodeRuntime();

	// apply inputs (if any) to emitter data
	for (int i = 0; i < ep->inputs.numElem(); ++i)
	{
		auto it = edata->inputs.find(ep->inputs[i].nameHash);
		if (it != edata->inputs.end())
			(*it).values[0] = ep->inputs[i].value;
	}

	IEqAudioSource::Params& virtualParams = edata->virtualParams;
	if (isAudibleToStart || soundingObj)
	{
		IEqAudioSource::Params& nodeParams = edata->nodeParams;
		edata->UpdateNodes();

		virtualParams.set_pitch(nodeParams.pitch * edata->epPitch);
		virtualParams.set_volume(nodeParams.volume * edata->epVolume);
		virtualParams.set_referenceDistance(nodeParams.referenceDistance * edata->epRadiusMultiplier);

		const int excludeFlags = (IEqAudioSource::UPDATE_PITCH | IEqAudioSource::UPDATE_VOLUME | IEqAudioSource::UPDATE_REF_DIST);
		virtualParams.merge(nodeParams, nodeParams.updateFlags & ~excludeFlags);

		virtualParams.set_rolloff(nodeParams.rolloff);
		virtualParams.set_airAbsorption(nodeParams.airAbsorption);
	}

	virtualParams.set_looping(script->loop);		// TODO: auto loop if repeat marker
	virtualParams.set_relative(is2Dsound);
	virtualParams.set_position(ep->origin);
	virtualParams.set_channel(channelType);
	virtualParams.set_state(startSilent ? IEqAudioSource::STOPPED : IEqAudioSource::PLAYING);
	virtualParams.set_releaseOnStop(releaseOnStop);

	ep->channelType = channelType;

	if (soundingObj && channelType != CHAN_INVALID)
		++soundingObj->m_numChannelSounds[channelType];

	// try start sound
	// TODO: EMITSOUND_FLAG_STARTSILENT handling here?
	if (!soundingObj)
	{
		SwitchSourceState(edata, !isAudibleToStart);
	}

	return ep->channelType;
}

bool CSoundEmitterSystem::SwitchSourceState(SoundEmitterData* emit, bool isVirtual)
{
	const SoundScriptDesc* script = emit->script;

#ifndef _RETAIL
	if (m_isolateSound && script != m_isolateSound)
	{
		if (emit->soundSource)
		{
			g_audioSystem->DestroySource(emit->soundSource);
			emit->soundSource = nullptr;
		}
		return false;
	}
#endif

	// start the real sound
	if (!isVirtual && emit->virtualParams.state != IEqAudioSource::STOPPED && !emit->soundSource)
	{
		FixedArray<const ISoundSource*, 16> samples;

		bool hasLoop = script->loop;

		if (script->randomSample || emit->sampleId != -1)
		{
			const ISoundSource* bestSample = script->GetBestSample(emit->sampleId);
			ASSERT(bestSample);	// shouldn't really happen

			samples.append(bestSample);
			hasLoop = hasLoop || bestSample->GetLoopRegions(nullptr) > 0;
		}
		else
		{
			// put all samples
			samples.append(script->samples);
			for(int i = 0; i < samples.numElem(); ++i)
				hasLoop = hasLoop || samples[i]->GetLoopRegions(nullptr) > 0;
		}

		// sound parameters to initialize SoundEmitter
		IEqAudioSource::Params& virtualParams = emit->virtualParams;
		virtualParams.set_looping(hasLoop);

		emit->soundSource = g_audioSystem->CreateSource();

		if (!emit->soundingObj)
		{
			// no sounding object
			// set looping sound to self destruct when outside max distance
			emit->soundSource->Setup(virtualParams.channel, samples, hasLoop ? LoopSourceUpdateCallback : nullptr, const_cast<SoundScriptDesc*>(script));
		}
		else
		{
			emit->soundSource->Setup(virtualParams.channel, samples, EmitterUpdateCallback, emit);
		}

		// start sound
		emit->soundSource->UpdateParams(virtualParams);

		if (snd_scriptsound_debug.GetBool())
		{
			DbgBox()
				.CenterSize(virtualParams.position, virtualParams.referenceDistance)
				.Color(color_white)
				.Time(1.0f);

			MsgInfo("started emitter %x '%s' ref=%g max=%g\n", emit, script->name.ToCString(), virtualParams.referenceDistance);
		}

		return true;
	}
	
	
	if (emit->soundSource)
	{
		// stop and drop the sound
		if (isVirtual || emit->soundSource->GetState() == IEqAudioSource::STOPPED)
		{
			g_audioSystem->DestroySource(emit->soundSource);
			emit->soundSource = nullptr;
		}

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
	CSoundingObject* soundingObj = emitter->soundingObj;

	IEqAudioSource::Params& virtualParams = emitter->virtualParams;
	IEqAudioSource::Params& nodeParams = emitter->nodeParams;
	emitter->UpdateNodes();

	// update virtual params
	if(nodeParams.updateFlags & IEqAudioSource::UPDATE_PITCH)
		virtualParams.set_pitch(nodeParams.pitch * emitter->epPitch);

	if (nodeParams.updateFlags & IEqAudioSource::UPDATE_VOLUME)
		virtualParams.set_volume(nodeParams.volume * emitter->epVolume);

	if (nodeParams.updateFlags & IEqAudioSource::UPDATE_REF_DIST)
		virtualParams.set_referenceDistance(nodeParams.referenceDistance * emitter->epRadiusMultiplier);

	// merge other params as usual
	const int excludeFlags = (IEqAudioSource::UPDATE_PITCH | IEqAudioSource::UPDATE_VOLUME | IEqAudioSource::UPDATE_REF_DIST);
	virtualParams.merge(nodeParams, nodeParams.updateFlags & ~excludeFlags);

	soundingObj->RecalcParameters(emitter, params, nodeParams.updateFlags);

	// update samples volume if they were
	IEqAudioSource* soundSource = emitter->soundSource;
	for (int i = 0; i < soundSource->GetSampleCount(); ++i)
		soundSource->SetSampleVolume(i, emitter->sampleVolume[i]);

	virtualParams.state = params.state;

	if (!params.relative)
	{
		bool isAudible = true;
		if (!virtualParams.relative)
		{
			const SoundScriptDesc* script = emitter->script;

			Vector3D listenerPos, listenerVel;
			g_audioSystem->GetListener(listenerPos, listenerVel);

			const float distToSound = lengthSqr(params.position - listenerPos);
			const float maxDistSqr = M_SQR(script->maxDistance);
			isAudible = distToSound < maxDistSqr;
		}

		// switch emitter between virtual and real here
		g_sounds->SwitchSourceState(emitter, !isAudible);
	}

	return 0;
}

int CSoundEmitterSystem::LoopSourceUpdateCallback(void* obj, IEqAudioSource::Params& params)
{
	const SoundScriptDesc* soundScript = (const SoundScriptDesc*)obj;
	if (params.relative)
		return 0;

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
	m_updateDone.Wait();

	m_updateDone.Clear();
	g_parallelJobs->AddJob(JOB_TYPE_AUDIO, [this](void*, int i) {
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
		m_updateDone.Raise();
	});
	g_parallelJobs->Submit();
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
	const KVSection* rootSec = kv.GetRootSection();
	for(int i = 0; i < rootSec->keys.numElem(); i++)
	{
		KVSection* kb = rootSec->keys[i];

		if(!stricmp( kb->GetName(), "include"))
			LoadScriptSoundFile( KV_GetValueString(kb) );
	}

	KVSection defaultsSec;

	for(int i = 0; i < rootSec->keys.numElem(); i++)
	{
		const KVSection* curSec = rootSec->keys[i];

		if (curSec->IsSection())
		{
			CreateSoundScript(curSec, &defaultsSec);
		}
		else if(!stricmp("default", curSec->GetName()))
		{
			defaultsSec.AddKey(KV_GetValueString(curSec), KV_GetValueString(curSec, 1));
		}
	}
}

void CSoundEmitterSystem::CreateSoundScript(const KVSection* scriptSection, const KVSection* defaultsSec)
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

	SoundScriptDesc* newSound = PPNew SoundScriptDesc(soundName);
	SoundScriptDesc::ParseDesc(*newSound, scriptSection, defaultsSec);

	auto sectionGetOrDefault = [scriptSection, defaultsSec](const char* name) {
		const KVSection* sec = scriptSection->FindSection(name);
		if (!sec && defaultsSec)
			sec = defaultsSec->FindSection(name);
		return sec;
	};

	newSound->maxDistance = KV_GetValueFloat(sectionGetOrDefault("maxDistance"), 0, m_defaultMaxDistance);
	newSound->loop = KV_GetValueFloat(sectionGetOrDefault("loop"), 0, false);
	newSound->is2d = KV_GetValueFloat(sectionGetOrDefault("is2d"), 0, false);
	{
		const KVSection* chanKey = sectionGetOrDefault("channel");

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
	}

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

void CSoundEmitterSystem::GetAllSoundsList(Array<SoundScriptDesc*>& list) const
{
	for (auto it = m_allSounds.begin(); it != m_allSounds.end(); ++it)
	{
		list.append(it.value());
	}
}

void CSoundEmitterSystem::RestartEmittersByScript(SoundScriptDesc* script)
{
#ifndef _RETAIL
	for (auto it = m_soundingObjects.begin(); it != m_soundingObjects.end(); ++it)
	{
		CSoundingObject* obj = it.key();
		for (auto emIt = obj->m_emitters.begin(); emIt != obj->m_emitters.end(); ++emIt)
		{
			SoundEmitterData* emitter = emIt.value();

			if (emitter->script != script)
				continue;

			// this will ensure emitter recreation
			emitter->nodesNeedUpdate = true;
			emitter->CreateNodeRuntime();
			SwitchSourceState(emitter, true);
			SwitchSourceState(emitter, false);
		}
	}
#endif
}