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

CGameSessionBase*	g_pGameSession = NULL;

void fng_car_variants(DkList<EqString>& list, const char* query)
{
	if (g_pGameSession)
		g_pGameSession->GetCarNames(list);
}

ConVar		g_car("g_car", "rollo", fng_car_variants, "player car", CV_ARCHIVE);

ConVar		g_autoHandbrake("g_autoHandbrake", "1", "Auto handbrake for steering help", CV_ARCHIVE);

ConVar		g_invicibility("g_invicibility", "0", "No damage for player car", CV_CHEAT);
ConVar		g_infiniteMass("g_infiniteMass", "0", "Infinite mass for player car", CV_CHEAT);

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
	m_viewCar = NULL;

	m_scriptIDCounter = 0;
	m_scriptIDReplayCounter = 0;
}

CGameSessionBase::~CGameSessionBase()
{

}

int CGameSessionBase::GetPhysicsIterations() const
{
//	if(g_replayData->m_state == REPL_PLAYING)
//		return g_replayData->m_playbackPhysIterations;

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
	m_scriptIDReplayCounter = 0;

	// load cars
	LoadCarData();

	g_pAIManager->Init();

	g_replayRandom.SetSeed(0);

	// start recorder
	if (g_replayData->m_state != REPL_INIT_PLAYBACK)
		g_replayData->StartRecording();

	if (g_replayData->m_state == REPL_INIT_PLAYBACK) // make scripted cars work first
		g_replayData->StartPlay();

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

	// start recorder
	if (g_replayData->m_state <= REPL_RECORDING)
	{
		//
		// Spawn default car if script not did
		//
		if (!GetPlayerCar())
		{
			// make a player car
			CCar* playerCar = CreateCar(g_car.GetString());

			if (playerCar)
			{
				playerCar->SetOrigin(Vector3D(0, 0.5, 10));
				playerCar->SetAngles(Vector3D(0, 0, 0));
				playerCar->Spawn();

				g_pGameWorld->AddObject(playerCar);

				SetPlayerCar(playerCar);
			}
		}
	}

	// load regions on player car position
	if (GetPlayerCar())
		g_pGameWorld->m_level.QueryNearestRegions(GetPlayerCar()->GetOrigin(), false);

	m_missionStatus = MIS_STATUS_INGAME;
}

extern ConVar sys_maxfps;


float CGameSessionBase::LoadCarReplay(CCar* pCar, const char* filename)
{
	int numTicks = 0;
	g_replayData->LoadVehicleReplay(pCar, filename, numTicks);

	float fixedTicksDelta = 1.0f / (sys_maxfps.GetFloat()*GetPhysicsIterations());

	return numTicks * fixedTicksDelta;
}

void CGameSessionBase::StopCarReplay(CCar* pCar)
{
	g_replayData->StopVehicleReplay(pCar);
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

void CGameSessionBase::Shutdown()
{
	g_pAIManager->Shutdown();

	m_leadCar = NULL;

	// car configs
	for (int i = 0; i < m_carEntries.numElem(); i++)
		delete m_carEntries[i];

	m_carEntries.clear();
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
	return (g_replayData->m_state == REPL_PLAYING ||
		g_replayData->m_state == REPL_INIT_PLAYBACK);
}

void CGameSessionBase::ResetReplay()
{
	g_replayData->m_tick = 0;
	g_replayRandom.SetSeed(0);

	// FIXME: remove objects?
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

	if (g_replayData)
	{
		// always regenerate predictable random
		g_replayRandom.SetSeed(g_replayData->m_tick);
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

				if (plrCar)
					removePos = plrCar->GetOrigin();

				// vehicle respawn is oriented on the lead car, but removal is always depends on player
				g_pAIManager->UpdateCarRespawn(fDt, spawnPos, removePos, leadCar->GetVelocity());
			}
		}

		g_pAIManager->UpdateNavigationVelocityMap(fDt);
	}

	// do replays and prerecorded stuff
	if (g_replayData)
		g_replayData->UpdatePlayback(fDt);
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

	g_pGameWorld->m_level.QueryNearestRegions(car->GetOrigin(), false);

	if (!g_replayData->IsCarPlaying(car))
	{
		// apply cheats
		{
			if (g_invicibility.GetBool())
				car->SetDamage(0.0f);

			car->SetInfiniteMass(g_infiniteMass.GetBool());
		}

		car->SetAutoHandbrake(g_autoHandbrake.GetBool());		// auto handbrake is a setting
		car->SetControlButtons(control.buttons);				// set the controls
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
		g_pGameWorld->UpdateWorld(0.0f);
		return;
	}

	UpdatePlayerControls();

	if (GetLeadCar())
		g_pGameWorld->m_level.QueryNearestRegions(GetLeadCar()->GetOrigin(), false);

	float phys_begin = MEASURE_TIME_BEGIN();
	g_pPhysics->Simulate(fDt, GetPhysicsIterations(), Game_OnPhysicsUpdate);

	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "physics time, ms: %g", abs(MEASURE_TIME_STATS(phys_begin)));

	g_pAIManager->UpdateCopStuff(fDt);

	// updates world
	g_pGameWorld->UpdateWorld(fDt);

	{
		OOLUA::Script& state = GetLuaState();
		EqLua::LuaStackGuard g(state);

		// bind function and call, table pushed automatically
		m_lua_misman_Update.Push();

		OOLUA::push(state, fDt);

		if (!m_lua_misman_Update.Call(1, 0))
		{
			Msg("CGameSessionBase::Init, :CMissionManager_InitMission() error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
}

CCar* CGameSessionBase::CreateCar(const char* name, int carType)
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		vehicleConfig_t* config = m_carEntries[i];

		if (!config->carName.CompareCaseIns(name))
		{
			CCar* car = NULL;

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
	}

	MsgError("Can't create car '%s'\n", name);

	return NULL;
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
			g_pAIManager->m_copCars.append(car);
			g_pAIManager->m_trafficCars.append(car);

			return car;
		}
	}

	MsgError("Can't create pursuer car '%s'\n", name);

	return NULL;
}

CCar* CGameSessionBase::Lua_CreateCar(const char* name, int type)
{
	if (g_replayData && g_replayData->m_state == REPL_PLAYING)
	{
		CGameObject* scriptObject = FindScriptObjectById(m_scriptIDReplayCounter);
		m_scriptIDReplayCounter++;

		if (scriptObject && (scriptObject->ObjType() == GO_CAR || scriptObject->ObjType() == GO_CAR_AI))
			return (CCar*)scriptObject;

		ASSERTMSG(false, varargs("Lua_CreateCar - no valid script car by replay (id=%d)", m_scriptIDReplayCounter - 1));
	}
	else
		return CreateCar(name, type);

	return NULL;
}

CAIPursuerCar* CGameSessionBase::Lua_CreatePursuerCar(const char* name, int type)
{
	if (g_replayData && g_replayData->m_state == REPL_PLAYING)
	{
		CGameObject* scriptObject = FindScriptObjectById(m_scriptIDReplayCounter);
		m_scriptIDReplayCounter++;

		if (scriptObject && scriptObject->ObjType() == GO_CAR_AI)
			return (CAIPursuerCar*)scriptObject;

		ASSERTMSG(false, varargs("Lua_CreatePursuerCar - no valid script car by replay (id=%d)", m_scriptIDReplayCounter - 1));
	}
	else
		return CreatePursuerCar(name, type);

	return NULL;
}

void CGameSessionBase::LoadCarData()
{
	// delete old entries
	for (int i = 0; i < m_carEntries.numElem(); i++)
		delete m_carEntries[i];

	m_carEntries.clear(false);

	PrecacheObject(CCar);

	// initialize vehicle list

	KeyValues kvs;
	if (kvs.LoadFromFile("scripts/vehicles.txt"))
	{
		kvkeybase_t* vehicleRegistry = kvs.GetRootSection()->FindKeyBase("vehicles");

		for (int i = 0; i < vehicleRegistry->keys.numElem(); i++)
		{
			kvkeybase_t* key = vehicleRegistry->keys[i];
			if (!key->IsSection() && !key->IsDefinition())
			{
				vehicleConfig_t* carent = new vehicleConfig_t();
				carent->carName = key->name;
				carent->carScript = KV_GetValueString(key);

				carent->scriptCRC = g_fileSystem->GetFileCRC32(carent->carScript.c_str(), SP_MOD);

				if (carent->scriptCRC != 0)
				{
					kvkeybase_t kvs;

					if (!KV_LoadFromFile(carent->carScript.c_str(), SP_MOD, &kvs))
					{
						MsgError("can't load default car script '%s'\n", carent->carScript.c_str());
						delete carent;
						return;
					}

					if (!ParseVehicleConfig(carent, &kvs))
					{
						MsgError("Car configuration '%s' is invalid!\n", carent->carScript.c_str());
						delete carent;
						return;
					}

					PrecacheStudioModel(carent->visual.cleanModelName.c_str());
					PrecacheStudioModel(carent->visual.damModelName.c_str());
					m_carEntries.append(carent);
				}
				else
					MsgError("Can't open car script '%s'\n", carent->carScript.c_str());
			}
		}

		kvkeybase_t* zone_presets = kvs.GetRootSection()->FindKeyBase("zones");

		if (zone_presets)
		{
			// thru all zone presets
			for (int i = 0; i < zone_presets->keys.numElem(); i++)
			{
				kvkeybase_t* zone_kv = zone_presets->keys[i];

				// thru vehicles in zone preset
				for (int i = 0; i < zone_kv->keys.numElem(); i++)
				{
					vehicleConfig_t* carConfig = FindCarEntryByName(zone_kv->keys[i]->name);

					if (!carConfig)
					{
						MsgWarning("Unknown vehicle '%s' for zone '%s'\n", zone_kv->keys[i]->name, zone_kv->name);
						continue;
					}

					civCarEntry_t* civCarEntry = g_pAIManager->FindCivCarEntry(zone_kv->keys[i]->name);

					// add new car entry if not exist
					if (!civCarEntry)
					{
						civCarEntry_t entry;
						entry.config = carConfig;

						int idx = g_pAIManager->m_civCarEntries.append(entry);
						civCarEntry = &g_pAIManager->m_civCarEntries[i];
					}

					int spawnInterval = KV_GetValueInt(zone_kv->keys[i], 0, 0);

					// append zone with given spawn interval
					civCarEntry->zoneList.append(carZoneInfo_t{ zone_kv->name, spawnInterval });
				}
			}
		}
		else
		{
			for (int i = 0; i < m_carEntries.numElem(); i++)
			{
				civCarEntry_t entry;
				entry.config = m_carEntries[i];
				entry.zoneList.append(carZoneInfo_t{ "default", 0 });

				g_pAIManager->m_civCarEntries.append(entry);
			}
		}

		kvkeybase_t* lightCop = kvs.GetRootSection()->FindKeyBase("lightcop");
		kvkeybase_t* heavyCop = kvs.GetRootSection()->FindKeyBase("heavycop");

		g_pAIManager->SetCopCarConfig(KV_GetValueString(lightCop, 0, "cop_regis"), COP_LIGHT);
		g_pAIManager->SetCopCarConfig(KV_GetValueString(heavyCop, 0, "cop_regis"), COP_HEAVY);
	}
	else
	{
		CrashMsg("FATAL: no scripts/vehicles.txt file!");
	}
}

void CGameSessionBase::GetCarNames(DkList<EqString>& list) const
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		if (m_carEntries[i]->flags.isCar)
			list.append(m_carEntries[i]->carName);
	}
}

vehicleConfig_t* CGameSessionBase::FindCarEntryByName(const char* name) const
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		if (!m_carEntries[i]->carName.CompareCaseIns(name))
			return m_carEntries[i];
	}

	return nullptr;
}

extern ConVar g_pause;

CCar* CGameSessionBase::GetViewCar() const
{
	if (g_replayData->m_state == REPL_PLAYING && !g_pause.GetBool())
	{
		replaycamera_t* cam = g_replayData->GetCurrentCamera();

		if (cam)
			return g_replayData->GetCarByReplayIndex(cam->targetIdx);
	}

	if (m_viewCar)
		return m_viewCar;

	if (m_viewCar == nullptr)
		return GetPlayerCar();

	return nullptr;
}

void CGameSessionBase::SetViewCar(CCar* pCar)
{
	if (m_viewCar == GetPlayerCar())
		m_viewCar = nullptr;
	else
		m_viewCar = pCar;
}

void CGameSessionBase::SetViewCarNone()
{
	SetViewCar(nullptr);
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
	SetPlayerCar,
	SetLeadCar,
	SetLeadCarNone,
	SetViewCar,
	SetViewCarNone,
	LoadCarReplay,
	StopCarReplay,
	SignalMissionStatus,
	ResetReplay
)
OOLUA_EXPORT_FUNCTIONS_CONST(
	CGameSessionBase,

	IsClient,
	IsServer,
	GetPlayerCar,
	GetLeadCar,
	GetViewCar,
	GetSessionType,
	IsGameDone,
	GetMissionStatus,
	IsReplay
)
