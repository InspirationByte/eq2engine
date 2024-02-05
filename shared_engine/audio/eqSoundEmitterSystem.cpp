//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Sound Emitter System
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

using namespace Threading;
static Threading::CEqMutex s_soundEmitterSystemMutex;

CStaticAutoPtr<CSoundEmitterSystem> g_sounds;

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

DECLARE_CVAR(snd_scriptsound_debug, "0", nullptr, CV_CHEAT);
DECLARE_CVAR(snd_scriptsound_showWarnings, "0", nullptr, 0);

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

	m_channelTypes.clear();
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
	CScopedMutex m(s_soundEmitterSystemMutex);

	// remove pending sounds
	m_pendingStartSounds.clear(true);

	for (auto it = m_soundingObjects.begin(); !it.atEnd(); ++it)
	{
		CSoundingObject* obj = it.key();
		obj->StopEmitter(CSoundingObject::ID_ALL, true);
	}

	m_soundingObjects.clear(true);

	for (auto it = m_allSounds.begin(); !it.atEnd(); ++it)
	{
		SoundScriptDesc* script = *it;
		delete script;
	}
	m_allSounds.clear(true);
	m_channelTypes.clear(true);
	m_isInit = false;
}

bool CSoundEmitterSystem::PrecacheSound(const char* pszName)
{
	if (*pszName == 0)
		return false;

	// find the present sound file
	SoundScriptDesc* script = FindSoundScript(pszName);

	if (!script)
	{
		if(snd_scriptsound_showWarnings.GetBool())
			MsgWarning("PrecacheSound: No sound found with name '%s'\n", pszName);
		return false;
	}

	if(script->samples.numElem() > 0)
		return true;

	for(int i = 0; i < script->soundFileNames.numElem(); i++)
	{
		EqString soundName;
		if (script->soundFileNames[i][0] != '$')
			soundName = SOUND_DEFAULT_PATH + script->soundFileNames[i];
		else
			soundName = script->soundFileNames[i].ToCString() + 1;

		ISoundSourcePtr sample = g_audioSystem->GetSample(soundName);

		if (sample)
		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			script->samples.append(sample);
		}
	}

	return script->samples.numElem() > 0;
}

SoundScriptDesc* CSoundEmitterSystem::FindSoundScript(const char* soundName) const
{
	const int namehash = StringToHash(soundName, true );

	auto it = m_allSounds.find(namehash);
	if (it.atEnd())
		return nullptr;

	return *it;
}

// simple sound emitter
int CSoundEmitterSystem::EmitSound(EmitParams* ep)
{
	return EmitSoundInternal(ep, -1, nullptr);
}

int CSoundEmitterSystem::EmitSoundInternal(EmitParams* ep, int objUniqueId, CSoundingObject* soundingObj)
{
	ASSERT(ep);

	const bool forceStartOnUpdate = (ep->flags & EMITSOUND_FLAG_START_ON_UPDATE) || !m_updateDone.Wait(0);
	if(forceStartOnUpdate && !(ep->flags & EMITSOUND_FLAG_PENDING))
	{
		CScopedMutex m(s_soundEmitterSystemMutex);
		PendingSound& pending = m_pendingStartSounds.append();
		pending.params = (*ep);
		pending.params.flags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		pending.params.flags |= EMITSOUND_FLAG_PENDING;
		pending.objUniqueId = objUniqueId;
		pending.soundingObj.Assign(soundingObj);
		
		return CHAN_INVALID;
	}

	SoundScriptDesc* script = FindSoundScript(ep->name.ToCString());

	if (!script)
	{
		if (snd_scriptsound_showWarnings.GetBool())
			MsgError("EmitSound: unknown sound '%s'\n", ep->name.ToCString());

		return CHAN_INVALID;
	}
#ifndef _RETAIL
	if(m_isolateSound && script != m_isolateSound)
		return CHAN_INVALID;
#endif

	if(script->samples.numElem() == 0 && (ep->flags & EMITSOUND_FLAG_FORCE_CACHED))
	{
		if(snd_scriptsound_showWarnings.GetBool())
			MsgWarning("Warning! use of EMITSOUND_FLAG_FORCE_CACHED flag!\n");
		PrecacheSound(ep->name.ToCString());
	}

	if (!script->samples.numElem())
	{
		MsgWarning("WARNING! Script sound '%s' is not cached!\n", script->name.ToCString());
		return CHAN_INVALID;
	}

	const Vector3D listenerPos = g_audioSystem->GetListenerPosition();

	const bool releaseOnStop = soundingObj == nullptr || (ep->flags & EMITSOUND_FLAG_RELEASE_ON_STOP);
	const bool is2Dsound = script->is2d || (ep->flags & EMITSOUND_FLAG_FORCE_2D);
	const bool startSilent = (ep->flags & EMITSOUND_FLAG_STARTSILENT);
	bool isAudibleToStart = !startSilent;
	if (!is2Dsound)
	{
		const float distToSoundSqr = lengthSqr(ep->origin - listenerPos);
		isAudibleToStart = !startSilent && (distToSoundSqr < M_SQR(script->maxDistance));
	}
	
	if (!isAudibleToStart && releaseOnStop)
	{
		return CHAN_INVALID;
	}

	if (!releaseOnStop && !soundingObj)
	{
		ASSERT_FAIL("Invalid value for releaseOnStop set\n");
	}
	
	const int channelType = (ep->channelType != CHAN_INVALID) ? ep->channelType : script->channelType;

	SoundEmitterData tmpEmit;
	SoundEmitterData* edata = &tmpEmit;
	if(soundingObj)
	{
		const int usedSounds = soundingObj->GetChannelSoundCount(channelType);

		// if entity reached the maximum sound count for self
		// at specific channel, we stop first sound
		if(usedSounds >= m_channelTypes[channelType].limit)
			soundingObj->StopFirstEmitterByChannel(channelType);

		edata = PPNew SoundEmitterData();
		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			m_soundingObjects.insert(soundingObj);
		}
	}

	// fill in start params
	edata->script = script;
	edata->soundingObj = soundingObj;
	edata->channelType = channelType;

	const float randPitch = (ep->flags & EMITSOUND_FLAG_RANDOM_PITCH) ? RandomFloat(-0.05f, 0.05f) : 0.0f;
	edata->epPitch = ep->pitch + randPitch;
	edata->epVolume = ep->volume;
	edata->epRadiusMultiplier = ep->radiusMultiplier;
	edata->sampleId = ep->sampleId;

	edata->CreateNodeRuntime();
	
	// apply inputs (if any) to emitter data
	edata->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);
	for (int i = 0; i < ep->inputs.numElem(); ++i)
		edata->SetInputValue(ep->inputs[i].nameHash, 0, ep->inputs[i].value);
	
	if (isAudibleToStart)
		edata->UpdateNodes();

	IEqAudioSource::Params& virtualParams = edata->virtualParams;
	virtualParams.set_looping(script->loop);		// TODO: auto loop if repeat marker
	virtualParams.set_relative(is2Dsound);
	virtualParams.set_position(ep->origin);
	virtualParams.set_channel(channelType);
	virtualParams.set_state(startSilent ? IEqAudioSource::STOPPED : IEqAudioSource::PLAYING);
	virtualParams.set_releaseOnStop(releaseOnStop);
	virtualParams.set_effectSlot(ep->effectSlot);

	ep->channelType = channelType;

	if (soundingObj && channelType != CHAN_INVALID)
		++soundingObj->m_numChannelSounds[channelType];

	// try start sound
	// TODO: EMITSOUND_FLAG_STARTSILENT handling here?
	if(soundingObj)
		soundingObj->AddEmitter(objUniqueId, edata);
	else
		SwitchSourceState(edata, !isAudibleToStart);

	return ep->channelType;
}

bool CSoundEmitterSystem::SwitchSourceState(SoundEmitterData* emit, bool isVirtual)
{
	SoundScriptDesc* script = emit->script;

	CRefPtr<IEqAudioSource> soundSource = emit->soundSource;

#ifndef _RETAIL
	if (m_isolateSound && script != m_isolateSound)
	{
		g_audioSystem->DestroySource(soundSource);
		emit->soundSource = nullptr;
		return false;
	}
#endif

	// start the real sound
	if (!isVirtual && emit->virtualParams.state != IEqAudioSource::STOPPED && !soundSource)
	{
		PROF_EVENT("Emitter Switch Source - Create");

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
		emit->virtualParams.set_looping(hasLoop);
		IEqAudioSource::Params startParams = emit->virtualParams;

		CRefPtr<IEqAudioSource> source = g_audioSystem->CreateSource();
		if (!emit->soundingObj)
		{
			// no sounding object
			// set looping sound to self destruct when outside max distance

			auto callbackFunc = [script](IEqAudioSource* source, IEqAudioSource::Params& params) -> int 
			{
				return LoopSourceUpdateCallback(source, params, script);
			};

			if(hasLoop)
				source->Setup(startParams.channel, samples, callbackFunc);
			else
				source->Setup(startParams.channel, samples, nullptr);

			emit->CalcFinalParameters(1.0f, startParams);
		}
		else
		{
			auto callbackFunc = [script, emit = CWeakPtr(emit)](IEqAudioSource* source, IEqAudioSource::Params& params) -> int
			{
				return EmitterUpdateCallback(source, params, emit);
			};

			source->Setup(startParams.channel, samples, callbackFunc);
		}

		// start sound
		source->UpdateParams(startParams);

		emit->soundSource = source;

		if (snd_scriptsound_debug.GetBool())
		{
			DbgSphere()
				.Position(startParams.position).Radius(startParams.referenceDistance)
				.Color(color_yellow)
				.Time(1.0f);

			DbgText3D()
				.Position(startParams.position)
				.Distance(50.0f)
				.Time(1.0f)
				.Text("start %s v=%.2f p=%.2f", script->name.ToCString(), startParams.volume[0], startParams.pitch);
		}

		return true;
	}
	
	
	if (soundSource)
	{
		PROF_EVENT("Emitter Switch Source - Destroy");

		// stop and drop the sound
		if (isVirtual || soundSource->GetState() == IEqAudioSource::STOPPED)
		{
			g_audioSystem->DestroySource(soundSource);
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

	for (auto it = m_soundingObjects.begin(); !it.atEnd(); ++it)
	{
		CSoundingObject* obj = it.key();
		obj->StopEmitter(CSoundingObject::ID_ALL);
	}
}

int CSoundEmitterSystem::EmitterUpdateCallback(IEqAudioSource* soundSource, IEqAudioSource::Params& params, CWeakPtr<SoundEmitterData> emitter)
{
	if (!emitter)
		return 0;

	PROF_EVENT("Emitter Update Callback");

	const SoundScriptDesc* script = emitter->script;
	CSoundingObject* soundingObj = emitter->soundingObj;

	if (!script || !soundingObj)
		return 0;

	IEqAudioSource::Params& virtualParams = emitter->virtualParams;
	IEqAudioSource::Params& nodeParams = emitter->nodeParams;

	const bool loopCommandChanged = (emitter->loopCommand & LOOPCMD_FLAG_CHANGED);
	const ELoopCommand loopCommand = (ELoopCommand)(emitter->loopCommand & 31);

	if (loopCommand != LOOPCMD_NONE)
	{
		float loopRemainTimeFactor = emitter->loopCommandTimeFactor;
		const float remainTimeFactorTarget = (loopCommand == LOOPCMD_FADE_IN) ? 1.0f : 0.0f;
		const float diff = remainTimeFactorTarget - loopRemainTimeFactor;
		if (fabs(diff) > F_EPS)
		{
			loopRemainTimeFactor += g_sounds->m_deltaTime * emitter->loopCommandRatePerSecond * sign(diff);
			loopRemainTimeFactor = clamp(loopRemainTimeFactor, 0.0f, 1.0f);

			emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, loopRemainTimeFactor);
			emitter->loopCommandTimeFactor = loopRemainTimeFactor;
		}
		else
		{
			// target reached
			if (loopCommand == LOOPCMD_FADE_OUT)
				params.set_state(IEqAudioSource::STOPPED);

			emitter->loopCommand = LOOPCMD_NONE;
		}
	}
	else if(loopCommandChanged)
	{
		emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);
	}

	emitter->loopCommand &= ~LOOPCMD_FLAG_CHANGED;

	emitter->UpdateNodes();
	emitter->CalcFinalParameters(soundingObj->GetSoundVolumeScale(), params);

	if (snd_scriptsound_debug.GetBool() && !script->is2d)
	{
		DbgSphere()
			.Position(params.position).Radius(params.referenceDistance)
			.Color(color_white);

		DbgText3D()
			.Position(params.position)
			.Distance(50.0f)
			.Text("%s v=%.2f p=%.2f", script->name.ToCString(), params.volume[0], params.pitch);
	}

	// update samples volume if they were
	{
		PROF_EVENT("Emitter Update Sample Volume");
		for (int i = 0; i < soundSource->GetSampleCount(); ++i)
		{
			const float playbackPos = emitter->samplePos[i];
			soundSource->SetSampleVolume(i, emitter->sampleVolume[i]);

			if (playbackPos >= 0.0f)
			{
				soundSource->SetSamplePlaybackPosition(i, playbackPos);
				emitter->samplePos[i] = -1.0f;
			}
		}
	}

	// clear out update flags here only since we're applied everything
	nodeParams.updateFlags = 0;
	virtualParams.state = params.state;

	if (!params.relative)
	{
		PROF_EVENT("Emitter Update SwitchSourceState");
		const Vector3D listenerPos = g_audioSystem->GetListenerPosition();

		const float distToSoundSqr = lengthSqr(params.position - listenerPos);
		const float maxDistSqr = M_SQR(script->maxDistance);
		const bool isAudible = distToSoundSqr < maxDistSqr;

		// switch emitter between virtual and real here
		g_sounds->SwitchSourceState(emitter, !isAudible);
	}

	return 0;
}

int CSoundEmitterSystem::LoopSourceUpdateCallback(IEqAudioSource* source, IEqAudioSource::Params& params, SoundScriptDesc* script)
{
	if (params.relative)
		return 0;

	const Vector3D listenerPos = g_audioSystem->GetListenerPosition();

	const float distToSoundSqr = lengthSqr(params.position - listenerPos);
	const float maxDistSqr = M_SQR(script->maxDistance);
	if (distToSoundSqr > maxDistSqr)
	{
		params.set_state(IEqAudioSource::STOPPED);
	}
	return 0;
}

//
// Updates all emitters and sound system itself
//
void CSoundEmitterSystem::Update(Threading::CEqSignal* waitFor)
{
	m_deltaTime = m_updateTimer.GetTime(true);

	PROF_EVENT("Sound Emitter System Update");
	m_updateDone.Wait();

	m_updateDone.Clear();
	g_parallelJobs->AddJob(JOB_TYPE_AUDIO, [this, waitFor](void*, int i) {

		if(waitFor)
			waitFor->Wait();

		PROF_EVENT("SoundEmitterSystem Update Job");

		g_audioSystem->BeginUpdate();

		// start all pending sounds we accumulated during sound pause
		{
			CScopedMutex m(s_soundEmitterSystemMutex);

			for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
			{
				PendingSound& pending = m_pendingStartSounds[i];
				EmitSoundInternal(&pending.params, pending.objUniqueId, pending.soundingObj);
			}

			m_pendingStartSounds.clear();
		}

		const Vector3D listenerPos = g_audioSystem->GetListenerPosition();

		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			for (auto it = m_soundingObjects.begin(); !it.atEnd(); ++it)
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

void CSoundEmitterSystem::OnRemoveSoundingObject(CSoundingObject* obj)
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
			if(!CreateSoundScript(curSec, &defaultsSec))
			{
				ASSERT_FAIL("Error processing %s: sound '%s' cannot be added (already registered?)", fileName, curSec->GetName());
			}
		}
		else if(!stricmp("default", curSec->GetName()))
		{
			defaultsSec.AddKey(KV_GetValueString(curSec), KV_GetValueString(curSec, 1));
		}
	}
}

bool CSoundEmitterSystem::CreateSoundScript(const KVSection* scriptSection, const KVSection* defaultsSec)
{
	if (!scriptSection)
		return false;

	EqString soundName(_Es(scriptSection->name).LowerCase());

	const int namehash = StringToHash(soundName, true);
	if (m_allSounds.contains(namehash))
		return false;

	SoundScriptDesc* newSound = PPNew SoundScriptDesc(soundName);
	SoundScriptDesc::ParseDesc(*newSound, scriptSection, defaultsSec);

	auto sectionGetOrDefault = [scriptSection, defaultsSec](const char* name) {
		const KVSection* sec = scriptSection->FindSection(name);
		if (!sec && defaultsSec)
			sec = defaultsSec->FindSection(name);
		return sec;
	};

	newSound->maxDistance = KV_GetValueFloat(sectionGetOrDefault("maxDistance"), 0, m_defaultMaxDistance);
	newSound->startLoopTime = KV_GetValueFloat(sectionGetOrDefault("startLoopTime"), 0, 0.0f);
	newSound->stopLoopTime = KV_GetValueFloat(sectionGetOrDefault("stopLoopTime"), 0, 0.0f);
	newSound->loop = KV_GetValueBool(sectionGetOrDefault("loop"), 0, false);
	newSound->is2d = KV_GetValueBool(sectionGetOrDefault("is2d"), 0, false);

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
	return true;
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
	for (auto it = m_allSounds.begin(); !it.atEnd(); ++it)
		list.append(it.value());
}

const char* CSoundEmitterSystem::GetScriptName(SoundScriptDesc* desc)
{
	return desc->name;
}

void CSoundEmitterSystem::RestartEmittersByScript(SoundScriptDesc* script)
{
#ifndef _RETAIL
	for (auto it = m_soundingObjects.begin(); !it.atEnd(); ++it)
	{
		CSoundingObject* obj = it.key();
		for (auto emIt = obj->m_emitters.begin(); !emIt.atEnd(); ++emIt)
		{
			SoundEmitterData* emitter = emIt.value();

			if (emitter->script != script)
				continue;

			// this will ensure emitter recreation
			emitter->nodesNeedUpdate = true;
			emitter->CreateNodeRuntime();
			emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);
			SwitchSourceState(emitter, true);
			SwitchSourceState(emitter, false);
		}
	}
#endif
}