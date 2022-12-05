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

uint8 SoundScriptDesc::FindVariableIndex(const char* varName) const
{
	char tmpName[32]{ 0 };
	strncpy(tmpName, varName, sizeof(tmpName));

	uint arrayIdx = 0;
	// try parse array index
	char* arrSub = strchr(tmpName, '[');
	if (arrSub)
	{
		*arrSub++ = 0;
		char* numberStart = arrSub;

		// check for numeric
		if (!(*numberStart >= '0' && *numberStart <= '9'))
		{
			MsgError("sound script '%s' mixer: array index is invalid for %s\n", name.ToCString(), tmpName);
			return 0xff;
		}

		// find closing
		arrSub = strchr(arrSub, ']');
		if (!arrSub)
		{
			MsgError("sound script '%s' mixer: missing ']' for %s\n", name.ToCString(), tmpName);
			return 0xff;
		}
		*arrSub = 0;
		arrayIdx = atoi(numberStart);
	}

	const int valIdx = nodeDescs.findIndex([tmpName](const SoundNodeDesc& desc) {
		return !strcmp(desc.name, tmpName);
	});

	if (valIdx == -1)
	{
		MsgError("sound script '%s': unknown var %s\n", name.ToCString(), tmpName);
		return 0xff;
	}

	return SoundNodeDesc::PackInputIdArrIdx((uint)valIdx, arrayIdx);
}

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
	// TODO: move to separate func as we'll need a reuse
	IEqAudioSource::Params& startParams = edata->startParams;

	startParams.set_volume(script->volume);
	startParams.set_pitch(script->pitch);

	startParams.set_looping(script->loop);		// TODO: auto loop if repeat marker
	startParams.set_referenceDistance(script->atten * ep->radiusMultiplier);
	startParams.set_rolloff(script->rolloff);
	startParams.set_airAbsorption(script->airAbsorption);
	startParams.set_relative(is2Dsound);
	startParams.set_position(ep->origin);
	startParams.set_channel(channelType);
	startParams.set_state(startSilent ? IEqAudioSource::STOPPED : IEqAudioSource::PLAYING);
	startParams.set_releaseOnStop(releaseOnStop);

	ep->channelType = channelType;

	const float randPitch = (ep->flags & EMITSOUND_FLAG_RANDOM_PITCH) ? RandomFloat(-0.05f, 0.05f) : 0.0f;

	edata->virtualParams = edata->startParams;
	edata->virtualParams.set_volume(edata->startParams.volume * ep->volume);
	edata->virtualParams.set_pitch(edata->startParams.pitch * ep->pitch + randPitch);
	edata->script = script;
	edata->soundingObj = soundingObj;
	edata->channelType = channelType;
	edata->sampleId = ep->sampleId;

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
	
	// stop and drop the sound
	if (emit->soundSource)
	{
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
	IEqAudioSource::Params& virtualParams = emitter->virtualParams;
	const SoundScriptDesc* script = emitter->script;
	const CSoundingObject* soundingObj = emitter->soundingObj;

	params.set_volume(virtualParams.volume * soundingObj->GetSoundVolumeScale());
	virtualParams.state = params.state;

	if (!params.relative)
	{
		bool isAudible = true;
		if (!virtualParams.relative)
		{
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

static void ParseNodeDescs(SoundScriptDesc& scriptDesc, const KVSection* scriptSection)
{
	Array<SoundNodeDesc>& nodeDescs = scriptDesc.nodeDescs;

	// parse constants, inputs, mixers
	for (int i = 0; i < scriptSection->KeyCount(); ++i)
	{
		const KVSection& valKey = *scriptSection->keys[i];
		if (valKey.IsSection())
			continue;

		int nodeType = -1;
		if (!stricmp(valKey.name, "const"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' const: name is required\n");
				continue;
			}
			const int hashName = StringToHash(nodeName);
			nodeType = SOUND_NODE_CONST;

			SoundNodeDesc& constDesc = nodeDescs.append();
			constDesc.type = nodeType;
			strncpy(constDesc.name, nodeName, sizeof(constDesc.name));
			constDesc.name[sizeof(constDesc.name) - 1] = 0;

			constDesc.c.value = KV_GetValueFloat(&valKey, 1, 0.0f);
		}
		else if (!stricmp(valKey.name, "input"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' input: name is required\n");
				continue;
			}
			const int hashName = StringToHash(nodeName);
			nodeType = SOUND_NODE_INPUT;

			SoundNodeDesc& inputDesc = nodeDescs.append();
			inputDesc.type = nodeType;
			strncpy(inputDesc.name, nodeName, sizeof(inputDesc.name));
			inputDesc.name[sizeof(inputDesc.name) - 1] = 0;

			inputDesc.input.rMin = KV_GetValueFloat(&valKey, 1, 0.0f);
			inputDesc.input.rMax = KV_GetValueFloat(&valKey, 2, 1.0f);
		}
		else if (!stricmp(valKey.name, "mixer"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' mixer: name is required\n");
				continue;
			}
			const int hashName = StringToHash(nodeName);
			nodeType = SOUND_NODE_FUNC;
			const char* funcTypeName = KV_GetValueString(&valKey, 1, "");
			int funcType = GetSoundFuncTypeByString(funcTypeName);
			if (funcType == -1)
			{
				MsgError("sound script '%s' mixer: %s unknown\n", funcTypeName);
				continue;
			}

			SoundNodeDesc& funcDesc = nodeDescs.append();
			funcDesc.type = nodeType;
			funcDesc.subtype = funcType;

			strncpy(funcDesc.name, nodeName, sizeof(funcDesc.name));
			funcDesc.name[sizeof(funcDesc.name) - 1] = 0;

			// parse format for each type
			switch ((ESoundFuncType)funcType)
			{
				case SOUND_FUNC_ADD:
				case SOUND_FUNC_SUB:
				case SOUND_FUNC_MUL:
				case SOUND_FUNC_DIV:
				case SOUND_FUNC_MIN:
				case SOUND_FUNC_MAX:
				{
					// 2 args
					for (int v = 0; v < 2; ++v)
					{
						const char* valName = KV_GetValueString(&valKey, v + 2, nullptr);
						if (!valName)
						{
							MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString(), funcDesc.name);
							continue;
						}

						funcDesc.func.inputIds[v] = scriptDesc.FindVariableIndex(valName);
					}
					funcDesc.func.outputCount = 1;
					break;
				}
				case SOUND_FUNC_AVERAGE:
				{
					// N args
					int nArg = 0;
					for (int v = 2; v < valKey.ValueCount(); ++v)
					{
						const char* valName = KV_GetValueString(&valKey, v, nullptr);
						ASSERT(valName);
						funcDesc.func.inputIds[nArg++] = scriptDesc.FindVariableIndex(valName);
					}
					funcDesc.func.outputCount = 1;
					break;
				}
				case SOUND_FUNC_CURVE:
				{
					// input x0 y0 x1 y1 ... xN yN
					const char* inputValName = KV_GetValueString(&valKey, 2, nullptr);
					if (!inputValName)
					{
						MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString());
						continue;
					}

					funcDesc.func.outputCount = 1;
					funcDesc.func.inputIds[0] = scriptDesc.FindVariableIndex(inputValName);

					// get 
					int nArg = 0;
					for (int v = 3; v < valKey.ValueCount(); ++v)
					{
						funcDesc.func.values[nArg++] = KV_GetValueFloat(&valKey, v, 0.5f);
					}
					if (nArg & 1)
					{
						MsgError("sound script '%s' mixer %s: uneven curve arguments\n", scriptDesc.name.ToCString(), funcDesc.name);
					}
					break;
				}
				case SOUND_FUNC_FADE:
				{
					// outputCount input x0 y0 x1 y1 ... xN yN
					const int numOutputs = KV_GetValueInt(&valKey, 2, 0);
					if (!numOutputs)
					{
						MsgError("sound script '%s' mixer %s: no outputs for fade\n", scriptDesc.name.ToCString(), funcDesc.name);
						continue;
					}

					const char* inputValName = KV_GetValueString(&valKey, 3, nullptr);
					if (!inputValName)
					{
						MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString(), funcDesc.name);
						continue;
					}

					funcDesc.func.outputCount = numOutputs;
					funcDesc.func.inputIds[0] = scriptDesc.FindVariableIndex(inputValName);

					// get 
					int nArg = 0;
					for (int v = 4; v < valKey.ValueCount(); ++v)
					{
						funcDesc.func.values[nArg++] = KV_GetValueFloat(&valKey, v, 0.5f);
					}
					if (nArg & 1)
					{
						MsgError("sound script '%s' mixer %s: uneven curve arguments\n", scriptDesc.name.ToCString(), funcDesc.name);
					}
					break;
				}
			} // switch funcType
		} // const, input, mixer
	} // for
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

	SoundScriptDesc* newSound = PPNew SoundScriptDesc;
	newSound->name = soundName;
	ParseNodeDescs(*newSound, scriptSection);

	if (newSound->nodeDescs.numElem())
	{
		MsgInfo("sound %s with %d node descs\n", soundName.ToCString(), newSound->nodeDescs.numElem());
	}

	auto sectionGetOrDefault = [scriptSection, defaultsSec](const char* name) {
		const KVSection* sec = scriptSection->FindSection(name);
		if (!sec && defaultsSec)
			sec = defaultsSec->FindSection(name);
		return sec;
	};

	newSound->volume = KV_GetValueFloat(sectionGetOrDefault("volume"), 0, 1.0f);
	newSound->pitch = KV_GetValueFloat(sectionGetOrDefault("pitch"), 0, 1.0f);
	newSound->rolloff = KV_GetValueFloat(sectionGetOrDefault("rollOff"), 0, 1.0f);
	newSound->airAbsorption = KV_GetValueFloat(sectionGetOrDefault("airAbsorption"), 0, 0.0f);

	newSound->atten = KV_GetValueFloat(sectionGetOrDefault("distance"), 0, m_defaultMaxDistance * 0.35f);
	newSound->maxDistance = KV_GetValueFloat(sectionGetOrDefault("maxDistance"), 0, m_defaultMaxDistance);

	newSound->loop = KV_GetValueBool(sectionGetOrDefault("loop"), 0, false);
	newSound->is2d = KV_GetValueBool(sectionGetOrDefault("is2D"), 0, false);
	newSound->randomSample = false;

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

			IEqAudioSource::Params& startParams = emitter->startParams;

			startParams.set_volume(script->volume);
			startParams.set_pitch(script->pitch);
			startParams.set_looping(script->loop);
			startParams.set_referenceDistance(script->atten);
			startParams.set_rolloff(script->rolloff);
			startParams.set_airAbsorption(script->airAbsorption);

			emitter->virtualParams |= startParams;
			if (emitter->soundSource)
			{
				emitter->soundSource->UpdateParams(startParams);
			}
		}
	}
#endif
}