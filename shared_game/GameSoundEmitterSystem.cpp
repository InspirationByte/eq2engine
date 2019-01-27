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

CSoundEmitterSystem* g_sounds = &s_ses;

void sounds_list(DkList<EqString>& list, const char* query)
{
	s_ses.GetAllSoundNames(list);
}

DECLARE_CMD_VARIANTS(test_scriptsound, "Test the scripted sound", sounds_list, 0)
{
	if(CMD_ARGC > 0)
	{
		g_sounds->PrecacheSound( CMD_ARGV(0).c_str() );

		EmitSound_t ep;
		ep.nFlags = (EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D);
		ep.fPitch = 1.0;
		ep.fRadiusMultiplier = 1.0f;
		ep.fVolume = 1.0f;
		ep.origin = Vector3D(0);
		ep.name = (char*)CMD_ARGV(0).c_str();
		ep.pObject = NULL;

		if(g_sounds->EmitSound( &ep ) == CHAN_INVALID)
			MsgError("Cannot play - not valid sound '%s'\n", CMD_ARGV(0).c_str());
	}
}

ConVar emitsound_debug("scriptsound_debug", "0", NULL, CV_CHEAT);

ConVar snd_effectsvolume("snd_effectsvolume", "0.5", NULL, CV_ARCHIVE);
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

		m_emitParams.pController = this;

		// sound must be aquired
		int chan = g_sounds->EmitSound(&m_emitParams);

		// can't start
		if(chan == CHAN_INVALID)
		{
			MsgError("Can't emit sound '%s', CHAN_INVALID\n", m_emitParams.name);
			return;
		}

		if(m_emitParams.emitterIndex < 0 || m_emitParams.emitterIndex > g_sounds->m_emitters.numElem()-1)
		{
			MsgError("Can't emit sound '%s', no out of emitters\n", m_emitParams.name);
			return;
		}

		// get emitter
		m_emitData = g_sounds->m_emitters[m_emitParams.emitterIndex];

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

void CSoundController::Play()
{
	StartSound(NULL);
}

void CSoundController::Pause()
{
	if(!m_emitData)
		return;

	if(!m_emitData->pEmitter)
		return;

	ASSERT(soundsystem->IsValidEmitter( m_emitData->pEmitter ));

	m_emitData->pEmitter->Pause();
}

void CSoundController::Stop(bool force)
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
	m_volume = fVolume;
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

CSoundChannelObject::~CSoundChannelObject()
{
	g_sounds->InvalidateSoundChannelObject(this);
}

void CSoundChannelObject::EmitSound(const char* name)
{
	EmitSound_t ep;
	ep.pObject = this;
	ep.name = (char*)name;

	int channel = g_sounds->EmitSound(&ep);

	if(channel == CHAN_INVALID)
		return;

	m_numChannelSounds[channel]++;
}

void CSoundChannelObject::EmitSoundWithParams(EmitSound_t *ep)
{
	ep->pObject = this;

	int channel = g_sounds->EmitSound(ep);

	if(channel == CHAN_INVALID)
		return;

	m_numChannelSounds[channel]++;
}

int CSoundChannelObject::GetChannelSoundCount(ESoundChannelType chan)
{
	return m_numChannelSounds[chan];
}

void CSoundChannelObject::DecrementChannelSoundCount(ESoundChannelType chan)
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
	pObject = nullptr;
	pController = nullptr;
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

CSoundEmitterSystem::CSoundEmitterSystem() : m_isInit(false), m_defaultMaxDistance(100.0f), m_isPaused(false), m_viewIsAvailable(false), m_numRooms(0)
{

}

void CSoundEmitterSystem::Init(float maxDistance)
{
	if(m_isInit)
		return;

	m_defaultMaxDistance = maxDistance;

	kvkeybase_t* soundSettings = GetCore()->GetConfig()->FindKeyBase("Sound");

	const char* baseScriptFilePath = soundSettings ? KV_GetValueString(soundSettings->FindKeyBase("EmitterScripts"), 0, NULL) : NULL;

	if(baseScriptFilePath == NULL)
	{
		MsgError("InitEFX: EQCONFIG missing Sound:EmitterScripts !\n");
		return;
	}

	LoadScriptSoundFile(baseScriptFilePath);

	m_isPaused = false;
	m_viewIsAvailable = false;
	m_numRooms = 0;

	m_isInit = true;
}

void CSoundEmitterSystem::Shutdown()
{
	StopAllSounds();

	for(int i = 0; i < m_controllers.numElem(); i++)
	{
		m_controllers[i]->Stop(true);

		delete m_controllers[i];
	}

	m_controllers.clear();

	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		m_emitters[i]->pEmitter->Pause();
		m_emitters[i]->pEmitter->Stop();
		soundsystem->FreeEmitter(m_emitters[i]->pEmitter);
		m_emitters[i]->pEmitter = NULL;

		delete m_emitters[i];
	}

	m_emitters.clear();

	for(int i = 0; i < m_allSounds.numElem(); i++)
	{
		for(int j = 0; j < m_allSounds[i]->pSamples.numElem(); j++)
			soundsystem->ReleaseSample( m_allSounds[i]->pSamples[j] );

		delete [] m_allSounds[i]->pszName;
		delete m_allSounds[i];
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

int CSoundEmitterSystem::GetEmitterIndexByEntityAndChannel(CSoundChannelObject* pEnt, ESoundChannelType chan)
{
	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		if(m_emitters[i]->pObject == pEnt && m_emitters[i]->channel == chan)
			return i;
	}

	return -1;
}

ISoundController* CSoundEmitterSystem::CreateSoundController(EmitSound_t *ep)
{
	ASSERT(ep);
	ASSERT(ep->name);

	soundScriptDesc_t* pScriptSound = FindSound(ep->name);

	// check
	if(!pScriptSound)
	{
		MsgError("CreateSoundController: Couldn't find script sound '%s'!\n", ep->name);
		return NULL;
	}

	CSoundController* pController = new CSoundController();

	m_controllers.append(pController);

	pController->m_soundName = ep->name; // copy sound name since the params can use non-permanent adresses
	pController->m_emitParams = *ep;
	pController->m_emitParams.name = (char*)pController->m_soundName.c_str();

	return pController;
}

void CSoundEmitterSystem::RemoveSoundController(ISoundController* cont)
{
	if(!cont)
		return;

	cont->Stop();

	delete cont;

	int idx = m_controllers.findIndex(cont);

	if( idx != -1)
		m_controllers.fastRemoveIndex( idx );
}

void CSoundEmitterSystem::InvalidateSoundChannelObject(CSoundChannelObject* pEnt)
{
	for (int i = 0; i < m_emitters.numElem(); i++)
	{
		if (m_emitters[i]->pObject == pEnt)
			m_emitters[i]->pObject = nullptr;
	}
}

void CSoundEmitterSystem::PrecacheSound(const char* pszName)
{
	// find the present sound file
	soundScriptDesc_t* pSound = FindSound(pszName);

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

void CSoundEmitterSystem::GetAllSoundNames(DkList<EqString>& soundNames) const
{
	for (int i = 0; i < m_allSounds.numElem(); i++)
	{
		soundNames.append(m_allSounds[i]->pszName);
	}
}

soundScriptDesc_t* CSoundEmitterSystem::FindSound(const char* soundName) const
{
	EqString sname(soundName);
	sname = sname.LowerCase();

	int snameHash = StringToHash( sname.c_str(), true );

	for(int i = 0; i < m_allSounds.numElem(); i++)
	{
		if(m_allSounds[i]->namehash == snameHash)
			return m_allSounds[i];
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

	if((emit->nFlags & EMITSOUND_FLAG_START_ON_UPDATE) || m_isPaused)
	{
		EmitSound_t newEmit = (*emit);
		newEmit.nFlags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		newEmit.name = xstrdup(newEmit.name);

		m_pendingStartSounds.append( newEmit );
		return CHAN_INVALID;
	}


	// quickly set
	emit->emitterIndex = -1;

#ifndef NO_ENGINE
	// don't start sound if game isn't started
	if(gpGlobals->curtime <= 0)
		return CHAN_INVALID;

#endif // NO_ENGINE

	soundScriptDesc_t* pScriptSound = FindSound(emit->name);

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
			if(pChanObj->GetChannelSoundCount(pScriptSound->channel) >= s_soundChannelMaxEmitters[pScriptSound->channel])
			{
				// find currently playing sound index
				int firstPlayingSound = GetEmitterIndexByEntityAndChannel( pChanObj, pScriptSound->channel );
				if(firstPlayingSound != -1)
				{
					EmitterData_t* substEmitter = m_emitters[firstPlayingSound];

					// if index is valid, shut up this sound
					if(substEmitter->pController)
						substEmitter->pController->Stop();
					else
						substEmitter->pEmitter->Stop();

					// and remove
					soundsystem->FreeEmitter( substEmitter->pEmitter );
					substEmitter->pEmitter = NULL;
					delete substEmitter;
					m_emitters.fastRemoveIndex(firstPlayingSound);

					// pChanObj sound count
					pChanObj->DecrementChannelSoundCount(pScriptSound->channel);
				}
			}
		}

		ISoundSample* bestSample = NULL;

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

		// alloc SoundEmitterSystem emitter data
		EmitterData_t* edata = new EmitterData_t();

		ISoundEmitter* sndEmitter = soundsystem->AllocEmitter();;

		edata->pEmitter = sndEmitter;
		edata->pObject = emit->pObject;

		edata->origin = emit->origin;
		edata->velocity = vec3_zero;		// only accessible by sound controller
		edata->interpolatedOrigin = emit->origin;
		edata->origVolume = pScriptSound->fVolume * emit->fVolume;
		edata->channel = pScriptSound->channel;
		edata->script = pScriptSound;

		edata->pController = emit->pController;

		edata->emitSoundData = *emit;

		// make new sound parameters
		soundParams_t sParams;

		sParams.maxDistance			= pScriptSound->fMaxDistance;
		sParams.referenceDistance	= pScriptSound->fAtten * emit->fRadiusMultiplier;
		sParams.pitch				= emit->fPitch * pScriptSound->fPitch;
		sParams.volume				= edata->origVolume;
		sParams.rolloff				= pScriptSound->fRolloff;
		sParams.airAbsorption		= pScriptSound->fAirAbsorption;
		sParams.is2D				= pScriptSound->is2d;

		// update emitter before playing
		UpdateEmitter( edata, sParams, true );

		if (emit->nFlags & EMITSOUND_FLAG_STARTSILENT)
			sParams.volume = 0.0f;

		if(emitsound_debug.GetBool())
		{
			float boxsize = sParams.referenceDistance;

			debugoverlay->Box3D(edata->origin-boxsize, edata->origin+boxsize, ColorRGBA(1,1,1,0.5f), 1.0f);
			MsgInfo("started sound '%s' ref=%g max=%g\n", pScriptSound->pszName, sParams.referenceDistance, sParams.maxDistance);
		}

#pragma todo("randomization flag for sounding pitch")
		//if(!pScriptSound->loop)
		//	sParams.pitch += RandomFloat(-0.05f,0.05f);

		sndEmitter->SetSample(bestSample);
		sndEmitter->SetParams(&sParams);

		sndEmitter->Play();

		int eIndex = m_emitters.append(edata);
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

	if ((emit->nFlags & EMITSOUND_FLAG_START_ON_UPDATE) || m_isPaused)
	{
		EmitSound_t newEmit = (*emit);
		newEmit.nFlags &= ~EMITSOUND_FLAG_START_ON_UPDATE;
		newEmit.emitterIndex = channel;	// temporary
		newEmit.name = xstrdup(newEmit.name);

		m_pendingStartSounds2D.append(newEmit);
		return;
	}

	// quickly set
	emit->emitterIndex = -1;

#ifndef NO_ENGINE
	// don't start sound if game isn't started
	if (gpGlobals->curtime <= 0)
		return;

#endif // NO_ENGINE

	soundScriptDesc_t* pScriptSound = FindSound(emit->name);

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

		int numSamples = pScriptSound->pSamples.numElem();

		if (numSamples > 0)
		{
			if (emit->sampleId > numSamples - 1)
				emit->sampleId = -1;

			if (emit->sampleId < 0)
				bestSample = pScriptSound->pSamples[RandomInt(0, numSamples - 1)];
			else
				bestSample = pScriptSound->pSamples[emit->sampleId];
		}
		else
			bestSample = pScriptSound->pSamples[0];

		// use the channel defined by the script
		if(channel == -1)
		{
			channel = pScriptSound->channel;
		}

		ISoundPlayable* staticChannel = soundsystem->GetStaticStreamChannel(channel);

		if(staticChannel)
		{
			float volume = emit->fVolume*pScriptSound->fVolume;

			if (channel == CHAN_STREAM)
				volume = snd_musicvolume.GetFloat();

			staticChannel->SetSample(bestSample);
			staticChannel->SetVolume(volume);
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

	CSoundController* controller = (CSoundController*)emitter->pController;

	Vector3D emitPos = emitter->origin;

	float fBestVolume = emitter->origVolume;

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

	// Sound occlusion tested here
	if(m_viewIsAvailable)
	{
		const Vector3D& viewPos = soundsystem->GetListenerPosition();

		// check occlusion flag and audible distance
		if(emitter->emitSoundData.nFlags & EMITSOUND_FLAG_OCCLUSION)
		{
			internaltrace_t trace;
			physics->InternalTraceLine(viewPos, emitPos, COLLISION_GROUP_WORLD, &trace);

			if (trace.fraction < 1.0f)
				fBestVolume *= 0.2f;
		}

		// do room lookup
		if(emitter->emitSoundData.nFlags & EMITSOUND_FLAG_ROOM_OCCLUSION)
		{
			int srooms[2];
			int nSRooms = eqlevel->GetRoomsForPoint( emitPos, srooms );

			if(m_numRooms > 0 && nSRooms > 0)
			{
				if(srooms[0] != m_rooms[0]) 
				{
					// check the portal for connection
					// TODO: recursive!
					int nPortal = eqlevel->GetFirstPortalLinkedToRoom( m_rooms[0], srooms[0], viewPos, 2);

					if(nPortal == -1)
					{
						fBestVolume = 0.0f;
					}
					else
					{
						// TODO: Better sound positioning using clipping by portal here

						Vector3D portal_pos = eqlevel->GetPortalPosition(nPortal);
						emitPos = portal_pos;

						// TODO: volume scaling according to the distance from one portal to another
						emitter->pEmitter->SetPosition( emitter->interpolatedOrigin );

						params.referenceDistance -= length(portal_pos - emitPos);
						fBestVolume = emitter->origVolume*0.25f;
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

		/*
		if(!bForceNoInterp)
			emitter->interpVolume = fBestVolume;
		else
			emitter->interpVolume = lerp(emitter->interpVolume, fBestVolume, gpGlobals->frametime*2.0f);
		*/
	}
#endif

	emitter->pEmitter->SetPosition(emitPos);
	emitter->pEmitter->SetVelocity(emitter->velocity);
	
	if (controller)
	{
		fBestVolume *= controller->m_volume;
	}
	
	if(emitter->pObject)
		fBestVolume *= emitter->pObject->GetSoundVolumeScale();
	
	params.volume = fBestVolume * snd_effectsvolume.GetFloat();

	if (!controller && (emitter->pEmitter->GetState() == SOUND_STATE_STOPPED))
		return false;

	return true;
}

void CSoundEmitterSystem::StopAllSounds()
{
	StopAll2DSounds();
	StopAllEmitters();
}

void CSoundEmitterSystem::StopAllEmitters()
{
	// remove pending sounds
	for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
	{
		EmitSound_t& snd = m_pendingStartSounds[i];
		delete[] snd.name;
	}

	m_pendingStartSounds.clear();

	// stop emitters
	for (int i = 0; i < m_emitters.numElem(); i++)
	{
		EmitterData_t* em = m_emitters[i];

		if (em->pController)
			em->pController->Stop(true);

		if (em->pEmitter)
		{
			em->pEmitter->Stop();

			soundsystem->FreeEmitter(em->pEmitter);
			em->pEmitter = nullptr;
		}

		delete em;
		m_emitters.fastRemoveIndex(i);
		i--;
	}

	soundsystem->Update();
}

void CSoundEmitterSystem::StopAll2DSounds()
{
	// remove pending sounds
	for (int i = 0; i < m_pendingStartSounds2D.numElem(); i++)
	{
		EmitSound_t& snd = m_pendingStartSounds2D[i];
		delete[] snd.name;
	}

	m_pendingStartSounds2D.clear();

	// stop static channels
	int i = 0;
	while(true)
	{
		ISoundPlayable* staticChannel = soundsystem->GetStaticStreamChannel(i);

		i++;
		if(!staticChannel)
			break;

		staticChannel->Stop();
		staticChannel->SetSample(NULL);
	}
}

void CSoundEmitterSystem::Update(bool force)
{
	PROFILE_CODE(soundsystem->Update());

	PROFILE_FUNC();

	ISoundPlayable* musicChannel = soundsystem->GetStaticStreamChannel(CHAN_STREAM);

	if(musicChannel)
		musicChannel->SetVolume( snd_musicvolume.GetFloat() );

	// don't update
	if(!force && soundsystem->GetPauseState())
		return;

	const Vector3D& viewPos = soundsystem->GetListenerPosition();

#ifndef NO_ENGINE
	m_numRooms = eqlevel->GetRoomsForPoint(viewPos, m_rooms );
	m_viewIsAvailable = m_numRooms > 0;
#endif
	if (!m_isPaused)
	{
		if (m_pendingStartSounds2D.numElem())
		{
			// play sounds
			for (int i = 0; i < m_pendingStartSounds2D.numElem(); i++)
			{
				EmitSound_t& snd = m_pendingStartSounds2D[i];
				Emit2DSound(&snd, snd.emitterIndex);

				delete [] snd.name;
			}

			// release
			m_pendingStartSounds2D.clear();
		}

		if (m_pendingStartSounds.numElem())
		{
			// play sounds
			for (int i = 0; i < m_pendingStartSounds.numElem(); i++)
			{
				EmitSound_t& snd = m_pendingStartSounds[i];
				EmitSound(&snd);

				delete[] snd.name;
			}

			// release
			m_pendingStartSounds.clear();
		}
	}

	for(int i = 0; i < m_emitters.numElem(); i++)
	{
		EmitterData_t* emitter = m_emitters[i];

		bool remove = (!emitter || (emitter && !emitter->pEmitter));

		ISoundEmitter* em = emitter->pEmitter;

		if( !remove )
		{
			soundParams_t params;
			em->GetParams( &params );

			remove = UpdateEmitter(emitter, params) == false;

			em->SetParams( &params );
		}

		if( remove )
		{
			if(emitter->pController)
				emitter->pController->Stop(true);

			soundsystem->FreeEmitter(em);

			// delete this emitter
			delete emitter;

			m_emitters.fastRemoveIndex(i);
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
		kvkeybase_t* kb = kv.GetRootSection()->keys[i];

		if(!stricmp( kb->GetName(), "include"))
			LoadScriptSoundFile( KV_GetValueString(kb) );
	}

	for(int i = 0; i < kv.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* curSec = kv.GetRootSection()->keys[i];

		if(!stricmp(curSec->name, "include"))
			continue;

		soundScriptDesc_t* newSound = new soundScriptDesc_t;

		newSound->pszName = xstrdup(curSec->name);
		newSound->namehash = StringToHash(newSound->pszName, true );

		newSound->fVolume = KV_GetValueFloat( curSec->FindKeyBase("volume"), 0, 1.0f );

		newSound->fAtten = KV_GetValueFloat( curSec->FindKeyBase("distance"), 0, m_defaultMaxDistance * 0.35f );
		newSound->fMaxDistance = KV_GetValueFloat(curSec->FindKeyBase("maxdistance"), 0, m_defaultMaxDistance);

		newSound->fPitch = KV_GetValueFloat( curSec->FindKeyBase("pitch"), 0, 1.0f );
		newSound->fRolloff = KV_GetValueFloat( curSec->FindKeyBase("rolloff"), 0, 1.0f );
		newSound->fAirAbsorption = KV_GetValueFloat( curSec->FindKeyBase("airabsorption"), 0, 0.0f );

		newSound->loop = KV_GetValueBool( curSec->FindKeyBase("loop"), 0, false );
		newSound->stopLoop = KV_GetValueBool( curSec->FindKeyBase("stopLoop"), 0, false );

		newSound->extraStreaming = KV_GetValueBool( curSec->FindKeyBase("streamed"), 0, false );
		newSound->is2d = KV_GetValueBool(curSec->FindKeyBase("is2d"), 0, false);

		kvkeybase_t* pKey = curSec->FindKeyBase("channel");

		if(pKey)
		{
			newSound->channel = ChannelTypeByName(KV_GetValueString(pKey));

			if(newSound->channel == CHAN_INVALID)
			{
				Msg("Invalid channel '%s' for sound %s\n", KV_GetValueString(pKey), newSound->pszName);
				newSound->channel = CHAN_STATIC;
			}
		}
		else
			newSound->channel = CHAN_STATIC;

		// pick 'rndwave' or 'wave' sections for lists
		pKey = curSec->FindKeyBase("rndwave", KV_FLAG_SECTION);

		if(!pKey)
			pKey = curSec->FindKeyBase("wave", KV_FLAG_SECTION);

		if(pKey)
		{
			for(int j = 0; j < pKey->keys.numElem(); j++)
			{
				kvkeybase_t* ent = pKey->keys[j];

				if(stricmp(ent->name, "wave"))
					continue;

				newSound->soundFileNames.append(KV_GetValueString(ent));
			}
		}
		else
		{
			pKey = curSec->FindKeyBase("wave");

			if(pKey)
				newSound->soundFileNames.append(KV_GetValueString(pKey));
		}

		if(newSound->soundFileNames.numElem() == 0)
			MsgWarning("empty sound script '%s'!\n", newSound->pszName);

		m_allSounds.append(newSound);
	}
}
