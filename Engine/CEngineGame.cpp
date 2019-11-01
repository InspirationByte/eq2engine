//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine interface for game
//				Contains base instuments for game
//////////////////////////////////////////////////////////////////////////////////

#include "IEngineGame.h"
#include "IDebugOverlay.h"
#include "IGameLibrary.h"
#include "GlobalVarsBase.h"
#include "KeyBinding/InputCommandBinder.h"
#include "sys_in_console.h"
#include "EqGameLevel.h"
#include "studio_egf.h"
#include "IViewRenderer.h"
#include "IDkPhysics.h"
//#include "Network/net_thread.h"
#include "utils/SmartPtr.h"

#include "sys_enginehost.h"

#include "Utils/eqthread.h"

using namespace Threading;

DkList<inputCommand_t> g_inputCommands;

// PERFORMANCE INFO
#define MIN_FPS         0.1         // Host minimum fps value for maxfps.
#define MAX_FPS         5000.0      // Upper limit for maxfps.

#define MAX_FRAMETIME	0.3
#define MIN_FRAMETIME	0.00001

enum LoadingState_e
{
	LOADING_NONE = 0,
	LOADING_INPROCESS,

	LOADING_DONE,
	LOADING_ERROR,

};

class CEngineGame : public IEngineGame, public CEqThread
{
public:
						CEngineGame( void );
	virtual				~CEngineGame( void );

	bool				IsInitialized() const {return true;}
	const char*			GetInterfaceName() const {return IENGINEGAME_INTERFACE_VERSION;};

	bool				Init( void );
	void				Shutdown( void );

	double				GetFrameTime( void );

	void				ProcessKeyChar		( ubyte ch );
	void				Key_Event			( int key, bool down );
	void				Mouse_Event			( float x, float y, int buttons, bool down );
	void				MouseMove_Event		( int x, int y, float deltaX, float deltaY );
	void				MouseWheel_Event	( int x, int y, int scroll);

	int					GetGameState( void );
	void				SetGameState( int gamestateType );

	bool				LoadSaveGame( pfnSavedGameLoader func, void* pData );

	bool				SetLevelName( char const* pszLevelName );

	bool				LoadGameCycle( bool loadSaved = false );
	void				UnloadGame(bool freeCache, bool force = false);
	bool				IsLevelChanged() {return m_bLevelChanged;}

	bool				Level_LoadWorldModels(const char* newworld);

	bool				EngineRunFrame( float dTime );
	bool				FilterTime( float dTime );
	double				StateUpdate( void );
	void				Reset( bool frameTimeOnly = false );

	void				SetCenterMouseCursor(bool enable) {m_bCenterMouse = enable;}

private:

	virtual	int			Run();

	double				m_fFrameTime;
	double				m_fRealTime;
	double				m_fOldRealTime;

	int					m_nGameState;

	bool				m_bIsGameLoaded;
	LoadingState_e		m_nLoadingState;

	EqString			m_sLevelName;
	bool				m_bLevelChanged;

	bool				m_bLoadFromSavedGame;

	bool				m_bCenterMouse;

	pfnSavedGameLoader	m_pfnSavedGameLoader;
	void*				m_pSaveGameData;

	CEqMutex			m_Mutex;
};

static CEngineGame	g_EngineGame;
IEngineGame*		engine = ( IEngineGame * )&g_EngineGame;


debugGraphBucket_t	g_gameUpdateGraph;
debugGraphBucket_t	g_preRenderGraph;
debugGraphBucket_t	g_sceneRenderGraph;
debugGraphBucket_t	g_postRenderGraph;

DECLARE_CMD(pause, "Pauses and unpauses the game", 0)
{
	if(g_consoleInput->IsVisible())
		return;

	if(engine->GetGameState() == IEngineGame::GAME_IDLE)
	{
		engine->SetGameState(IEngineGame::GAME_PAUSE);
	}
	else if(engine->GetGameState() == IEngineGame::GAME_PAUSE)
	{
		engine->SetGameState(IEngineGame::GAME_IDLE);
	}
}

DECLARE_CMD(unload,"Unload current level",0)
{
	// Set states to synchronize engine with input
	engine->SetGameState(IEngineGame::GAME_LEVEL_UNLOAD);
}

GlobalVarsBase*		gpGlobals = NULL;

CEngineGame::CEngineGame()
{
	m_fFrameTime			= 0.0f;
	m_fOldRealTime			= 0.0f;

	m_bIsGameLoaded		= false;
	m_bLevelChanged			= true;

	m_bLoadFromSavedGame	= false;

	m_bCenterMouse			= false;

	m_nLoadingState			= LOADING_NONE;

	m_nGameState			= GAME_NOTRUNNING;

	GetCore()->RegisterInterface( IENGINEGAME_INTERFACE_VERSION, this);

}

CEngineGame::~CEngineGame()
{
}

bool CEngineGame::Init( void )
{
	g_gameUpdateGraph.Init("Game update time", ColorRGB(1,1,0), 100.0f, 0.25f, true);
	g_preRenderGraph.Init("Prerender time", ColorRGB(1,1,0), 100.0f, 0.25f, true);
	g_sceneRenderGraph.Init("Scene render time", ColorRGB(1,0,0), 100.0f, 0.25f, true);
	g_postRenderGraph.Init("Postrender time", ColorRGB(1,1,0), 100.0f, 0.25f, true);

	return true;
}

void  CEngineGame::Shutdown( void )
{

}

void CEngineGame::Key_Event( int key, bool down )
{
	if( gamedll->Key_Event( key, down) )
		g_inputCommandBinder->OnKeyEvent(key,down);
}

void CEngineGame::Mouse_Event( float x, float y, int buttons, bool down )
{
	if(gamedll->Mouse_Event( x, y, buttons, down ))
		g_inputCommandBinder->OnMouseEvent(buttons,down);
}

void CEngineGame::MouseMove_Event( int x, int y, float deltaX, float deltaY )
{
	if(m_nGameState != GAME_PAUSE)
		gamedll->MouseMove_Event(x, y, deltaX, deltaY);
}

void CEngineGame::MouseWheel_Event(int x, int y, int scroll)
{
	if(gamedll->MouseWheel_Event(x, y, scroll))
		g_inputCommandBinder->OnMouseWheel(scroll);
}

void CEngineGame::ProcessKeyChar( ubyte ch )
{
	// Only in-game ui
	gamedll->ProcessKeyChar( ch );
}

double CEngineGame::StateUpdate( void )
{
	double frameTime = m_fFrameTime;

	switch(m_nGameState)
	{
		case GAME_NOTRUNNING:
			// Don't update
			frameTime = 0.0f;

			break;
		case GAME_LEVEL_LOAD:
			
			frameTime = 0.0f;

			// load game
			if( !IsRunning() && m_nLoadingState == LOADING_NONE )
			{
				m_nLoadingState = LOADING_INPROCESS;

				//g_pEngineHost->EnterResourceLoading();

				StartThread("EqLoaderThread");
			}

			// there is final phase handling
			if(m_nLoadingState != LOADING_INPROCESS)
			{
				WaitForThread();

				if(m_nLoadingState == LOADING_DONE)
				{
					if(m_bLoadFromSavedGame)
					{
						(m_pfnSavedGameLoader)( m_pSaveGameData );
					}
					else
					{
						gamedll->GameStart();
						Reset();
					}

					//g_pEngineHost->EndResourceLoading();

					g_consoleInput->SetVisible( false );
					g_pEngineHost->SetCursorShow(false);
				}
				else if(m_nLoadingState == LOADING_ERROR)
				{
					SetGameState(GAME_NOTRUNNING);
					m_bLevelChanged = true;
					UnloadGame(true);

					g_consoleInput->SetVisible(true);

					Reset();

					//g_pEngineHost->EndResourceLoading();
				}

				m_bLoadFromSavedGame = false;

				// set there is nothing loading right now
				m_nLoadingState = LOADING_NONE;
			}

			return frameTime;
		case GAME_LEVEL_UNLOAD:

			if(!m_bIsGameLoaded)
			{
				Msg("No game started, nothing to unload\n");
				SetGameState(GAME_NOTRUNNING);
				break;
			}

			Reset();

			frameTime = 0.0f;

			gamedll->GameEnd();

			m_bLevelChanged = true;
			UnloadGame(true);

			SetGameState(GAME_NOTRUNNING);
			break;
		case GAME_PAUSE:
			// Stop all on pause
			frameTime = 0.0f;
			break;
		case GAME_IDLE:

			// wait for loading thread and don't do gamedll state updates
			if(IsRunning())
				return frameTime;

			// Do nothing
			break;
	}

	return gamedll->StateUpdate(m_nGameState, frameTime);
}

int	CEngineGame::GetGameState( void )
{
	return m_nGameState;
}

void CEngineGame::SetGameState( int gamestateType )
{
	m_Mutex.Lock();
	m_nGameState = gamestateType;
	m_Mutex.Unlock();
}

bool CEngineGame::SetLevelName( char const* pszLevelName )
{
	if(!g_pLevel->CheckLevelExist(pszLevelName))
	{
		MsgError("ERROR: Level '%s' doesn't exist\n", pszLevelName);
		return false;
	}

	if(stricmp(m_sLevelName.GetData(), pszLevelName))
		m_bLevelChanged = true;

	m_sLevelName = pszLevelName;

	return true;
}

bool CEngineGame::LoadSaveGame( pfnSavedGameLoader func, void* pData )
{
	m_bLoadFromSavedGame = true;

	m_pfnSavedGameLoader = func;
	m_pSaveGameData = pData;

	SetGameState( IEngineGame::GAME_LEVEL_LOAD );

	return true;
}

class CEqPhysicsThread : public CEqThread
{
public:
	CEqPhysicsThread()
	{
		m_fDT = 0;
		m_fLastUpdate = 0;
	}

	virtual int Run()
	{
		float measure_physics = MEASURE_TIME_BEGIN();

		physics->Simulate(m_fDT, m_nSubSteps);
		
		m_SetFrameTimeMutex.Lock();
		m_fLastUpdate = abs(MEASURE_TIME_STATS(measure_physics));
		m_SetFrameTimeMutex.Unlock();

		return 0;
	}

	void SetFrameTime(float fDT, int nSubSteps)
	{
		m_SetFrameTimeMutex.Lock();

		m_fDT = fDT;
		m_nSubSteps = nSubSteps;

		m_SetFrameTimeMutex.Unlock();
	}

	float GetLastUpdateTime()
	{
		return m_fLastUpdate;
	}

	float	m_fDT;
	int		m_nSubSteps;

	int		m_fLastUpdate;

	CEqMutex m_SetFrameTimeMutex;
};

CEqPhysicsThread g_PhysicsThread;

int	CEngineGame::Run()
{
	// unload first
	UnloadGame( m_bLevelChanged );

	if( LoadGameCycle( m_bLoadFromSavedGame ) )
	{
		CScopedMutex m(m_Mutex);
		m_nLoadingState = LOADING_DONE;
	}
	else
	{
		CScopedMutex m(m_Mutex);
		m_nLoadingState = LOADING_ERROR;
	}

	return 0;
}

bool CEngineGame::LoadGameCycle( bool loadSaved )
{
	m_Mutex.Lock();

	// init game dll
	gpGlobals = gamedll->GetGlobalVars();

	m_Mutex.Unlock();

	if(!gpGlobals)
		return false;

	m_bIsGameLoaded = false;

	// start loading thread and loading screen

	gamedll->LoadingScreenFunction(0, 0.0f);

	// start networking thread
	//NETStartThread();

	// start server
	//NETStartServer();

	// reset all game stats
	Reset();

	m_Mutex.Lock();
	strcpy(gpGlobals->worldname,m_sLevelName.GetData());
	m_Mutex.Unlock();

	gamedll->LoadingScreenFunction(LOADINGSCREEN_SHOW, 0.15);

	if(m_bLevelChanged)
	{
		strcpy(gpGlobals->worldname, "no_world");

		float measure_loadw = MEASURE_TIME_BEGIN();

		// create physics scene
		physics->CreateScene();
		
		// proceed to load level

		g_pLevel->m_levelpath = m_sLevelName.GetData();

		gamedll->LoadingScreenFunction(LOADINGSCREEN_SHOW, 0.4f);

		if(	!g_pLevel->LoadCompatibleWorldFile("world.build"))
			return false;

		gamedll->LoadingScreenFunction(LOADINGSCREEN_SHOW, 0.6f);

		if(	!g_pLevel->LoadCompatibleWorldFile("geometry.build"))
			return false;

		gamedll->LoadingScreenFunction(LOADINGSCREEN_SHOW, 0.8f);

		if(	!g_pLevel->LoadCompatibleWorldFile("physics.build"))
			return false;

		g_pLevel->m_bIsLoaded = true;

		strcpy(gpGlobals->worldname, m_sLevelName.GetData());

		DevMsg(DEVMSG_CORE, "World loading time: %f ms\n", MEASURE_TIME_STATS(measure_loadw));
	}

	m_bIsGameLoaded = true;

	// Precache error model file
	g_studioModelCache->PrecacheModel( "models/error.egf" );

	gamedll->LoadingScreenFunction(LOADINGSCREEN_SHOW, 0.9f);

	if(!loadSaved)
	{
		gamedll->SpawnEntities( g_pLevel->m_EntityKeyValues.GetRootSection() );
	}

	m_bLevelChanged = false;

	// reset all game stats
	Reset();

	gamedll->LoadingScreenFunction(LOADINGSCREEN_DESTROY, 1.0f);

	CScopedMutex m(m_Mutex);

	// load all game scripts
	if(!gamedll->GameLoad())
		return false;

	MsgInfo("Loading completed\n");

	g_inputCommands.clear();

	//g_PhysicsThread.StartWorkerThread("EqPhysicsThread");

	return true;
}

void CEngineGame::UnloadGame(bool freeCache, bool force)
{
	if(m_bIsGameLoaded && g_pLevel->IsLoaded())
	{
		//g_PhysicsThread.StopThread( true );

		if(m_bLevelChanged || force)
		{
			materials->Wait();
			g_pShaderAPI->Finish();

			g_pLevel->UnloadLevel();
			memset(gpGlobals->worldname,0,sizeof(gpGlobals->worldname));

			// always destroy physics
			physics->DestroyScene();
		}

		m_bIsGameLoaded = false;
		gamedll->GameUnload();
		soundsystem->Update();
		g_pEngineHost->SetCursorShow(true);

		// stop network threads
		//NETStopThread();

		// shutdown server
		//NETShutdownServer();

		// free all cached data
		if( freeCache )
			g_studioModelCache->ReleaseCache();
	}
	
}

double CEngineGame::GetFrameTime( void )
{
	return m_fFrameTime;
}

DECLARE_CVAR(sys_multistep,1,"Multi-step mode",CV_CHEAT);

#define TICK_FRAMES_PER_SECOND 80

bool CEngineGame::EngineRunFrame( float dTime )
{
	// Reset mouse
	if(GetGameState() != IEngineGame::GAME_RUNNING_MENU)
	{
		if(!g_consoleInput->IsVisible() && GetGameState() == IEngineGame::GAME_IDLE && m_bCenterMouse)
		{
			g_pEngineHost->SetCursorPosition(IVector2D(g_pEngineHost->GetWindowSize().x/2,g_pEngineHost->GetWindowSize().y/2));
		}
	}

	// Do frame skip, time scale
	if(!FilterTime( dTime ))
		return false;

	float frametime = StateUpdate();

	if(frametime == 0.0f && m_nGameState == GAME_IDLE)
		frametime = 0.0001f;

	// Update global vars
	if(gpGlobals)
	{
		gpGlobals->curtime += frametime;
		gpGlobals->frametime = frametime;
		gpGlobals->realtime = m_fRealTime;
	}

	debugoverlay->Graph_DrawBucket(&g_gameUpdateGraph);
	debugoverlay->Graph_DrawBucket(&g_preRenderGraph);
	debugoverlay->Graph_DrawBucket(&g_sceneRenderGraph);
	debugoverlay->Graph_DrawBucket(&g_postRenderGraph);

	// do post-render and matsystem updates only
	if( m_nLoadingState == LOADING_INPROCESS || 
		m_nGameState == GAME_NOTRUNNING || 
		m_nGameState == GAME_LEVEL_UNLOAD || 
		m_nGameState == GAME_LEVEL_LOAD)
	{
		float measure_postrender = MEASURE_TIME_BEGIN();

		// Render frame. It must handle loading screen render
		gamedll->OnRenderScene();

		debugoverlay->Text(Vector4D(1), " game postrender: %.2f ms",abs(MEASURE_TIME_STATS(measure_postrender)));
		debugoverlay->Graph_AddValue(&g_postRenderGraph, abs(MEASURE_TIME_STATS(measure_postrender)));

		return true;
	}

	debugoverlay->Text(Vector4D(1,1,1,1), "------------------------------");

	// update physics
	//g_MTGameEngine.postMessage(ALL_THREADS, MTENGINE_UPDATEPHYSICS, &frametime, sizeof(float));

	int nFrames = (TICK_FRAMES_PER_SECOND*frametime);

	if(!sys_multistep.GetBool())
		nFrames = 1;

	if(nFrames < 1)
		nFrames = 1;

	if(nFrames > 8)
		nFrames = 8;

	debugoverlay->Text(Vector4D(1,1,1,1), "engine iterations: %d\n", nFrames);
	debugoverlay->Text(Vector4D(1,1,1,1), "game time: %.2f\n", gpGlobals->curtime);

	debugoverlay->Text(Vector4D(1,1,1,1), "------------------------------");

	if(GetGameState() == IEngineGame::GAME_IDLE)
	{
		physics->Simulate(frametime, nFrames);
		//g_PhysicsThread.SetFrameTime(frametime, nFrames);
	}

	// subtract full frametime
	if(gpGlobals)
		gpGlobals->curtime -= frametime;

	float frametime_per_upd = frametime / nFrames;

	//g_PhysicsThread.WaitForThread();

	float measure_physics = Platform_GetCurrentTime();//g_PhysicsThread.GetLastUpdateTime();

	debugoverlay->Text(Vector4D(1), " engine physics: %.2f ms",measure_physics);

	float measure_gamedll = MEASURE_TIME_BEGIN();

	for(int i = 0; i < nFrames; i++)
	{
		// use smaller frame time
		if(gpGlobals)
		{
			gpGlobals->curtime += frametime_per_upd;
			gpGlobals->frametime = frametime_per_upd;
		}

		// Update game (hud, stats,entity render list, etc)
		gamedll->Update( frametime_per_upd );
	}

	// restore frame time
	gpGlobals->frametime = frametime;

	debugoverlay->Graph_AddValue(&g_gameUpdateGraph, abs(MEASURE_TIME_STATS(measure_gamedll)));

	// Signal physics thread to work after the game update
	if(GetGameState() == IEngineGame::GAME_IDLE)
	{
		physics->DrawDebug();

		physics->UpdateEvents();
		//g_PhysicsThread.SignalWork();
	}

	debugoverlay->Text(Vector4D(1), " game update: %.2f ms",abs(MEASURE_TIME_STATS(measure_gamedll)));

	float measure_prerender = MEASURE_TIME_BEGIN();
	// Render frame in game
	gamedll->PreRender();
	debugoverlay->Text(Vector4D(1), " game prerender: %.2f ms",abs(MEASURE_TIME_STATS(measure_prerender)));

	debugoverlay->Graph_AddValue(&g_preRenderGraph, abs(MEASURE_TIME_STATS(measure_prerender)));

	float measure_scene = MEASURE_TIME_BEGIN();
	// Draw all (including materials)
	gamedll->OnRenderScene();
	debugoverlay->Text(Vector4D(1), " scene view: %.2f ms",abs(MEASURE_TIME_STATS(measure_scene)));
	debugoverlay->Graph_AddValue(&g_sceneRenderGraph, abs(MEASURE_TIME_STATS(measure_scene)));

	float measure_postrender = MEASURE_TIME_BEGIN();

	// Render frame in game
	gamedll->PostRender();

	debugoverlay->Text(Vector4D(1), " game postrender: %.2f ms",abs(MEASURE_TIME_STATS(measure_postrender)));
	debugoverlay->Graph_AddValue(&g_postRenderGraph, abs(MEASURE_TIME_STATS(measure_postrender)));

	return true;
}

void fnstart_world_variants(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	DKFINDDATA* findData = nullptr;
	EqString fileName = (char*)g_fileSystem->FindFirst("worlds/*", &findData, SP_MOD);

	if (fileName.Length())
	{
		if (g_fileSystem->FindIsDirectory(findData) && fileName.Find(".") == -1)
			list.append(fileName.Path_Strip_Ext());

		while ((fileName = (char*)g_fileSystem->FindNext(findData)).Length())
		{
			if (g_fileSystem->FindIsDirectory(findData) && fileName.Find(".") == -1)
				list.append(fileName.Path_Strip_Ext());
		}

		g_fileSystem->FindClose(findData);
	}
}

DECLARE_CMD_VARIANTS(start_world,"Loads world and starts new game", fnstart_world_variants, 0)
{
	if(CMD_ARGC)
	{
		// Check if level changing is allowed
		if(engine->SetLevelName( CMD_ARGV(0).c_str() ))
			engine->SetGameState(IEngineGame::GAME_LEVEL_LOAD);
	}
	else
	{
		Msg("Usage: start_world <levelname> <[gamemode]>\n");
	}
}

DECLARE_CMD( restart, "Restarts current level", 0)
{
	if(engine->GetGameState() == IEngineGame::GAME_NOTRUNNING)
		return;

	// load game without level name setup
	engine->SetGameState( IEngineGame::GAME_LEVEL_LOAD );
}

ConVar sys_maxfps("sys_maxfps","100","Fps limit for fastest mashines",CV_CHEAT);
ConVar sys_timescale("sys_timescale","1.0f","Frame time scale",CV_CHEAT);

bool CEngineGame::FilterTime( float dTime )
{
	// Accumulate some time
	m_fRealTime += dTime;

	float fps = sys_maxfps.GetFloat();
	if ( fps != 0 )
	{
		// Limit fps to withing tolerable range
		fps = max( MIN_FPS, fps );
		fps = min( MAX_FPS, fps );

		float minframetime = 1.0 / fps;

		if(( m_fRealTime - m_fOldRealTime ) < minframetime )
			return false;
	}

	m_fFrameTime = m_fRealTime - m_fOldRealTime;
	m_fOldRealTime = m_fRealTime;

	if(sys_timescale.GetFloat() != 1.0f)
	{
		m_fFrameTime *= sys_timescale.GetFloat();

		// don't allow really long or short frames
		m_fFrameTime = clamp( m_fFrameTime, MIN_FRAMETIME, MAX_FRAMETIME );
	}

	if(gpGlobals && gpGlobals->timescale != 1.0f)
	{
		m_fFrameTime *= gpGlobals->timescale;

		// don't allow really long or short frames
		m_fFrameTime = clamp( m_fFrameTime, MIN_FRAMETIME, MAX_FRAMETIME );
	}

	return true;
}

void CEngineGame::Reset(bool frameTimeOnly)
{
	m_fFrameTime = 0;

	if(!frameTimeOnly)
	{
		m_fRealTime = 0.0f;
		m_fOldRealTime = 0.0f;
	}

	if(gpGlobals)
	{
		gpGlobals->frametime = 0;

		if(!frameTimeOnly)
		{
			gpGlobals->curtime = 0;
			gpGlobals->realtime = 0;
			gpGlobals->timescale = 1.0f;
		}
	}
}