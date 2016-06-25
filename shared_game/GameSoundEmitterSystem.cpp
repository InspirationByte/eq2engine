//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

#include "GameSoundEmitterSystem.h"
#include "core_base_header.h"
#include "IEngineWorld.h"
#include "IEngineHost.h"
#include "IViewRenderer.h"
#include "IDebugOverlay.h"
#include "math/Random.h"

#include "Shiny.h"

#ifndef NO_ENGINE
#include "BaseEngineHeader.h"
#include "IEngineGame.h"
#include "EngineEntities.h"
#include "IDkPhysics.h"
#endif

static CSoundEmitterSystem s_ses;

CSoundEmitterSystem* ses = &s_ses;

DECLARE_CMD(test_scriptsound, "Test the scripted sound",0)
{
	if(args)
	{
		if(!args->numElem())
			return;

		ses->PrecacheSound( args->ptr()[0].GetData() );

		EmitSound_t ep;
		ep.nFlags = (EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D);
		ep.fPitch = 1.0;
		ep.fRadiusMultiplier = 1.0f;
		ep.fVolume = 1.0f;
		ep.origin = Vector3D(0);
		ep.name = (char*)args->ptr()[0].GetData();
		ep.pObject = NULL;

		if(ses->EmitSound( &ep ) == CHAN_INVALID)
			MsgError("not valid sound '%s'\n", args->ptr()[0].GetData());
	}
}

ConVar emitsound_debug("scriptsound_debug", "0", NULL, CV_CHEAT);
ConVar snd_musicvolume("snd_musicvolume", "0.5", NULL, CV_ARCHIVE);

bool CSoundController::IsStopped() const
{
	if(!m_emitData)
		return true;

	if(!m_emitData->pEmitter)
		return true;

	return (m_emitData->pEmitter->GetState() == SOUND_STATE_STOPPED);
}

void CSoundController::StartSound(const char* newSoundName)
{
	// if stopped
	if(!m_emitData)
	{
		if (newSoundName != NULL)
		{
			m_soundName = newSoundName;
			m_emitParams.name = (char*)m_soundName.c_str();
		}

		// sound must be aquired
		int chan = ses->EmitSound(&m_emitParams);

		// can't start
		if(chan == CHAN_INVALID)
		{
			MsgError("Can't emit sound '%s', CHAN_INVALID\n", m_emitParams.name);
			return;
		}

		if(m_emitParams.emitterIndex < 0 || m_emitParams.emitterIndex > ses->m_pCurrentTempEmitters.numElem()-1)
		{
			MsgError("Can't emit sound '%s', no out of emitters\n", m_emitParams.name);
			return;
		}

		// get emitter
		m_emitData = ses->m_pCurrentTempEmitters[m_emitParams.emitterIndex];

		// set controller
		m_emitData->pController = this;

		return;
	}

	if(!m_emitData->pEmitter)
	{
		m_emitData = NULL;
		return;
	}

	// resume previous sample
	m_emitData->pEmitter->Play();
}

void CSoundController::PauseSound()
{
	if(!m_emitData)
		return;

	if(!m_emitData->pEmitter)
		return;

	ASSERT(soundsystem->IsValidEmitter( m_emitData->pEmitter ));

	m_emitData->pEmitter->Pause();
}

void CSoundController::StopSound(bool force)
{
	if(!m_emitData)
	{
		return;
	}

	if(!m_emitData->pEmitter)
	{
		m_emitData->pController = NULL;
		m_emitParams.emitterIndex = -1;
		return;
	}

	if(!force && m_emitData->script && m_emitData->script->stopLoop)
		m_emitData->pEmitter->StopLoop();
	else
		m_emitData->pEmitter->Stop();

	// at stop we invalidate channel and detach emitter
	m_emitData->pController = NULL;
	m_emitParams.emitterIndex = -1;
	m_emitData = NULL;
}

void CSoundController::StopLoop()
{
	if(!m_emitData)
	{
		return;
	}

	if(!m_emitData->pEmitter)
	{
		m_emitData->pController = NULL;
		m_emitParams.emitterIndex = -1;
		return;
	}

	m_emitData->pEmitter->StopLoop();
}

void CSoundController::SetPitch(float fPitch)
{
	if(!m_emitData)
		return;

	if(!m_emitData->pEmitter)
		return;

	float fSoundPitch = m_emitData->script->fPitch;

	soundParams_t params;

	m_emitData->pEmitter->GetParams(&params);

	params.pitch = fSoundPitch*fPitch;

	m_emitData->pEmitter->SetParams(&params);
}

void CSoundController::SetVolume(float fVolume)
{
	if(!m_emitData)
		return;

	if(!m_emitData->pEmitter)
		return;

	float fSoundVolume = m_emitData->origVolume;

	if(m_emitData->pObject)
		fSoundVolume *= m_emitData->pObject->GetSoundVolumeScale();

	soundParams_t params;
	m_emitData->pEmitter->GetParams(&params);

	params.volume = fSoundVolume*fVolume;

	m_emitData->pEmitter->SetParams(&params);
}

void CSoundController::SetOrigin(const Vector3D& origin)
{
	m_emitParams.origin = origin;

	if(!m_emitData)
		return;

	m_emitData->origin = origin;
}

void CSoundController::SetVelocity(const Vector3D& velocity)
{
	if(!m_emitData)
		return;

	m_emitData->velocity = velocity;
}

//----------------------------------------------------------------------------

CSoundChannelObject::CSoundChannelObject() : m_volumeScale(1.0f)
{
	memset(m_numChannelSounds, 0, sizeof(m_numChannelSounds));
}

void CSoundChannelObject::EmitSound(const char* name)
{
	EmitSound_t ep;
	ep.pObject = this;
	ep.name = (char*)name;

	int channel = ses->EmitSound(&ep);

	if(channel == CHAN_INVALID)
		return;

	m_numChannelSounds[channel]++;
}

void CSoundChannelObject::EmitSoundWithParams(EmitSound_t *ep)
{
	ep->pObject = this;

	int channel = ses->EmitSound(ep);

	if(channel == CHAN_INVALID)
		return;

	m_numChannelSounds[channel]++;
}

int CSoundChannelObject::GetChannelSoundCount(Channel_t chan)
{
	return m_numChannelSounds[chan];
}

void CSoundChannelObject::DecrementChannelSoundCount(Channel_t chan)
{
	if(chan == CHAN_INVALID)
		return;

	if(m_numChannelSounds[chan] > 0)
		m_numChannelSounds[chan]--;
}

void EmitSound_t::Init( const char* pszName, const Vector3D& pos, float volume, float pitch, float radius, CSoundChannelObject* obj, int flags )
{
	name = pszName;
	fVolume = volume;
	fPitch = pitch;
	fRadiusMultiplier = radius;
	pObject = NULL;
	nFlags = flags;	// all sounds forced to use simple occlusion
	origin = pos;
	sampleId = -1;
	emitterIndex = -1;
}

//----------------------------------------------------------------------------
//
//    SOUND EMITTER SYSTEM
//
//----------------------------------------------------------------------------

void CSoundEmitterSystem::Init(float maxDistance)
{
	if(m_isInit)
		return;

	m_defaultMaxDistance = maxDistance;

	LoadScriptSoundFile("scripts/sounds.txt");

	m_bViewIsAvailable = false;
	m_nRooms = 0;

	m_isInit = true;
}

void CSoundEmitterSystem::Shutdown()
{
	for(int i = 0; i < m_pSoundControllerList.numElem(); i++)
	{
		m_pSoundControllerList[i]->StopSound(true);

		delete m_pSoundControllerList[i];
	}

	m_pSoundControllerList.clear();

	for(int i = 0; i < m_pCurrentTempEmitters.numElem(); i++)
	{
		m_pCurrentTempEmitters[i]->pEmitter->Pause();
		m_pCurrentTempEmitters[i]->pEmitter->Stop();
		soundsystem->FreeEmitter(m_pCurrentTempEmitters[i]->pEmitter);
		m_pCurrentTempEmitters[i]->pEmitter = NULL;

		delete m_pCurrentTempEmitters[i];
	}

	m_pCurrentTempEmitters.clear();

	for(int i = 0; i < m_scriptsoundlist.numElem(); i++)
	{
		for(int j = 0; j < m_scriptsoundlist[i]->pSamples.numElem(); j++)
			soundsystem->ReleaseSample( m_scriptsoundlist[i]->pSamples[j] );

		delete [] m_scriptsoundlist[i]->pszName;
		delete m_scriptsoundlist[i];
	}

	m_scriptsoundlist.clear();

	m_isInit = false;
}

int CSoundEmitterSystem::GetEmitterIndexByEntityAndChannel(CSoundChannelObject* pEnt, Channel_t chan)
{
	for(int i = 0; i < m_pCurrentTempEmitters.numElem(); i++)
	{
		if(m_pCurrentTempEmitters[i]->pObject == pEnt && m_pCurrentTempEmitters[i]->channel == chan)
			return i;
	}

	return -1;
}

ISoundController* CSoundEmitterSystem::CreateSoundController(EmitSound_t *ep)
{
	ASSERT(ep);
	ASSERT(ep->name);

	scriptsounddata_t* pScriptSound = FindSound(ep->name);

	// check
	if(!pScriptSound)
	{
		MsgError("CreateSoundController: Couldn't find script sound '%s'!\n", ep->name);
		return NULL;
	}

	CSoundController* pController = new CSoundController();

	m_pSoundControllerList.append(pController);

	pController->m_soundName = ep->name; // copy sound name since the params can use non-permanent adresses
	pController->m_emitParams = *ep;
	pController->m_emitParams.name = (char*)pController->m_soundName.c_str();

	return pController;
}

void CSoundEmitterSystem::RemoveSoundController(ISoundController* cont)
{
	if(!cont)
		return;

	cont->StopSound();

	delete cont;

	int idx = m_pSoundControllerList.findIndex(cont);

	if( idx != -1)
		m_pSoundControllerList.fastRemoveIndex( idx );
}

void CSoundEmitterSystem::PrecacheSound(const char* pszName)
{
	// find the present sound file
	scriptsounddata_t* pSound = FindSound(pszName);

	if(!pSound)
		return;

	if(pSound->pSamples.numElem() > 0)
		return;

#ifndef NO_ENGINE
	g_pEngineHost->EnterResourceLoading();
#endif

	for(int i = 0; i < pSound->soundFileNames.numElem(); i++)
	{
		int flags = (pSound->extraStreaming ? SAMPLE_FLAG_STREAMED : 0) | (pSound->loop ? SAMPLE_FLAG_LOOPING : 0);

		ISoundSample* pCachedSample = soundsystem->LoadSample(pSound->soundFileNames[i].GetData(), flags);
		if(pCachedSample)
			pSound->pSamples.append(pCachedSample);
	}

#ifndef NO_ENGINE
	 if(gpGlobals->curtime > 0)
		 MsgWarning("WARNING! Late precache of sound '%s'\n", pszName);

	 g_pEngineHost->EndResourceLoading();
#endif
}

scriptsounddata_t* CSoundEmitterSystem::FindSound(const char* soundName)
{
	EqString sname(soundName);
	sname = sname.LowerCase();

	int snameHash = StringToHash( sname.c_str() );

	for(int i = 0; i < m_scriptsoundlist.numElem(); i++)
	{
		if(m_scriptsoundlist[i]->namehash == snameHash)
			return m_scriptsoundlist[i];
	}

	return NULL;
}

#ifndef NO_ENGINE
extern CWorldInfo* g_pWorldInfo;
#endif

ConVar snd_roloff("snd_roloff", "2.0");

// simple sound emitter
int CSoundEmitterSystem::EmitSound(EmitSound_t* emit)
{
	ASSERT(emit);

	// quickly set
	emit->emitterIndex = -1;

	if(emit->nFlags & EMITSOUND_FLAG_START_ON_UPDATE)
	{
		EmitSound_t newEmit = (*emit);
		newEmit.nFlags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		m_pUnreleasedSounds.append( newEmit );

		return CHAN_INVALID;
	}

#ifndef NO_ENGINE
	// don't start sound if game isn't started
	if(gpGlobals->curtime <= 0)
		return CHAN_INVALID;

#endif // NO_ENGINE

	scriptsounddata_t* pScriptSound = FindSound(emit->name);

	if(pScriptSound)
	{
		if(pScriptSound->pSamples.numElem() == 0 && (emit->nFlags & EMITSOUND_FLAG_FORCE_CACHED))
		{
			MsgWarning("Warning! use of EMITSOUND_FLAG_FORCE_CACHED flag!\n");
			PrecacheSound( emit->name );
		}

		if(pScriptSound->pSamples.numElem() == 0)
		{
			MsgWarning("WARNING! Script sound '%s' is not cached!\n", emit->name);
			return CHAN_INVALID;
		}

		// use worldinfo for static sounds that does not have entites
		CSoundChannelObject* pChanObj = emit->pObject;

		//if(!pSoundChannelEntity)
		//	pSoundChannelEntity = g_pWorldInfo;

		if(pChanObj)
		{
			// if entity reached the maximum sound count for self
			if(pChanObj->GetChannelSoundCount(pScriptSound->channel) >= channel_max_sounds[pScriptSound->channel])
			{
				// find currently playing sound index
				int firstPlayingSound = GetEmitterIndexByEntityAndChannel( pChanObj, pScriptSound->channel );
				if(firstPlayingSound != -1)
				{
					EmitterData_t* substEmitter = m_pCurrentTempEmitters[firstPlayingSound];

					// if index is valid, shut up this sound
					if(substEmitter->pController)
						substEmitter->pController->StopSound();
					else
						substEmitter->pEmitter->Stop();

					// and remove
					soundsystem->FreeEmitter( substEmitter->pEmitter );
					substEmitter->pEmitter = NULL;
					delete substEmitter;
					m_pCurrentTempEmitters.fastRemoveIndex(firstPlayingSound);

					// pChanObj sound count
					pChanObj->DecrementChannelSoundCount(pScriptSound->channel);
				}
			}
		}

		// alloc SoundEmitterSystem emitter data
		EmitterData_t* edata = new EmitterData_t();

		edata->pEmitter = soundsystem->AllocEmitter();
		edata->pObject = emit->pObject;

		edata->origin = emit->origin;
		edata->velocity = vec3_zero;		// only accessible by sound controller
		edata->interpolatedOrigin = emit->origin;
		edata->origVolume = emit->fVolume * pScriptSound->fVolume;
		edata->channel = pScriptSound->channel;
		edata->script = pScriptSound;

		edata->pController = NULL;

		edata->emitSoundData = *emit;

		// make new sound parameters
		soundParams_t sParams;

		sParams.maxDistance			= pScriptSound->fMaxDistance;
		sParams.referenceDistance	= pScriptSound->fAtten * emit->fRadiusMultiplier;
		sParams.pitch				= emit->fPitch * pScriptSound->fPitch;
		sParams.rolloff				= pScriptSound->fRolloff;
		sParams.volume				= edata->origVolume;
		sParams.airAbsorption		= pScriptSound->fAirAbsorption;
		sParams.is2D				= pScriptSound->is2d;

		if(emit->nFlags & EMITSOUND_FLAG_STARTSILENT)
			sParams.volume = 0.0f;

		// update emitter before playing
		UpdateEmitter( edata, sParams, true );

		if(emitsound_debug.GetBool())
		{
			float boxsize = sParams.referenceDistance;

			debugoverlay->Box3D(edata->origin-boxsize, edata->origin+boxsize, ColorRGBA(1,1,1,0.5f), 1.0f);
			MsgInfo("started sound '%s' ref=%g max=%g\n", pScriptSound->pszName, sParams.referenceDistance, sParams.maxDistance);
		}

		ISoundSample* bestSample;

#pragma todo("randomization flag for sounding pitch")
		//if(!pScriptSound->loop)
		//	sParams.pitch += RandomFloat(-0.05f,0.05f);

		if(emit->sampleId > pScriptSound->pSamples.numElem()-1)
			emit->sampleId = -1;

		if(emit->sampleId < 0)
		{
			if(pScriptSound->pSamples.numElem() == 1 )
				bestSample = pScriptSound->pSamples[0];
			else
				bestSample = pScriptSound->pSamples[RandomInt(0,pScriptSound->pSamples.numElem()-1)];
		}
		else
		{
			bestSample = pScriptSound->pSamples[ emit->sampleId ];
		}

		edata->pEmitter->SetSample(bestSample);
		edata->pEmitter->SetParams(&sParams);

		edata->pEmitter->Play();

		int eIndex = m_pCurrentTempEmitters.append(edata);
		emit->emitterIndex = eIndex;

		return edata->channel;
	}
	else if(emitsound_debug.GetBool())
		MsgError("EmitSound: unknown sound '%s'\n", emit->name);

	return CHAN_INVALID;
}

void CSoundEmitterSystem::Emit2DSound(EmitSound_t* emit, int channel)
{
	ASSERT(emit);

	// quickly set
	emit->emitterIndex = -1;

#ifndef NO_ENGINE
	// don't start sound if game isn't started
	if (gpGlobals->curtime <= 0)
		return;

#endif // NO_ENGINE

	scriptsounddata_t* pScriptSound = FindSound(emit->name);

	if (pScriptSound)
	{
		if(pScriptSound->pSamples.numElem() == 0 && (emit->nFlags & EMITSOUND_FLAG_FORCE_CACHED))
			PrecacheSound( emit->name );

		if(pScriptSound->pSamples.numElem() == 0)
		{
			MsgWarning("WARNING! Script sound '%s' is not cached!\n", emit->name);
			return;
		}

		ISoundSample* bestSample;

		if(emit->sampleId > pScriptSound->pSamples.numElem()-1)
			emit->sampleId = -1;

		if(emit->sampleId < 0)
		{
			if(pScriptSound->pSamples.numElem() == 1 )
				bestSample = pScriptSound->pSamples[0];
			else
				bestSample = pScriptSound->pSamples[RandomInt(0,pScriptSound->pSamples.numElem()-1)];
		}
		else
		{
			bestSample = pScriptSound->pSamples[ emit->sampleId ];
		}

		// use the channel defined by the script
		if(channel == -1)
		{
			channel = pScriptSound->channel;
		}

		ISoundPlayable* staticChannel = soundsystem->GetStaticStreamChannel(channel);

		if(staticChannel)
		{
			staticChannel->SetSample(bestSample);
			staticChannel->SetVolume(emit->fVolume*pScriptSound->fVolume);
			staticChannel->SetPitch(emit->fPitch*pScriptSound->fPitch);

			staticChannel->Play();
		}
	}
	else if(emitsound_debug.GetBool())
		MsgError("Emit2DSound: unknown sound '%s'\n", emit->name);
}

#ifndef NO_ENGINE
ConVar snd_occlusion_debug("snd_occlusion_debug", "0");
#endif // NO_ENGINE

bool CSoundEmitterSystem::UpdateEmitter( EmitterData_t* emitter, soundParams_t &params, bool bForceNoInterp )
{
	ASSERT(soundsystem->IsValidEmitter( emitter->pEmitter ));

	Vector3D emitPos = emitter->origin;

#ifndef NO_ENGINE
	if(emitter->pObject)
	{
		BaseEntity* pEnt = (BaseEntity*)emitter->pObject;

		if(pEnt != g_pWorldInfo)
			emitPos += pEnt->GetAbsOrigin();

		if(pEnt->GetState() == BaseEntity::ENTITY_REMOVE)
		{
			emitter->origin += pEnt->GetAbsOrigin();
			emitter->pObject = NULL;
		}
	}
#endif // NO_ENGINE

	emitter->pEmitter->SetPosition(emitPos);
	emitter->pEmitter->SetVelocity(emitter->velocity);

	if(m_bViewIsAvailable)
	{
		// Sound occlusion tests are here
		float fBestVolume = emitter->origVolume;

#ifndef NO_ENGINE

		// Game engine occlusion stuff

		// check occlusion flag and audible distance
		if(emitter->emitSoundData.nFlags & EMITSOUND_FLAG_OCCLUSION)
		{
			internaltrace_t trace;
			physics->InternalTraceLine(m_vViewPos, emitPos, COLLISION_GROUP_WORLD, &trace);

			if(trace.fraction < 1.0f)
				fBestVolume *= 0.2f;
		}

		if(emitter->emitSoundData.nFlags & EMITSOUND_FLAG_ROOM_OCCLUSION)
		{
			int srooms[2];
			int nSRooms = eqlevel->GetRoomsForPoint( emitPos, srooms );

			if(m_nRooms > 0 && nSRooms > 0)
			{
				if(srooms[0] != m_rooms[0])
				{
					// check the portal for connection
					int nPortal = eqlevel->GetFirstPortalLinkedToRoom( m_rooms[0], srooms[0], m_vViewPos, 2);

					if(nPortal == -1)
					{
						fBestVolume = 0.0f;
					}
					else
					{
						// TODO: Better sound positioning using clipping by portal here

						Vector3D portal_pos = eqlevel->GetPortalPosition(nPortal);
						emitPos = portal_pos;

						emitter->pEmitter->SetPosition( emitter->interpolatedOrigin );

						params.referenceDistance -= length(portal_pos - emitPos);
						fBestVolume = emitter->origVolume*0.05f;
					}
				}
			}

			if(bForceNoInterp)
			{
				emitter->interpolatedOrigin = emitPos;
			}
			else
			{
				emitter->interpolatedOrigin = lerp( emitter->interpolatedOrigin, emitPos, gpGlobals->frametime*2.0f );
			}
		}

		if(bForceNoInterp)
		{
			params.volume = fBestVolume;
		}
		else
		{
			params.volume = lerp(params.volume, fBestVolume, gpGlobals->frametime*2.0f);
		}
#else
		if(emitter->pObject && !emitter->pController)
			fBestVolume *= emitter->pObject->GetSoundVolumeScale();

		params.volume = fBestVolume;
#endif
	}

	if( !emitter->pController && (emitter->pEmitter->GetState() == SOUND_STATE_STOPPED) )
		return false;

	return true;
}

void CSoundEmitterSystem::StopAllSounds()
{
	for(int i = 0; i < m_pCurrentTempEmitters.numElem(); i++)
	{
		m_pCurrentTempEmitters[i]->pEmitter->Pause();
		m_pCurrentTempEmitters[i]->pEmitter->Stop();
	}
}

void CSoundEmitterSystem::Update()
{
	PROFILE_CODE(soundsystem->Update());

	PROFILE_FUNC();

	ISoundPlayable* musicChannel = soundsystem->GetStaticStreamChannel(CHAN_STREAM);

	if(musicChannel)
		musicChannel->SetVolume( snd_musicvolume.GetFloat() );

	// don't update
	if(soundsystem->GetPauseState())
		return;

	m_vViewPos = soundsystem->GetListenerPosition();

#ifndef NO_ENGINE
	m_nRooms = eqlevel->GetRoomsForPoint( m_vViewPos, m_rooms );
	m_bViewIsAvailable = m_nRooms > 0;
#endif

	if(m_pUnreleasedSounds.numElem())
	{
		// play sounds
		for(int i = 0; i < m_pUnreleasedSounds.numElem(); i++)
			EmitSound( &m_pUnreleasedSounds[i] );

		// release
		m_pUnreleasedSounds.clear();
	}

	for(int i = 0; i < m_pCurrentTempEmitters.numElem(); i++)
	{
		EmitterData_t* emitter = m_pCurrentTempEmitters[i];

		bool remove = (!emitter || (emitter && !emitter->pEmitter));

		if( !remove )
		{
			soundParams_t params;
			emitter->pEmitter->GetParams( &params );

			remove = UpdateEmitter(emitter, params) == false;

			emitter->pEmitter->SetParams( &params );
		}

		if( remove )
		{
			if(emitter->pController)
				emitter->pController->StopSound(true);

			soundsystem->FreeEmitter( emitter->pEmitter );

			// delete this emitter
			delete emitter;

			m_pCurrentTempEmitters.fastRemoveIndex(i);
			i--;
		}
	}
}

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
		if(!stricmp( kv.GetRootSection()->keys[i]->name, "include"))
			LoadScriptSoundFile( kv.GetRootSection()->keys[i]->values[0]);
	}

	for(int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* curSec = kv.GetRootSection()->keys[i];

		if(!stricmp(curSec->name, "include"))
			continue;

		char* scrsoundName = xstrdup(curSec->name);

		scriptsounddata_t* pSoundData = new scriptsounddata_t;

		pSoundData->pszName = scrsoundName;

		EqString sname(scrsoundName);
		sname = sname.LowerCase();

		pSoundData->namehash = StringToHash( sname.c_str() );

		pSoundData->fVolume = KV_GetValueFloat( curSec->FindKeyBase("volume"), 0, 1.0f );
		pSoundData->fAtten = KV_GetValueFloat( curSec->FindKeyBase("distance"), 0, m_defaultMaxDistance * 0.35f );
		pSoundData->fPitch = KV_GetValueFloat( curSec->FindKeyBase("pitch"), 0, 1.0f );
		pSoundData->fRolloff = KV_GetValueFloat( curSec->FindKeyBase("rolloff"), 0, 1.0f );
		pSoundData->fMaxDistance = KV_GetValueFloat( curSec->FindKeyBase("maxdistance"), 0, m_defaultMaxDistance );
		pSoundData->fAirAbsorption = KV_GetValueFloat( curSec->FindKeyBase("airabsorption"), 0, 0.0f );
		pSoundData->loop = KV_GetValueBool( curSec->FindKeyBase("loop"), 0, false );
		pSoundData->stopLoop = KV_GetValueBool( curSec->FindKeyBase("stopLoop"), 0, false );
		pSoundData->extraStreaming = KV_GetValueBool( curSec->FindKeyBase("streamed"), 0, false );
		pSoundData->is2d = KV_GetValueBool(curSec->FindKeyBase("is2d"), 0, false);

		kvkeybase_t* pKey = curSec->FindKeyBase("channel");

		if(pKey)
		{
			pSoundData->channel = ChannelFromString((char*)KV_GetValueString(pKey));

			if(pSoundData->channel == CHAN_INVALID)
			{
				Msg("Invalid channel '%s' for sound %s\n", KV_GetValueString(pKey), pSoundData->pszName);
				pSoundData->channel = CHAN_STATIC;
			}
		}
		else
			pSoundData->channel = CHAN_STATIC;

		kvkeybase_t* pRndWaveSec = curSec->FindKeyBase("rndwave", KV_FLAG_SECTION);

		if(pRndWaveSec)
		{
			for(int j = 0; j < pRndWaveSec->keys.numElem(); j++)
			{
				if(stricmp(pRndWaveSec->keys[j]->name, "wave"))
					continue;

				pSoundData->soundFileNames.append(pRndWaveSec->keys[j]->values[0]);
			}
		}
		else
		{
			pKey = curSec->FindKeyBase("wave");

			if(pKey)
				pSoundData->soundFileNames.append(pKey->values[0]);
			else
				MsgError("There is no any wave file for script sound '%s'!\n", pSoundData->pszName);
		}

		m_scriptsoundlist.append(pSoundData);
	}
}
