//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#include "luabinding/LuaBinding.h"

#include "game_singleplayer.h"
#include "replay.h"
#include "eqParallelJobs.h"
#include "DrvSynHUD.h"

#include "Shiny.h"	// profiler

extern CGameSession* g_pGameSession;

EqString	g_scriptName = "defaultmission";
ConVar		g_car("g_car", "rollo", "player car", CV_ARCHIVE);

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

bool LoadMissionScript( const char* name )
{
	g_scriptName = name;

	// first of all we reinitialize default mission script
	if( !EqLua::LuaBinding_LoadAndDoFile( GetLuaState(), varargs("scripts/missions/defaultmission.lua"), "defaultmission" ) )
	{
		MsgError("default mission script init error:\n\n%s\n", OOLUA::get_last_error(GetLuaState()).c_str());
		return false;
	}

	// don't start both times
	if(g_scriptName != "defaultmission")
	{
		EqString scriptFileName(varargs("scripts/missions/%s.lua", g_scriptName.c_str()));

		// then we load custom script
		if( !EqLua::LuaBinding_LoadAndDoFile( GetLuaState(), scriptFileName.c_str(), g_scriptName.c_str() ) )
		{
			MsgError("mission script init error:\n\n%s\n", OOLUA::get_last_error(GetLuaState()).c_str());
			g_scriptName = "defaultmission";
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------

CGameSession::CGameSession()
{
	m_waitForExitTime = 0.0f;
	m_missionStatus = MIS_STATUS_INGAME;

	m_localControls = 0;
	m_playerCar = NULL;
	m_leadCar = NULL;

	m_scriptIDCounter = 0;
}

CGameSession::~CGameSession()
{

}

void CGameSession::Init()
{
	PrecacheObject(CAIPursuerCar);

	OOLUA::Script& state = GetLuaState();

	EqLua::LuaStackGuard g(state);

	// init mission manager pointers
	if(!OOLUA::get_global(state, "missionmanager", m_missionManagerTable))
	{
		MsgError("CGameSession::Init Error - cannot get missionmanager Lua table!\n");
	}

	if(!m_lua_misman_InitMission.Get(m_missionManagerTable, "InitMission"))
	{
		MsgError("CGameSession::Init Error - cannot get missionmanager's InitMission() function!\n");
	}

	if(!m_lua_misman_Update.Get(m_missionManagerTable, "Update"))
	{
		MsgError("CGameSession::Init Error - cannot get missionmanager's Update() function!\n");
	}

	g.Release();

	//-------------------------------------------------------------------
	m_scriptIDCounter = 0;

	g_pAIManager->Init();

	// load cars
	LoadCarData();

	g_pGameWorld->m_random.SetSeed(0);

	// start recorder
	if( g_replayData->m_state != REPL_INITIALIZE )
	{
		g_replayData->StartRecording();
	}

	// MISSION SCRIPT INITIALIZER CALL
	{
		// bind function and call, table pushed automatically
		m_lua_misman_InitMission.Push();

		if(!m_lua_misman_InitMission.Call(0, 0))
			Msg("CGameSession::Init, :CMissionManager_InitMission() error:\n %s\n", OOLUA::get_last_error(state).c_str());
	}

	if( g_replayData->m_state == REPL_INITIALIZE )
	{
		g_replayData->StartPlay();
	}

	// start recorder
	if( g_replayData->m_state != REPL_INITIALIZE )
	{
		//
		// Spawn default car if script not did
		//
		if( GetPlayerCar() == NULL )
		{
			// make a player car
			CCar* playerCar = CreateCar( g_car.GetString() );

			if(playerCar)
			{
				playerCar->SetOrigin(Vector3D(0, 0.5, 10));
				playerCar->SetAngles(Vector3D(0,0,0));
				playerCar->Spawn();

				g_pGameWorld->AddObject( playerCar );

				SetPlayerCar(playerCar);
			}
		}

		// load regions on player car position
		g_pGameWorld->m_level.QueryNearestRegions( GetPlayerCar()->GetOrigin(), false);
	}

	m_missionStatus = MIS_STATUS_INGAME;
}

void CGameSession::LoadCarReplay(CCar* pCar, const char* filename)
{
	if(	g_replayData->m_state != REPL_PLAYING &&
		g_replayData->m_state != REPL_INITIALIZE)
		g_replayData->LoadVehicleReplay(pCar, filename);
}

void CGameSession::Shutdown()
{
	if( g_replayData->m_state == REPL_RECORDING )
	{
		g_replayData->Stop();
		g_replayData->SaveToFile("last.rdat");
	}

	g_pAIManager->Shutdown();

	for(int i = 0; i < m_carEntries.numElem(); i++)
		delete m_carEntries[i];

	m_carEntries.clear();

	m_playerCar = NULL;
	m_localControls = 0;
	m_leadCar = NULL;
}

bool CGameSession::IsGameDone(bool checkTime /*= true*/) const
{
	if(checkTime)
		return (m_missionStatus > MIS_STATUS_INGAME) && m_waitForExitTime <= 0.0f;
	else
		return (m_missionStatus > MIS_STATUS_INGAME);
}

void CGameSession::SignalMissionStatus(int missionStatus, float waitForExitTime)
{
	m_waitForExitTime = waitForExitTime;
	m_missionStatus = missionStatus;
}

int	CGameSession::GetMissionStatus() const
{
	return m_missionStatus;
}

bool CGameSession::IsReplay() const
{
	return (g_replayData->m_state == REPL_PLAYING ||
			g_replayData->m_state == REPL_INITIALIZE);
}

void CGameSession::ResetReplay()
{
	g_replayData->m_tick = 0;
	g_pGameWorld->m_random.SetSeed(0);

	// FIXME: remove objects?
}

int	CGameSession::GenScriptID()
{
	return m_scriptIDCounter++;
}

CGameObject* CGameSession::FindScriptObjectById(int scriptID)
{
	const DkList<CGameObject*>& objList = g_pGameWorld->m_pGameObjects;

	for(int i = 0; i < objList.numElem(); i++)
	{
		if( objList[i]->GetScriptID() == scriptID )
			return objList[i];
	}

	return NULL;
}

void Game_OnPhysicsUpdate(float fDt, int iterNum)
{
	PROFILE_FUNC();

	if(fDt <= 0.0f)
		return;

	//CScopedMutex m(g_parallelJobs->GetMutex());

	if(g_replayData)
		g_pGameWorld->m_random.SetSeed(g_replayData->m_tick);

	// update traffic car spawn/remove from here
	if( IsServer() && (iterNum == 0) && g_pGameSession->GetLeadCar())
	{
		Vector3D spawnPos = g_pGameSession->GetLeadCar()->GetOrigin();
		Vector3D removePos = g_pGameSession->GetLeadCar()->GetOrigin();

		if( g_pGameSession->GetPlayerCar() )
			removePos = g_pGameSession->GetPlayerCar()->GetOrigin();

		Vector3D leadVel = g_pGameSession->GetLeadCar()->GetVelocity();


		g_pAIManager->UpdateCarRespawn(fDt, spawnPos, removePos, leadVel);
	}

	// update replay recording
	if(g_replayData)
	{
		//g_replayData->UpdateRecording(fDt);
		g_replayData->UpdatePlayback(fDt);
	}
}

void GameJob_UpdatePhysics(void* data)
{
	float fDt = *(float*)data;

	float phys_begin = MEASURE_TIME_BEGIN();
	g_pPhysics->Simulate( fDt, PHYSICS_ITERATION_COUNT, Game_OnPhysicsUpdate );
	debugoverlay->Text(ColorRGBA(1,1,0,1), "physics time, ms: %g", abs(MEASURE_TIME_STATS(phys_begin)));
}

//
// Updates the game
//
void CGameSession::Update( float fDt )
{
	PROFILE_FUNC();

	if( m_missionStatus != MIS_STATUS_INGAME )
	{
		m_waitForExitTime -= fDt;

		m_waitForExitTime = max(0,m_waitForExitTime);
	}

	if(fDt <= 0.0f)
	{
		g_pGameWorld->UpdateWorld( 0.0f );
		return;
	}

	CCar* player_car = GetPlayerCar();

	if (GetLeadCar())
		g_pGameWorld->m_level.QueryNearestRegions(GetLeadCar()->GetOrigin(), false);

	if (GetPlayerCar())
		g_pGameWorld->m_level.QueryNearestRegions(GetPlayerCar()->GetOrigin(), false);

	if( player_car )
	{
		if( !g_replayData->IsCarPlaying( player_car ) )
			m_playerCar->SetControlButtons( m_localControls );

		debugoverlay->Text(ColorRGBA(1,1,0,1), "Car speed: %.1f MPH, Gear: %d", player_car->GetSpeed(), player_car->GetGear());
		debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "Felony: %.2f, pursued by: %d\n", player_car->GetFelony()*100.0f, player_car->GetPursuedCount());
	}

	//static float jobFrametime = fDt;
	//jobFrametime = fDt;

	//g_parallelJobs->AddJob(GameJob_UpdatePhysics, &jobFrametime);
	//g_parallelJobs->Submit();

	float phys_begin = MEASURE_TIME_BEGIN();
	g_pPhysics->Simulate( fDt, PHYSICS_ITERATION_COUNT, Game_OnPhysicsUpdate );

	debugoverlay->Text(ColorRGBA(1,1,0,1), "physics time, ms: %g", abs(MEASURE_TIME_STATS(phys_begin)));

	g_pAIManager->UpdateCopStuff(fDt);

	// updates world
	g_pGameWorld->UpdateWorld( fDt );

	{
		OOLUA::Script& state = GetLuaState();
		EqLua::LuaStackGuard g(state);

		// bind function and call, table pushed automatically
		m_lua_misman_Update.Push();

		OOLUA::push( state, fDt );

		if(!m_lua_misman_Update.Call(1, 0))
		{
			Msg("CGameSession::Init, :CMissionManager_InitMission() error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
}

void CGameSession::UpdateLocalControls( int nControls )
{
	CCar* playerCar = GetPlayerCar();

	/*
	// filter
	if(	playerCar &&
		(playerCar->m_conf->m_sirenType != SIREN_NONE &&
		playerCar->m_sirenEnabled &&
		(nControls & IN_HORN)))
	{
		nControls &= ~IN_HORN;
	}*/

	m_localControls = nControls;
}

CCar* CGameSession::CreateCar(const char* name, int carType)
{
	for(int i = 0; i < m_carEntries.numElem(); i++)
	{
		if(!m_carEntries[i]->carName.CompareCaseIns(name))
		{
			CCar* car = NULL;

			if (carType == CAR_TYPE_NORMAL)
				car = new CCar(m_carEntries[i]);
			else if (carType == CAR_TYPE_TRAFFIC_AI)
			{
				CAITrafficCar* traffic = new CAITrafficCar(m_carEntries[i]);
				traffic->InitAI(NULL,NULL);

				car = traffic;
			}
			else if (carType == CAR_TYPE_PURSUER_COP_AI)
			{
				CAIPursuerCar* traffic = new CAIPursuerCar(m_carEntries[i], PURSUER_TYPE_COP);
				traffic->InitAI(NULL, NULL);

				car = traffic;
			}
			else if (carType == CAR_TYPE_PURSUER_GANG_AI)
			{
				CAIPursuerCar* traffic = new CAIPursuerCar(m_carEntries[i], PURSUER_TYPE_GANG);
				traffic->InitAI(NULL, NULL);

				car = traffic;
			}

			if (!car)
				MsgError("Car is exist, but unknown car type '%d'\n", carType);

			return car;
		}
	}

	MsgError("cannot find car %s\n", name);

	return NULL;
}

CAIPursuerCar* CGameSession::CreatePursuerCar(const char* name, int type)
{
	for(int i = 0; i < m_carEntries.numElem(); i++)
	{
		if(!m_carEntries[i]->carName.CompareCaseIns(name))
		{
			CAIPursuerCar* car = new CAIPursuerCar(m_carEntries[i],(EPursuerAIType) type);
			car->InitAI(NULL, NULL);

			// manage this car
			g_pAIManager->m_copCars.append(car);
			g_pAIManager->m_trafficCars.append(car);

			return car;
		}
	}

	MsgError("cannot find car %s\n", name);

	return NULL;
}

void CGameSession::LoadCarData()
{
	// delete old entries
	for(int i = 0; i < m_carEntries.numElem(); i++)
		delete m_carEntries[i];

	m_carEntries.clear(false);

	PrecacheObject( CCar );

	// initialize vehicle list

	KeyValues kvs;
	if(kvs.LoadFromFile("scripts/cars.txt"))
	{
		for(int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
		{
			kvkeybase_t* key = kvs.GetRootSection()->keys[i];
			if(!key->IsSection() && !key->IsDefinition())
			{
				carConfigEntry_t* carent = new carConfigEntry_t();
				carent->carName = key->name;
				carent->carScript = KV_GetValueString(key);

				carent->scriptCRC = g_fileSystem->GetFileCRC32(carent->carScript.c_str(), SP_MOD);

				if(carent->scriptCRC != 0)
				{
					kvkeybase_t kvs;

					if( !KV_LoadFromFile(carent->carScript.c_str(), SP_MOD,&kvs) )
					{
						MsgError("can't load default car script '%s'\n", carent->carScript.c_str());
						delete carent;
						return;
					}

					if(!ParseCarConfig(carent, &kvs))
					{
						MsgError("Car configuration '%s' is invalid!\n", carent->carScript.c_str());
						delete carent;
						return;
					}

					PrecacheStudioModel( carent->m_cleanModelName.c_str() );
					PrecacheStudioModel( carent->m_damModelName.c_str() );

					Msg("added car '%s'\n", carent->carName.c_str());
					m_carEntries.append(carent);
				}
				else
					MsgError("cannot open car parameters file '%s'\n", carent->carScript.c_str());
			}
		}

		kvkeybase_t* civCars = kvs.GetRootSection()->FindKeyBase("civilian");

		if (civCars)
		{
			for (int i = 0; i < civCars->keys.numElem(); i++)
			{
				carConfigEntry_t* conf = FindCarEntryByName(civCars->keys[i]->name );

				if(conf)
					g_pAIManager->m_civCarEntries.append(conf);
			}
		}
		else
			g_pAIManager->m_civCarEntries.append(m_carEntries);

		kvkeybase_t* lightCop = kvs.GetRootSection()->FindKeyBase("lightcop");
		kvkeybase_t* heavyCop = kvs.GetRootSection()->FindKeyBase("heavycop");

		g_pAIManager->SetCopCarConfig(KV_GetValueString(lightCop, 0, "cop_regis"), COP_LIGHT);
		g_pAIManager->SetCopCarConfig(KV_GetValueString(heavyCop, 0, "cop_regis"), COP_HEAVY);
	}
	else
	{
		MsgError("no scripts/cars.txt file!");
	}
}

carConfigEntry_t* CGameSession::FindCarEntryByName(const char* name)
{
	for (int i = 0; i < m_carEntries.numElem(); i++)
	{
		if (!m_carEntries[i]->carName.CompareCaseIns(name))
			return m_carEntries[i];
	}

	return NULL;
}

CCar* CGameSession::GetPlayerCar() const
{
	return m_playerCar;
}

extern ConVar g_pause;

CCar* CGameSession::GetViewCar() const
{
	if(g_replayData->m_state == REPL_PLAYING && !g_pause.GetBool())
	{
		replaycamera_t* cam = g_replayData->GetCurrentCamera();

		if( cam )
			return g_replayData->GetCarByReplayIndex( cam->targetIdx );
	}

	return GetPlayerCar();
}

void CGameSession::SetPlayerCar(CCar* pCar)
{
	if(m_playerCar)
		m_playerCar->m_isLocalCar = false;

	m_playerCar = pCar;

	g_pGameHUD->SetDisplayMainVehicle(m_playerCar);

	if(m_playerCar)
		m_playerCar->m_isLocalCar = true;
}

CCar* CGameSession::GetLeadCar() const
{
	if(m_leadCar)
		return m_leadCar;

	return GetPlayerCar();
}

void CGameSession::SetLeadCar(CCar* pCar)
{
	m_leadCar = pCar;
}

OOLUA_EXPORT_FUNCTIONS(
	CGameSession,

	CreateCar,
	CreatePursuerCar,
	SetPlayerCar,
	SetLeadCar,
	LoadCarReplay,
	SignalMissionStatus,
	ResetReplay
)
OOLUA_EXPORT_FUNCTIONS_CONST(
	CGameSession,

	IsClient,
	IsServer,
	GetPlayerCar,
	GetLeadCar,
	GetSessionType,
	IsGameDone,
	GetMissionStatus,
	IsReplay
)
