//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2018
//////////////////////////////////////////////////////////////////////////////////
// Description: Game session base implementations
//////////////////////////////////////////////////////////////////////////////////

#include "luabinding/LuaBinding.h"

#include "session_base.h"
#include "replay.h"
#include "eqParallelJobs.h"
#include "DrvSynHUD.h"

#include "Shiny.h"	// profiler

#include "world.h"
#include "input.h"

#include "pedestrian.h"

extern ConVar g_autoHandbrake;
extern ConVar g_invicibility;
extern ConVar g_infiniteMass;
extern ConVar g_difficulty;

CGameSessionBase*	g_pGameSession = NULL;

bool		g_bIsServer = true;

// game definitions
bool IsClient()
{
	return !g_bIsServer;
}

bool IsServer()
{
	return g_bIsServer;
}

//---------------------------------------------------------------------------------------------

CGameSessionBase::CGameSessionBase()
{
	m_waitForExitTime = 0.0f;
	m_missionStatus = MIS_STATUS_INGAME;

	m_leadCar = NULL;
	m_viewObject = NULL;

	m_scriptIDCounter = 0;
}

CGameSessionBase::~CGameSessionBase()
{

}

int CGameSessionBase::GetPhysicsIterations() const
{
//	if(g_replayTracker->m_state == REPL_PLAYING)
//		return g_replayTracker->m_playbackPhysIterations;

	return 2;
}

void CGameSessionBase::Init()
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	// init mission manager pointers
	if (!OOLUA::get_global(state, "missionmanager", m_missionManagerTable))
	{
		MsgError("CGameSessionBase::Init Error - cannot get missionmanager Lua table!\n");
	}

	if (!m_lua_misman_InitMission.Get(m_missionManagerTable, "InitMission"))
	{
		MsgError("CGameSessionBase::Init Error - cannot get missionmanager's InitMission() function!\n");
	}

	if (!m_lua_misman_GetRandomSeed.Get(m_missionManagerTable, "GetRandomSeed"))
	{
		MsgError("CGameSessionBase::Init Error - cannot get missionmanager's GetRandomSeed() function!\n");
	}

	if (!m_lua_misman_Update.Get(m_missionManagerTable, "Update"))
	{
		MsgError("CGameSessionBase::Init Error - cannot get missionmanager's Update() function!\n");
	}

	if (!m_lua_misman_Finalize.Get(m_missionManagerTable, "Finalize"))
	{
		MsgError("CGameSessionBase::Init Error - cannot get missionmanager's Finalize() function!\n");
	}

	g.Release();

	//-------------------------------------------------------------------

	m_scriptIDCounter = 0;

	m_gameTime = 0.0;

	// load cars
	LoadCarsPedsRegistry();

	// init AI manager and put car registry into it
	g_pAIManager->Init(m_pedEntries);

	// start recorder
	if (g_replayTracker->m_state != REPL_INIT_PLAYBACK)
		g_replayTracker->StartRecording();

	if (g_replayTracker->m_state == REPL_INIT_PLAYBACK) // make scripted cars work first
		g_replayTracker->StartPlay();

	// just before the scripted call occurs, we fade in by default
	g_pGameHUD->FadeIn(true);

	//
	// MISSION SCRIPT INITIALIZER CALL
	// also will spawn mission objects, if not in replay.
	//
	{
		// bind function and call, table pushed automatically
		m_lua_misman_InitMission.Push();

		if (!m_lua_misman_InitMission.Call(0, 0))
			MsgError("CGameSessionBase::Init, :CMissionManager_InitMission() error:\n %s\n", OOLUA::get_last_error(state).c_str());
	}

	{
		m_lua_misman_GetRandomSeed.Push();
		if (!m_lua_misman_GetRandomSeed.Call(0, 1))
		{
			MsgError("CGameSessionBase::GetRandomSeed, :CMissionManager_InitMission() error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}

		int seedValue = 0;
		OOLUA::pull(GetLuaState(), seedValue);

		g_replayRandom.SetSeed(seedValue);
		g_replayTracker->PushEvent(REPLAY_EVENT_RANDOM_SEED, -1, (void*)seedValue);
	}

	m_missionStatus = MIS_STATUS_INGAME;
}

void CGameSessionBase::OnLoadingDone()
{
	if (IsServer())
	{
		// spawn cars around
		CCar* leadCar = GetLeadCar();

		if (leadCar)
			g_pAIManager->InitialSpawns(leadCar->GetOrigin());
	}
}

extern ConVar sys_maxfps;

float CGameSessionBase::LoadCarReplay(CCar* pCar, const char* filename)
{
	int numTicks = 0;
	g_replayTracker->LoadVehicleReplay(pCar, filename, numTicks);

	float fixedTicksDelta = 1.0f / (sys_maxfps.GetFloat()*GetPhysicsIterations());

	return numTicks * fixedTicksDelta;
}

void CGameSessionBase::StopCarReplay(CCar* pCar)
{
	g_replayTracker->StopVehicleReplay(pCar);
}

void CGameSessionBase::FinalizeMissionManager()
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	m_lua_misman_Finalize.Push();
	if (!m_lua_misman_Finalize.Call(0, 0))
	{
		MsgError("CGameSessionBase::Init, :CMissionManager_Finalize() error:\n %s\n", OOLUA::get_last_error(state).c_str());
	}
}

void CGameSessionBase::ClearCarsPedsRegistry()
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
		delete m_carEntries[i];

	m_carEntries.clear();

	for (int i = 0; i < m_pedEntries.numElem(); i++)
		delete m_pedEntries[i];

	m_pedEntries.clear();
}

void CGameSessionBase::Shutdown()
{
	g_pAIManager->Shutdown();

	m_leadCar = NULL;

	ClearCarsPedsRegistry();
}

bool CGameSessionBase::IsGameDone(bool checkTime /*= true*/) const
{
	if (checkTime)
		return (m_missionStatus > MIS_STATUS_INGAME) && m_waitForExitTime <= 0.0f;
	else
		return (m_missionStatus > MIS_STATUS_INGAME);
}

void CGameSessionBase::SignalMissionStatus(int missionStatus, float waitForExitTime)
{
	m_waitForExitTime = waitForExitTime;
	m_missionStatus = missionStatus;
}

int	CGameSessionBase::GetMissionStatus() const
{
	return m_missionStatus;
}

bool CGameSessionBase::IsReplay() const
{
	return (g_replayTracker->m_state == REPL_PLAYING ||
		g_replayTracker->m_state == REPL_INIT_PLAYBACK);
}

int	CGameSessionBase::GenScriptID()
{
	return m_scriptIDCounter++;
}

CGameObject* CGameSessionBase::FindScriptObjectById(int scriptID)
{
	const DkList<CGameObject*>& objList = g_pGameWorld->m_gameObjects;

	for (int i = 0; i < objList.numElem(); i++)
	{
		if (objList[i]->GetScriptID() == scriptID)
			return objList[i];
	}

	return NULL;
}

void Game_OnPhysicsUpdate(float fDt, int iterNum)
{
	PROFILE_FUNC();

	if (fDt <= 0.0f)
		return;

	if (g_replayTracker)
	{
		// always regenerate predictable random
		g_replayRandom.SetSeed(g_replayTracker->m_tick);
		g_replayRandom.Regenerate();
	}

	// next is only calculated at the server
	if (IsServer())
	{
		CCar* leadCar = g_pGameSession->GetLeadCar();

		// update traffic lights
		g_pGameWorld->UpdateTrafficLightState(fDt);

		// update traffic car spawn/remove from here
		if (iterNum == 0)
		{
			if (leadCar)
			{
				CCar* plrCar = g_pGameSession->GetPlayerCar();

				Vector3D spawnPos, removePos;

				spawnPos = removePos = leadCar->GetOrigin();

				//if (plrCar)
				//	removePos = plrCar->GetOrigin();

				// vehicle respawn is oriented on the lead car, but removal is always depends on player
				g_pAIManager->UpdateCarRespawn(fDt, spawnPos, removePos, leadCar->GetVelocity());

				// pedestrians respawned too
				g_pAIManager->UpdatePedestrianRespawn(fDt, spawnPos, removePos, leadCar->GetVelocity());
			}
		}

		g_pAIManager->UpdateNavigationVelocityMap(fDt);
	}

	// do replays and prerecorded stuff
	if (g_replayTracker)
		g_replayTracker->UpdatePlayback(fDt);
}

void GameJob_UpdatePhysics(void* data)
{
	float fDt = *(float*)data;

	float phys_begin = MEASURE_TIME_BEGIN();
	g_pPhysics->Simulate(fDt, g_pGameSession->GetPhysicsIterations(), Game_OnPhysicsUpdate);
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "physics time, ms: %g", abs(MEASURE_TIME_STATS(phys_begin)));
}

void CGameSessionBase::UpdateAsPlayerCar(const playerControl_t& control, CCar* car)
{
	if(!g_pGameWorld->IsValidObject(car))
		return;

	g_pGameWorld->QueryNearestRegions(car->GetOrigin(), false);

	if (!g_replayTracker->IsCarPlaying(car))
	{
		// apply cheats
		{
			if (g_invicibility.GetBool())
				car->SetDamage(0.0f);

			car->SetInfiniteMass(g_infiniteMass.GetBool());
		}

		car->SetAutoHandbrake(g_autoHandbrake.GetBool());
		car->SetAutoGearSwitch(true);

		car->SetControlButtons(control.buttons);

		car->SetControlVars(
			(control.buttons & IN_ACCELERATE) ? control.accelBrakeValue : 0.0f,
			(control.buttons & IN_BRAKE) ? control.accelBrakeValue : 0.0f,
			control.steeringValue);

		car->UpdateLightsState();
	}
}

//
// Updates the game
//
void CGameSessionBase::Update(float fDt)
{
	PROFILE_FUNC();

	if (m_missionStatus != MIS_STATUS_INGAME)
	{
		m_waitForExitTime -= fDt;

		m_waitForExitTime = max(0.0f, m_waitForExitTime);
	}

	if (fDt <= 0.0f)
	{
		if (GetLeadCar())
			g_pGameWorld->QueryNearestRegions(GetLeadCar()->GetOrigin(), false);

		g_pGameWorld->UpdateWorld(0.0f);
		return;
	}

	m_gameTime += fDt;

	UpdatePlayerControls();

	float phys_begin = MEASURE_TIME_BEGIN();
	g_pPhysics->Simulate(fDt, GetPhysicsIterations(), Game_OnPhysicsUpdate);

	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "physics time, ms: %g", abs(MEASURE_TIME_STATS(phys_begin)));

	g_pAIManager->UpdateCopStuff(fDt);

	// updates world
	g_pGameWorld->UpdateWorld(fDt);

	UpdateMission(fDt);
}

void CGameSessionBase::UpdateMission(float fDt)
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	// bind function and call, table pushed automatically
	m_lua_misman_Update.Push();

	OOLUA::push(state, fDt);

	if (!m_lua_misman_Update.Call(1, 0))
	{
		Msg("CGameSessionBase::Update, :CMissionManager_Update() error:\n %s\n", OOLUA::get_last_error(state).c_str());
	}
}

CCar* CGameSessionBase::CreateCar(const char* name, int carType)
{
	vehicleConfig_t* config = GetVehicleConfig(name);

	if (!config)
	{
		MsgError("Can't create car '%s'\n", name);
		return nullptr;
	}

	CCar* car = nullptr;

	if (carType == CAR_TYPE_NORMAL)
	{
		car = new CCar(config);
	}
	else if (carType == CAR_TYPE_TRAFFIC_AI)
	{
		CAITrafficCar* traffic = new CAITrafficCar(config);
		car = traffic;
	}
	else if (carType == CAR_TYPE_PURSUER_COP_AI)
	{
		CAIPursuerCar* traffic = new CAIPursuerCar(config, PURSUER_TYPE_COP);
		car = traffic;
	}
	else if (carType == CAR_TYPE_PURSUER_GANG_AI)
	{
		CAIPursuerCar* traffic = new CAIPursuerCar(config, PURSUER_TYPE_GANG);
		car = traffic;
	}

	if (!car)
		MsgError("Unknown car type '%d'\n", carType);

	return car;
}

CAIPursuerCar* CGameSessionBase::CreatePursuerCar(const char* name, int type)
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		vehicleConfig_t* config = m_carEntries[i];

		if (!config->carName.CompareCaseIns(name))
		{
			CAIPursuerCar* car = new CAIPursuerCar(config, (EPursuerAIType)type);
			car->InitAI(false);

			// manage this car
			g_pAIManager->m_trafficCars.append(car);

			return car;
		}
	}

	MsgError("Can't create pursuer car '%s'\n", name);

	return NULL;
}

CCar* CGameSessionBase::Lua_CreateCar(const char* name, int type)
{
	if (g_replayTracker && g_replayTracker->m_state == REPL_PLAYING)
	{
		CCar* scriptCar = g_replayTracker->GetCarByScriptId(m_scriptIDCounter);
		
		if (scriptCar)
		{
			GenScriptID();
			return scriptCar;
		}

		ASSERTMSG(false, varargs("Lua_CreateCar - no valid script car by replay (id=%d, tick=%d)", m_scriptIDCounter, g_replayTracker->m_tick));
	}
	else
		return CreateCar(name, type);

	return NULL;
}

CAIPursuerCar* CGameSessionBase::Lua_CreatePursuerCar(const char* name, int type)
{
	if (g_replayTracker && g_replayTracker->m_state == REPL_PLAYING)
	{
		CCar* scriptCar = g_replayTracker->GetCarByScriptId(m_scriptIDCounter);

		if (scriptCar)
		{
			GenScriptID();
			return (CAIPursuerCar*)scriptCar;
		}
			
		ASSERTMSG(false, varargs("Lua_CreatePursuerCar - no valid script car by replay (id=%d, tick=%d)", m_scriptIDCounter, g_replayTracker->m_tick));
	}
	else
		return CreatePursuerCar(name, type);

	return NULL;
}

int CGameSessionBase::RegisterVehicleConfig(const char* name, kvkeybase_t* params)
{
	// reload car config
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		vehicleConfig_t* conf = m_carEntries[i];
		if (conf->carName == name)
		{
			conf->DestroyCleanup();
			return ParseVehicleConfig(conf, params);
		}
	}

	vehicleConfig_t* conf = new vehicleConfig_t();
	conf->carName = name;
	conf->carScript = name;

	conf->scriptCRC = 0;

	if (!ParseVehicleConfig(conf, params))
	{
		delete conf;
		return -1;
	}

	return m_carEntries.append(conf);
}

vehicleConfig_t* CGameSessionBase::GetVehicleConfig(const char* name)
{
	// find existing config
	vehicleConfig_t* exConfig = FindVehicleConfig(name);

	if (exConfig)
		return exConfig;

	// try load new one
	EqString configPath = CombinePath(2, "scripts/vehicles", _Es(name)+".txt");

	kvkeybase_t kvb;
	if (!KV_LoadFromFile(configPath.c_str(), SP_MOD, &kvb))
	{
		MsgError("can't load car script '%s'\n", name);
		return nullptr;
	}

	// register it!
	int configIdx = RegisterVehicleConfig(name, &kvb);
	if (configIdx == -1)
		return nullptr;

	return m_carEntries[configIdx];
}

void CGameSessionBase::LoadCarsPedsRegistry()
{
	// delete old entries
	ClearCarsPedsRegistry();

	PrecacheObject(CCar);
	PrecacheObject(CPedestrian);

	// initialize vehicle list
	KeyValues kvs;

	// initialize pedestrian list
	if (kvs.LoadFromFile("scripts/pedestrians.def"))
	{
		kvkeybase_t* pedRegistry = kvs.GetRootSection();

		for (int i = 0; i < pedRegistry->keys.numElem(); i++)
		{
			kvkeybase_t* key = pedRegistry->keys[i];
			if (!key->IsSection() && !key->IsDefinition())
			{
				pedestrianConfig_t* conf = new pedestrianConfig_t();
				conf->name = key->name;
				conf->model = KV_GetValueString(key);
				conf->hasAI = KV_GetValueBool(key, 1, true);

				if (g_fileSystem->FileExist(conf->model.c_str(), SP_MOD))
				{
					// FIXME: it's bad to precache models here
					PrecacheStudioModel(conf->model.c_str());

					m_pedEntries.append(conf);
				}
				else
					delete conf;
			}
		}
	}
	else
		CrashMsg("FATAL: no scripts/pedestrians.def file!");
}

void CGameSessionBase::GetCarNames(DkList<EqString>& list) const
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		if (m_carEntries[i]->flags.isCar)
			list.append(m_carEntries[i]->carName);
	}
}

vehicleConfig_t* CGameSessionBase::FindVehicleConfig(const char* name) const
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		if (!m_carEntries[i]->carName.CompareCaseIns(name))
			return m_carEntries[i];
	}

	return nullptr;
}

extern ConVar g_pause;

CGameObject* CGameSessionBase::GetViewObject() const
{
	if (g_replayTracker->m_state == REPL_PLAYING && !g_pause.GetBool())
	{
		replayCamera_t* cam = g_replayTracker->GetCurrentCamera();

		if (cam)
			return g_replayTracker->GetCarByReplayIndex(cam->targetIdx);
	}

	if (m_viewObject)
		return m_viewObject;

	if (m_viewObject == nullptr)
		return GetPlayerCar();

	return nullptr;
}

void CGameSessionBase::SetViewObject(CGameObject* viewObj)
{
	if (m_viewObject == GetPlayerCar())
		m_viewObject = nullptr;
	else
		m_viewObject = viewObj;
}

void CGameSessionBase::SetViewObjectToNone()
{
	SetViewObject(nullptr);
}

CCar* CGameSessionBase::GetLeadCar() const
{
	if (m_leadCar)
		return m_leadCar;

	return GetPlayerCar();
}

void CGameSessionBase::SetLeadCar(CCar* pCar)
{
	m_leadCar = pCar;
}

void CGameSessionBase::SetLeadCarNone()
{
	SetLeadCar(nullptr);
}

OOLUA_EXPORT_FUNCTIONS(
	CGameSessionBase,

	CreateCar,
	CreatePursuerCar, 
	RegisterVehicleConfig,
	SetPlayerCar,
	SetLeadCar,
	SetLeadCarNone,
	SetViewObject,
	SetViewObjectToNone,
	LoadCarReplay,
	StopCarReplay,
	SignalMissionStatus
)
OOLUA_EXPORT_FUNCTIONS_CONST(
	CGameSessionBase,

	IsClient,
	IsServer,
	GetPlayerCar,
	GetLeadCar,
	GetViewObject,
	GetSessionType,
	IsGameDone,
	GetMissionStatus,
	IsReplay
)
