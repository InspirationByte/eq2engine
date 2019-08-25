//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: State of game
//////////////////////////////////////////////////////////////////////////////////

//
// TODO:
//		- More code refactoring from C-style to better C++ style
//

#include "state_game.h"
#include "CameraAnimator.h"
#include "materialsystem/MeshBuilder.h"

#include "eqParallelJobs.h"

#include "session_stuff.h"

#include "game_singleplayer.h"
#include "game_multiplayer.h"

#include "Rain.h"

#include "KeyBinding/InputCommandBinder.h"

#include "sys_in_console.h"

#include "sys_host.h"
#include "FontCache.h"

#include "DrvSynHUD.h"

#include "Shiny.h"

#include "director.h"

#include "world.h"

#include "input.h"

static CCameraAnimator	s_cameraAnimator;
CCameraAnimator*		g_pCameraAnimator = &s_cameraAnimator;

extern ConVar			net_server;

ConVar					g_pause("g_pause", "0");
int						g_nOldControlButtons	= 0;

Vector3D				g_freeLookAngles;

enum EReplayScheduleType
{
	REPLAY_SCHEDULE_NONE = 0,
	REPLAY_SCHEDULE_REPLAY,
	REPLAY_SCHEDULE_DIRECTOR,

	REPLAY_SCHEDULE_REPLAY_NORESTART,
};

enum EReplayMode
{
	REPLAY_MODE_NONE = 0,

	REPLAY_MODE_QUICK_REPLAY,
	REPLAY_MODE_STORED_REPLAY,
	REPLAY_MODE_DEMO,
};

void Game_ShutdownSession(bool restart = false);
void Game_InitializeSession();

void Game_QuickRestart(bool demo)
{
	if(EqStateMgr::GetCurrentStateType() != GAME_STATE_GAME)
		return;

	//SetCurrentState(NULL);

	if(!demo)
	{
		g_replayData->Stop();
		g_replayData->Clear();
	}

	g_State_Game->QuickRestart(demo);
}

void Game_OnPhysicsUpdate(float fDt, int iterNum);

DECLARE_CMD(restart, "Restarts game quickly", 0)
{
	Game_QuickRestart(false);
}

DECLARE_CMD(fastseek, "Seeks to the replay frame. (Visual mistakes are possible)", 0)
{
	if(g_pGameSession == NULL)
		return;

	int replayTo = 0;

	if(CMD_ARGC > 0)
		replayTo = atoi(CMD_ARGV(0).c_str());

	// restart if if we want earlier ticks
	if (replayTo < g_replayData->m_tick)
	{
		g_replayData->Stop();
		g_replayData->m_state = REPL_INIT_PLAYBACK;

		Game_QuickRestart(true);

		// load manually, not using loading screen
		g_State_Game->LoadGame();
	}
 
	int remainingFrames = replayTo - g_replayData->m_tick;

	const float frameRate = 1.0f / 60.0f; // TODO: use g_replayData->m_demoFrameRate

	while(remainingFrames > 0)
	{
		g_replayData->ForceUpdateReplayObjects();

		Game_OnPhysicsUpdate(frameRate, 0);

		remainingFrames--;
	}

	if(replayTo > 0)
		g_pCameraAnimator->Reset();

	// wait for any pending regions
	g_pGameWorld->m_level.WaitForThread();

}

void Game_InstantReplay(int replayTo)
{
	if(g_pGameSession == NULL)
		return;

	if(	replayTo == 0 && 
		g_replayData->m_state == REPL_PLAYING ||
		replayTo < g_replayData->m_tick)
	{
		// revert
		g_replayData->Stop();
		g_replayData->m_state = REPL_INIT_PLAYBACK;

		Game_QuickRestart(true);
	}

	if (!g_State_Game->IsGameRunning())
	{
		// load manually, not using loading screen
		g_State_Game->LoadGame();
	}

	int remainingFrames = replayTo - g_replayData->m_tick;

	const float frameRate = 1.0f / 60.0f;

	while(remainingFrames > 0)
	{
		// it's broken, but should be fine after fixing bugs
		g_pGameSession->Update(frameRate);
		remainingFrames -= g_pGameSession->GetPhysicsIterations();
	}

	if (replayTo > 0)
		g_pCameraAnimator->Reset();
}

DECLARE_CMD(instantreplay, "Does instant replay (slowly). You can fetch to frame if specified", 0)
{
	int replayTo = 0;
	if(CMD_ARGC > 0)
		replayTo = atoi(CMD_ARGV(0).c_str());

	Game_InstantReplay( replayTo );
}

void fnstart_variants(DkList<EqString>& list, const char* query)
{
	DKFINDDATA* findData = nullptr;
	char* fileName = (char*)g_fileSystem->FindFirst(LEVELS_PATH "*.lev", &findData, SP_MOD);

	if(fileName)
	{
		list.append(_Es(fileName).Path_Strip_Ext());

		while(fileName = (char*)g_fileSystem->FindNext(findData))
		{
			if(!g_fileSystem->FindIsDirectory(findData))
				list.append(_Es(fileName).Path_Strip_Ext());
		}

		g_fileSystem->FindClose(findData);
	}
}

DECLARE_CMD_VARIANTS(start, "start a game with specified mission or level name", fnstart_variants, 0)
{
	if(CMD_ARGC == 0)
		return;

	// unload game
	if(EqStateMgr::GetCurrentStateType() == GAME_STATE_GAME)
	{
		g_State_Game->UnloadGame();
	}

	// always set level name
	g_pGameWorld->SetLevelName( CMD_ARGV(0).c_str() );

	EqString scriptFileName(varargs("scripts/missions/%s.lua", CMD_ARGV(0).c_str()));
	if( g_fileSystem->FileExist(scriptFileName.c_str()) )
	{
		// try load mission script
		g_State_Game->SetMissionScript(CMD_ARGV(0).c_str());
	}
	else
		g_State_Game->SetMissionScript("defaultmission");

	EqStateMgr::SetCurrentState( g_states[GAME_STATE_GAME], true);
}

//------------------------------------------------------------------------------

void fnMaxplayersTest(ConVar* pVar,char const* pszOldValue)
{
	if(g_pGameSession != NULL && g_pGameSession->GetSessionType() == SESSION_NETWORK)
		Msg("maxplayers will be changed upon restart\n");
}

ConVar sv_maxplayers("maxplayers", "1", fnMaxplayersTest, "Maximum players allowed on the server\n");

//------------------------------------------------------------------------------
// Initilizes game session
//------------------------------------------------------------------------------

void Game_InitializeSession()
{
	Msg("-- InitializeSession --\n");

	if(!g_pGameSession)
	{
		if(net_server.GetBool())
			g_svclientInfo.maxPlayers = sv_maxplayers.GetInt();
		else if(g_svclientInfo.maxPlayers <= 1)
			net_server.SetBool(true);

		if( g_svclientInfo.maxPlayers > 1 )
		{
			CNetGameSession* netSession = new CNetGameSession();
			g_pGameSession = netSession;
		}
		else
			g_pGameSession = new CSingleGameSession();
	}

#ifndef __INTELLISENSE__

	OOLUA::set_global(GetLuaState(), "gameses", g_pGameSession);
	OOLUA::set_global(GetLuaState(), "gameHUD", g_pGameHUD);

#endif // __INTELLISENSE__

	if(g_replayData->m_state != REPL_INIT_PLAYBACK)
		g_replayData->Clear();

	g_pCameraAnimator->Reset();

	g_pGameSession->Init();

	// reset buttons
	ZeroInputControls();
}

void Game_ShutdownSession(bool restart)
{
	Msg("-- ShutdownSession%s --\n", restart ? "Restart" : "");
	g_parallelJobs->Wait();

	effectrenderer->RemoveAllEffects();

	if(g_pGameSession)
	{
		if(!restart)
			g_pGameSession->FinalizeMissionManager();

		g_pGameSession->Shutdown();
	}

	delete g_pGameSession;
	g_pGameSession = NULL;
}

DECLARE_CMD(spawn_car, "Spawns a car at crosshair", CV_CHEAT)
{
	if(CMD_ARGC == 0)
		return;

	Vector3D start = g_freeCamProps.position;
	Vector3D dir;
	AngleVectors(g_freeCamProps.angles, &dir);

	Vector3D end = start + dir*1000.0f;

	CollisionData_t coll;
	g_pPhysics->TestLine(start, end, coll, OBJECTCONTENTS_SOLID_GROUND);

	if(coll.fract < 1.0f)
	{
		CCar* newCar = g_pGameSession->CreateCar(CMD_ARGV(0).c_str());

		if(!newCar)
		{
			MsgError("Unknown car '%s'\n", CMD_ARGV(0).c_str());
			return;
		}

		newCar->SetOrigin(coll.position);
		newCar->Spawn();
		newCar->SetColorScheme(RandomInt(0,newCar->m_conf->numColors-1));
		newCar->AlignToGround();

		g_pGameWorld->AddObject( newCar );
		g_pGameSession->SetPlayerCar( newCar );
	}
}

DECLARE_CMD(shift_to_car, "Shift to car picked with ray", CV_CHEAT)
{
	if(g_replayData->m_state == REPL_PLAYING)
		return;

	CCar* pickedCar = Director_GetCarOnCrosshair(false);

	if(pickedCar != nullptr)
		g_pGameSession->SetPlayerCar( pickedCar );
}

ConVar eq_profiler_display("eqProfiler_display", "0", "Display profiler on screen");
extern ConVar g_pause;

//-------------------------------------------------------------------------------

CState_Game* g_State_Game = new CState_Game();

CState_Game::CState_Game() : CBaseStateHandler()
{
	m_uiLayout = nullptr;
	m_loadingScreen = nullptr;

	m_replayMode = REPLAY_MODE_NONE;
	m_isGameRunning = false;
	m_fade = 1.0f;
	m_isLoading = -1;
	m_missionScriptName = "defaultmission";
	m_scheduledQuickReplay = REPLAY_SCHEDULE_NONE;
	m_storedMissionStatus = MIS_STATUS_INGAME;

	m_gameMenuName = "Ingame";

	RegisterInputJoysticEssentials();
}

CState_Game::~CState_Game()
{

}

void CState_Game::LoadGame()
{


	while(m_isLoading != -1)
	{
		if (!DoLoadingFrame()) // actual level loading happened here
		{
			SetNextState(g_states[GAME_STATE_TITLESCREEN]);
			break;
		}
	}
}

void CState_Game::UnloadGame()
{
	if(!g_pPhysics)
		return;

	m_tunnelEfx = nullptr;

	if (m_isGameRunning)
	{
		// if it were recording, stop the replay and save last session
		if (g_replayData->m_state == REPL_RECORDING)
		{
			g_replayData->Stop();

			g_fileSystem->MakeDir(USERREPLAYS_PATH, SP_MOD);
			g_replayData->SaveToFile(USERREPLAYS_PATH "_lastSession.rdat");
		}
	}
	else
		MsgWarning("UnloadGame called while loading...\n");

	// renderer must be reset
	g_pShaderAPI->Reset(STATE_RESET_ALL);
	g_pShaderAPI->Apply();

	g_pGameHUD->Cleanup();

	m_storedMissionStatus = MIS_STATUS_INGAME;

	g_pGameWorld->Cleanup();
	Game_ShutdownSession();

	g_pPhysics->SceneShutdown();

	g_studioModelCache->ReleaseCache();

	g_sounds->Shutdown();

	delete g_pPhysics;
	g_pPhysics = NULL;

	m_isGameRunning = false;
	m_isLoading = -1;
}

bool CState_Game::DoLoadMission()
{
	EqString scriptFileName(varargs("scripts/missions/%s.lua", m_missionScriptName.c_str()));

	// then we load custom script
	if (!EqLua::LuaBinding_LoadAndDoFile(GetLuaState(), scriptFileName.c_str(), "MissionLoader"))
	{
		MsgError("mission script init error:\n\n%s\n", OOLUA::get_last_error(GetLuaState()).c_str());

		// okay, try reinitialize with default mission script
		if (!EqLua::LuaBinding_LoadAndDoFile(GetLuaState(), "scripts/missions/defaultmission.lua", "MissionLoader"))
		{
			MsgError("default mission script init error:\n\n%s\n", OOLUA::get_last_error(GetLuaState()).c_str());
			return false;
		}

		m_missionScriptName = "defaultmission";
	}

	return true;
}

bool CState_Game::SetMissionScript( const char* name )
{
	m_missionScriptName = name;

	EqString scriptFileName(varargs("scripts/missions/%s.lua", m_missionScriptName.c_str()));

	return g_fileSystem->FileExist(scriptFileName.c_str(), SP_MOD);
}

const char* CState_Game::GetMissionScriptName() const
{
	return m_missionScriptName.c_str();
}

void CState_Game::StopStreams()
{
	g_sounds->StopAllSounds();
	g_sounds->SetPaused(true);
}

void CState_Game::QuickRestart(bool replay)
{
	StopStreams();

	m_isGameRunning = false;
	m_exitGame = false;
	m_showMenu = false;

	m_fade = 1.0f;

	g_pause.SetBool(false);
	
	// renderer must be reset
	g_pShaderAPI->Reset(STATE_RESET_ALL);
	g_pShaderAPI->Apply();

	g_pGameHUD->Cleanup();
	g_pGameWorld->Cleanup(false);

	// store mission status if we're going to replay
	m_storedMissionStatus = g_pGameSession->GetMissionStatus();

	if(m_replayMode != REPLAY_MODE_STORED_REPLAY)
		m_replayMode = replay ? REPLAY_MODE_QUICK_REPLAY : REPLAY_MODE_NONE;

	Game_ShutdownSession(true);

	// loader to the phase 5 (world loading)
	m_isLoading = 5;
}

void CState_Game::OnEnterSelection( bool isFinal )
{
	if(isFinal)
	{
		m_fade = 0.0f;
		m_exitGame = true;
	}
}

void CState_Game::SetupMenuStack( const char* name )
{
	OOLUA::Table menuStacks;
	if(!OOLUA::get_global(GetLuaState(), "CurrentGameMenuTable", menuStacks))
	{
		MsgError("Failed to get CurrentGameMenuTable table, No game menus available!\n", name);
		return;
	}

	OOLUA::Table menuStackObject;
	if(!menuStacks.safe_at(name, menuStackObject))
	{
		MsgWarning("Menu '%s' not found in CurrentGameMenuTable table!\n", name);
		return;
	}

	SetMenuObject(menuStackObject);
}

void CState_Game::OnMenuCommand( const char* command )
{
	int missionStatus = g_pGameSession->GetMissionStatus();

	if(!stricmp(command, "continue"))
	{
		m_showMenu = false;
	}
	else if(!stricmp(command, "showMap"))
	{
		Msg("TODO: show the map\n");
	}
	else if(!stricmp(command, "restartGame"))
	{
		m_exitGame = false;
		m_fade = 0.0f;

		// fail the game
		if(missionStatus == MIS_STATUS_INGAME)
			g_pGameSession->SignalMissionStatus(MIS_STATUS_FAILED, 0.0f);

		m_scheduledRestart = true;
	}
	else if(!stricmp(command, "quickReplay") || !stricmp(command, "goToDirector"))
	{
		m_exitGame = false;
		m_fade = 0.0f;

		// fail the game
		if(missionStatus == MIS_STATUS_INGAME)
			g_pGameSession->SignalMissionStatus(MIS_STATUS_FAILED, 0.0f);

		m_scheduledQuickReplay = !stricmp(command, "goToDirector") ? REPLAY_SCHEDULE_DIRECTOR : REPLAY_SCHEDULE_REPLAY;
	}
}

// when changed to this state
// @from - used to transfer data
void CState_Game::OnEnter( CBaseStateHandler* from )
{
	if(m_isGameRunning)
		return;

	m_isLoading = 0;
	m_loadingError = false;
	m_exitGame = false;
	m_showMenu = false;

	m_scheduledRestart = false;
	m_scheduledQuickReplay = REPLAY_SCHEDULE_NONE;

	m_fade = 1.0f;

	m_menuTitleStr = LocalizedString("#MENU_GAME_TITLE_PAUSE");

	m_tunnelEfx = soundsystem->FindEffect("tunnel_reverb");

	// init hud layout
	m_uiLayout = equi::Manager->CreateElement("Panel");

	kvkeybase_t uiKvs;

	if (KV_LoadFromFile("resources/ui_pausemenu.res", SP_MOD, &uiKvs))
		m_uiLayout->InitFromKeyValues(&uiKvs);

	m_menuDummy = m_uiLayout->FindChild("Menu");

	if (!m_menuDummy)
	{
		m_menuDummy = equi::Manager->CreateElement("HudElement");
		m_menuDummy->SetPosition(0);
		m_menuDummy->SetSize(IVector2D(640, 480));
		m_menuDummy->SetScaling(equi::UI_SCALING_ASPECT_H);
		m_menuDummy->SetTextAlignment(TEXT_ALIGN_HCENTER);

		m_uiLayout->AddChild(m_menuDummy);
	}

	m_loadingScreen = equi::Manager->CreateElement("Panel");

	kvkeybase_t loadingKvs;

	if (KV_LoadFromFile("resources/loadingscreen.res", SP_MOD, &loadingKvs))
		m_loadingScreen->InitFromKeyValues(&loadingKvs);
}

bool CState_Game::DoLoadingFrame()
{
	if (m_isLoading == 0)
		UnloadGame();

	// FIXME: use queue instead?
	switch (m_isLoading)
	{
		case 1:
		{
			g_sounds->Init(EQ_DRVSYN_DEFAULT_SOUND_DISTANCE);
			g_sounds->SetPaused(true);
			break;
		}
		case 2:
		{
			PrecacheStudioModel("models/error.egf");
			PrecacheScriptSound("menu.back");
			PrecacheScriptSound("menu.roll");
			break;
		}
		case 3:
		{
			g_pPhysics = new CPhysicsEngine();
			g_pPhysics->SceneInit();
			break;
		}
		case 4:
		{
			m_loadingError = !DoLoadMission();
			break;
		}
		case 5:
		{
			m_loadingError = !g_pGameWorld->LoadLevel();
			break;
		}
		case 6:
		{
			g_pGameWorld->Init();
			break;
		}
		case 7:	// FINAL
		{
			g_pGameHUD->Init();
			Game_InitializeSession();
			g_pause.SetBool(false);

			// loading completed
			m_isLoading = -1;

			return true;
		}
	}

	m_isLoading++;

	return !m_loadingError;
}

// when the state changes to something
// @to - used to transfer data
void CState_Game::OnLeave( CBaseStateHandler* to )
{
	m_replayMode = REPLAY_MODE_NONE;

	delete m_uiLayout;
	m_uiLayout = m_menuDummy = nullptr;

	delete m_loadingScreen;
	m_loadingScreen = nullptr;

	if(!g_pGameSession)
		return;

	UnloadGame();
}

int CState_Game::GetPauseMode() const
{
	if(!g_pGameSession)
		return PAUSEMODE_PAUSE;

	if(g_pGameSession->IsGameDone())
	{
		return g_pGameSession->GetMissionStatus() == MIS_STATUS_SUCCESS ? PAUSEMODE_COMPLETE : PAUSEMODE_GAMEOVER;
	}

	if((g_pause.GetBool() || m_showMenu) && g_pGameSession->GetSessionType() == SESSION_SINGLE)
		return PAUSEMODE_PAUSE;

	return PAUSEMODE_NONE;
}

void CState_Game::SetPauseState( bool state )
{
	// can't set to pause when in demo mode
	if(m_replayMode == REPLAY_MODE_DEMO)
		return;

	if(!m_exitGame)
		m_showMenu = state;

	if(m_showMenu)
	{
		m_selection = 0;

		int missionStatus = (m_replayMode == REPLAY_MODE_QUICK_REPLAY) ? m_storedMissionStatus : g_pGameSession->GetMissionStatus();

		if(m_replayMode <= REPLAY_MODE_QUICK_REPLAY)
		{
			if(missionStatus == MIS_STATUS_SUCCESS)
			{
				SetupMenuStack("MissionSuccess");
			}
			else if(missionStatus == MIS_STATUS_FAILED)
			{
				SetupMenuStack("MissionFailed");
			}
			else
				SetupMenuStack( m_gameMenuName.c_str() );
		}
		else
			SetupMenuStack( m_gameMenuName.c_str() );
	}

	// update pause state
	UpdatePauseState();
}

bool CState_Game::StartReplay( const char* path, bool demoMode )
{
	if(g_replayData->LoadFromFile( path ))
	{
		EqStateMgr::ChangeState( this );
		m_scheduledQuickReplay = REPLAY_SCHEDULE_REPLAY_NORESTART;

		m_replayMode = demoMode ? REPLAY_MODE_DEMO : REPLAY_MODE_STORED_REPLAY;

		return true;
	}

	return false;
}

void CState_Game::DrawLoadingScreen()
{
	const IVector2D& screenSize = g_pHost->GetWindowSize();

	materials->Setup2D(screenSize.x, screenSize.y);

	m_loadingScreen->SetSize(screenSize);
	m_loadingScreen->Render();

	/*
	IEqFont* font = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD+TEXT_STYLE_ITALIC);

	const wchar_t* loadingStr = LocalizedString("#GAME_IS_LOADING");

	eqFontStyleParam_t param;
	param.styleFlag |= TEXT_STYLE_SHADOW;
	param.scale = 40.0f;

	float offs = sinf(g_pHost->GetCurTime()*0.5f) * 50.0f;

	font->RenderText(loadingStr, Vector2D(100 + offs,screenSize.y - 100), param);

	param.scale = 90.0f;
	param.styleFlag &= ~TEXT_STYLE_SHADOW;
	param.textColor.w = 0.35f;

	offs *= 1.5f;

	font->RenderText(loadingStr, Vector2D(100 + offs,screenSize.y - 100), param);
	*/
	
}

void CState_Game::OnLoadingDone()
{
	// set menus
	if (m_replayMode != REPLAY_MODE_STORED_REPLAY)
	{
		int sessionType = g_pGameSession->GetSessionType();

		m_gameMenuName = "Ingame";
	}
	else
		m_gameMenuName = "Replay";

	m_isGameRunning = true;

	g_sounds->SetPaused(false);
	g_sounds->Set2DChannelsVolume(CHAN_STREAM, 1.0f);

	g_pGameSession->OnLoadingDone();
}

//-------------------------------------------------------------------------------
// Game frame step along with rendering
//-------------------------------------------------------------------------------
bool CState_Game::Update( float fDt )
{
	if(m_loadingError)
		return false;

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	if(!m_isGameRunning)
	{
		DrawLoadingScreen();
		
		if (m_isLoading != -1)
		{
			if(!DoLoadingFrame()) // actual level loading happened here
				SetNextState(g_states[GAME_STATE_TITLESCREEN]);

			return true;
		}

		// wait for world and materials to be loaded
		if (g_pGameWorld->m_level.IsWorkDone() && materials->GetLoadingQueue() == 0)
			OnLoadingDone();

		return true;
	}

	float fGameFrameDt = fDt;

	bool replayDirectorMode = Director_IsActive();

	bool gameDone = g_pGameSession->IsGameDone(false);
	bool gameDoneTimedOut = g_pGameSession->IsGameDone();

	// force end this game
	if(gameDone && m_showMenu && !gameDoneTimedOut)
	{
		g_pGameSession->SignalMissionStatus(g_pGameSession->GetMissionStatus(), -1.0f);
		m_showMenu = false;
	}

	gameDoneTimedOut = g_pGameSession->IsGameDone();

	if(gameDoneTimedOut && !m_exitGame)
	{
		if(m_replayMode == REPLAY_MODE_DEMO)
		{
			m_exitGame = true;
			m_fade = 0.0f;
			SetNextState(g_states[GAME_STATE_TITLESCREEN]);
		}
		else if(!m_showMenu && !replayDirectorMode)
		{
			// set other menu
			SetPauseState(!m_scheduledRestart && !m_scheduledQuickReplay);
		}
	}

	// update pause state
	if( !UpdatePauseState() )
		fGameFrameDt = 0.0f;

	// reset buttons
	if(m_showMenu)
		ZeroInputControls();

	//
	// Update, Render, etc
	//
	DoGameFrame( fGameFrameDt );

	if(m_replayMode == REPLAY_MODE_DEMO)
	{
		materials->Setup2D(screenSize.x,screenSize.y);

		IEqFont* font = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD | TEXT_STYLE_ITALIC);

		eqFontStyleParam_t fontParam;
		fontParam.styleFlag |= TEXT_STYLE_SHADOW;
		fontParam.align = TEXT_ALIGN_HCENTER;
		fontParam.textColor = ColorRGBA(fabs(sinf(g_pHost->GetCurTime()*2.0f)),0.0f,0.0f,1.0f);
		fontParam.scale = 30.0f;

		font->RenderText("Demo", Vector2D(screenSize.x/2,screenSize.y - 100), fontParam);
	}

	DrawMenu(fDt);

	if(m_exitGame || m_scheduledRestart || m_scheduledQuickReplay)
	{
		if(m_scheduledQuickReplay == REPLAY_SCHEDULE_REPLAY_NORESTART)
		{
			m_scheduledQuickReplay = REPLAY_SCHEDULE_NONE;
			return !m_exitGame;
		}

		ColorRGBA blockCol(0.0,0.0,0.0,m_fade);

		Vertex2D_t tmprect1[] = { MAKETEXQUAD(0, 0,screenSize.x, screenSize.y, 0) };

		materials->Setup2D(screenSize.x,screenSize.y);

		BlendStateParam_t blending;
		blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
		blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect1,elementsOf(tmprect1), NULL, blockCol, &blending);

		m_fade += fDt;

		if(m_fade >= 1.0f)
		{
			if(m_scheduledRestart)
				Game_QuickRestart(false);

			if(m_scheduledQuickReplay > 0)
			{
				Game_InstantReplay(0);
				Director_Enable(m_scheduledQuickReplay==REPLAY_SCHEDULE_DIRECTOR);
			}

			m_scheduledRestart = false;
			m_scheduledQuickReplay = REPLAY_SCHEDULE_NONE;

			return !m_exitGame;
		}
	}
	else
	{
		// only fade sound
		if( m_fade > 0.0f )
		{
			m_fade -= fDt*5.0f;
		}
	}

	return true;
}

bool CState_Game::UpdatePauseState()
{
	int pauseMode = GetPauseMode();

	if( pauseMode > 0 )
	{
		ISoundPlayable* musicChan = soundsystem->GetStaticStreamChannel(CHAN_STREAM);
		if(musicChan)
			musicChan->Pause();

		ISoundPlayable* voiceChan = soundsystem->GetStaticStreamChannel(CHAN_VOICE);
		if(voiceChan)
			voiceChan->Pause();
	}
	else if(m_pauseState)
	{
		ISoundPlayable* musicChan = soundsystem->GetStaticStreamChannel(CHAN_STREAM);
		if(musicChan && musicChan->GetState() != SOUND_STATE_PLAYING)
			musicChan->Play();

		ISoundPlayable* voiceChan = soundsystem->GetStaticStreamChannel(CHAN_VOICE);
		if(voiceChan && voiceChan->GetState() != SOUND_STATE_PLAYING)
			voiceChan->Play();
	}

	soundsystem->SetPauseState( pauseMode > 0);
	m_pauseState = (pauseMode > 0);

	return (pauseMode == 0);
}

void CState_Game::DrawMenu( float fDt )
{
	if( !m_showMenu )
		return;

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	materials->Setup2D(screenSize.x,screenSize.y);

	m_uiLayout->SetSize(screenSize);
	m_uiLayout->Render();

	IVector2D halfScreen(screenSize.x/2, screenSize.y/2);

	IEqFont* font = m_menuDummy->GetFont();

	eqFontStyleParam_t fontParam;
	fontParam.align = m_menuDummy->GetTextAlignment();
	fontParam.styleFlag |= TEXT_STYLE_SHADOW;
	fontParam.textColor = color4_white;
	fontParam.scale = m_menuDummy->GetFontScale() * m_menuDummy->CalcScaling();

	{
		lua_State* state = GetLuaState();

		EqLua::LuaStackGuard g(state);

		int numElems = 0;

		oolua_ipairs(m_menuElems)
			numElems++;
		oolua_ipairs_end()

		int menuPosY = halfScreen.y - numElems*font->GetLineHeight(fontParam)*0.5f;

		Vector2D mTextPos(halfScreen.x, menuPosY);

		fontParam.textColor = ColorRGBA(0.7f,0.7f,0.7f,1.0f);
		font->RenderText(m_menuTitleStr.c_str(), mTextPos, fontParam);

		oolua_ipairs(m_menuElems)
			int idx = _i_index_-1;

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			const wchar_t* token = GetMenuItemString(elem);

			if(m_selection == idx)
				fontParam.textColor = ColorRGBA(1,0.7f,0.0f,1.0f);
			else
				fontParam.textColor = ColorRGBA(1,1,1,1.0f);

			Vector2D eTextPos(halfScreen.x, menuPosY+_i_index_*font->GetLineHeight(fontParam));

			font->RenderText(token ? token : L"No token", eTextPos, fontParam);
		oolua_ipairs_end()

	}
}

CCar* CState_Game::GetViewCar() const
{
	CGameObject* viewObj = g_pGameSession ? g_pGameSession->GetViewObject() : NULL;

	CCar* viewedCar = IsCar(viewObj) ? (CCar*)viewObj : nullptr;
	/*
	if(g_replayData->m_state == REPL_PLAYING && g_replayData->m_cameras.numElem() > 0)
	{
		// replay controls camera
		replayCamera_t* replCamera = g_replayData->GetCurrentCamera();

		if(replCamera)
		{
			CCar* cameraCar = g_replayData->GetCarByReplayIndex( replCamera->targetIdx );

			// only if it's valid
			if(g_pGameWorld->IsValidObject(cameraCar))
				viewObject = cameraCar;
		}
	}*/

	return viewedCar;
}

Vector3D CState_Game::GetViewVelocity() const
{
	CCar* viewedCar = GetViewCar();

	Vector3D cam_velocity = vec3_zero;

	// animate the camera if car is present
	if( viewedCar && g_pCameraAnimator->GetMode() <= CAM_MODE_INCAR && !Director_FreeCameraActive() )
		cam_velocity = viewedCar->GetVelocity();

	return cam_velocity;
}

void GRJob_DrawEffects(void* data, int i)
{
	float fDt = *(float*)data;
	effectrenderer->DrawEffects( fDt );
}

void CState_Game::RenderMainView3D( float fDt )
{
	static float jobFrametime = fDt;
	jobFrametime = fDt;

	// post draw effects
	g_parallelJobs->AddJob(GRJob_DrawEffects, &jobFrametime);
	g_parallelJobs->Submit();

	// rebuild view
	const IVector2D& screenSize = g_pHost->GetWindowSize();
	g_pGameWorld->BuildViewMatrices(screenSize.x,screenSize.y, 0);

	// frustum update
	PROFILE_CODE(g_pGameWorld->UpdateOccludingFrustum());

	// render world
	PROFILE_CODE(g_pGameWorld->Draw( 0 ));

	// Render 3D parts of HUD
	g_pGameWorld->BuildViewMatrices(screenSize.x, screenSize.y, 0);
	g_pGameHUD->Render3D(fDt);
}

void CState_Game::RenderMainView2D( float fDt )
{
	const IVector2D& screenSize = g_pHost->GetWindowSize();

	// draw HUD
	g_pGameHUD->Render( fDt, screenSize );

	Director_Draw( fDt );

	if(!g_pause.GetBool())
		PROFILE_UPDATE();

	if(eq_profiler_display.GetBool())
	{
		EqString profilerStr = PROFILE_GET_TREE_STRING().c_str();

		materials->Setup2D(screenSize.x,screenSize.y);

		eqFontStyleParam_t params;
		params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;

		static IEqFont* consoleFont = g_fontCache->GetFont("console", 16);

		consoleFont->RenderText(profilerStr.c_str(), Vector2D(45), params);
	}
}

void CState_Game::DoGameFrame(float fDt)
{
	// Update game
	PROFILE_FUNC();

	// step the game simulation
	g_pGameSession->UpdateLocalControls( g_nClientButtons, g_joySteeringValue, g_joyAccelBrakeValue );
	g_pGameSession->Update(fDt);

	float cameraFrameTimes = fDt;

	// director camera is updated in real time
	if (Director_IsActive())
	{
		cameraFrameTimes = g_pHost->GetSysFrameTime();
		const float minCameraFrameTime = (1.0f / 60.0f);
		cameraFrameTimes = max(cameraFrameTimes, minCameraFrameTime);
	}

	DoCameraUpdates(cameraFrameTimes);

	// render all
	RenderMainView3D( fDt );
	RenderMainView2D( fDt );

	g_nOldControlButtons = g_nClientButtons;
}

ConVar g_alwaysfreelook("g_alwaysfreelook", "0", nullptr, CV_ARCHIVE);

bool GotFreeLook()
{
	return (g_nClientButtons & IN_FREELOOK) || g_alwaysfreelook.GetBool();
}

void CState_Game::DoCameraUpdates( float fDt )
{
	int camControls = g_nClientButtons;

	if (g_replayData->m_state == REPL_PLAYING)
		camControls &= ~IN_CHANGECAM;

	CViewParams* curView = g_pGameWorld->GetView();

	if( Director_FreeCameraActive() )
	{
		float cameraFrameTimes = g_pHost->GetSysFrameTime();
		const float minCameraFrameTime = (1.0f / 60.0f);
		cameraFrameTimes = max(cameraFrameTimes, minCameraFrameTime);

		Director_UpdateFreeCamera(cameraFrameTimes);

		curView->SetOrigin(g_freeCamProps.position);
		curView->SetAngles(g_freeCamProps.angles);
		curView->SetFOV(g_freeCamProps.fov);

		g_pCameraAnimator->SetOrigin(g_freeCamProps.position);
	}
	else
	{
		if(!g_pCameraAnimator->IsScripted())
		{
			CGameObject* viewObject = g_pGameSession->GetViewObject();

			if(!g_pGameWorld->IsValidObject(viewObject))
			{
				g_pGameSession->SetViewObject(NULL);
				viewObject = g_pGameSession->GetViewObject();
			}

			if (!GotFreeLook())
				g_freeLookAngles = vec3_zero;

			Vector3D lookAngles = g_freeLookAngles;

			// apply cameras from replays
			if(g_replayData->m_state == REPL_PLAYING)
			{
				// replay controls camera
				replayCamera_t* replCamera = g_replayData->GetCurrentCamera();

				if(replCamera)
				{
					CCar* cameraCar = g_replayData->GetCarByReplayIndex( replCamera->targetIdx );

					// only if it's valid
					if(g_pGameWorld->IsValidObject(cameraCar))
					{
						viewObject = cameraCar;

						g_pCameraAnimator->SetMode( (ECameraMode)replCamera->type );
						g_pCameraAnimator->SetFOV(replCamera->fov);

						g_pCameraAnimator->SetOrigin( replCamera->origin );

						if (!GotFreeLook())
							lookAngles = replCamera->rotation;
					}
				}
			}

			g_pCameraAnimator->SetAngles(lookAngles);

			/*
			if( viewObject && viewObject->GetPhysicsBody() )
			{
				DkList<CollisionPairData_t>& pairs = viewObject->GetPhysicsBody()->m_collisionList;

				if(pairs.numElem())
				{
					float powScale = 1.0f / (float)pairs.numElem();

					float invMassA = viewObject->GetPhysicsBody()->GetInvMass();

					for(int i = 0; i < pairs.numElem(); i++)
					{
						float appliedImpulse = fabs(pairs[i].appliedImpulse) * invMassA * powScale;

						if(pairs[i].impactVelocity > 1.5f)
							g_pCameraAnimator->ViewShake(min(appliedImpulse*0.5f, 5.0f), min(appliedImpulse*0.5f, 0.25f));
					}
				}
			}
			*/

			g_pCameraAnimator->Update(fDt, camControls, viewObject);
		}

		// set final result to the world renderer
		g_pGameWorld->SetView( g_pCameraAnimator->GetComputedView() );

		// as always
		g_freeCamProps.position = curView->GetOrigin();
		g_freeCamProps.angles = curView->GetAngles();
		g_freeCamProps.fov = DIRECTOR_DEFAULT_CAMERA_FOV;
	}

	// also update various systems
	Vector3D viewVelocity = GetViewVelocity();

	Vector3D f,r,u;
	AngleVectors(curView->GetAngles(), &f, &r, &u);

	sndEffect_t* sndEffect = nullptr;
	{
		const float TUNNEL_SOUND_TRACE_UPDIST = 15.0f;
		const float TUNNEL_SOUND_TRACE_FORWARD = 15.0f;

		Vector3D tunnelTracePos1 = curView->GetOrigin();
		Vector3D tunnelTracePos2 = curView->GetOrigin() + f * TUNNEL_SOUND_TRACE_FORWARD;

		CollisionData_t coll1;
		CollisionData_t coll2;

		g_pPhysics->TestLine(tunnelTracePos1, tunnelTracePos1 + vec3_up * TUNNEL_SOUND_TRACE_UPDIST, coll1, OBJECTCONTENTS_SOLID_OBJECTS);
		g_pPhysics->TestLine(tunnelTracePos1, tunnelTracePos2, coll2, OBJECTCONTENTS_SOLID_OBJECTS);

		Vector3D middlePos = lerp(tunnelTracePos1, tunnelTracePos2, coll2.fract);
		g_pPhysics->TestLine(middlePos, middlePos + vec3_up * TUNNEL_SOUND_TRACE_UPDIST, coll2, OBJECTCONTENTS_SOLID_OBJECTS);

		// set tunnel sound effect
		if (coll1.fract < 1.0f && coll2.fract < 1.0f)
			sndEffect = m_tunnelEfx;
	}

	soundsystem->SetListener(curView->GetOrigin(), f, u, viewVelocity, sndEffect);

	effectrenderer->SetViewSortPosition(curView->GetOrigin());
	g_pRainEmitter->SetViewVelocity(viewVelocity);
}

void CState_Game::HandleKeyPress( int key, bool down )
{
	if(!m_isGameRunning)
		return;

	if( m_replayMode == REPLAY_MODE_DEMO )
	{
		if(m_fade <= 0.0f)
		{
			m_fade = 0.0f;
			m_exitGame = true;
			SetNextState(g_states[GAME_STATE_TITLESCREEN]);
		}

		return;
	}

	if ((key == KEY_ESCAPE || key == KEY_JOY_START || (key == KEY_JOY_Y && m_showMenu && IsCanPopMenu())) && down)
	{
		if (m_showMenu && !IsMenuActive())
			return;

		if (IsMenuActive() && IsCanPopMenu())
		{
			EmitSound_t es("menu.back");
			g_sounds->Emit2DSound(&es);

			PopMenu();

			return;
		}

		SetPauseState(!m_showMenu);
	}

	if(IsMenuActive())
	{
		if(!down)
			return;

		if(key == KEY_ENTER || key == KEY_JOY_A)
		{
			if(PreEnterSelection())
				EnterSelection();
		}
		else if(key == KEY_LEFT || key == KEY_RIGHT || key == KEY_JOY_DPAD_LEFT || key == KEY_JOY_DPAD_RIGHT)
		{
			int direction = (key == KEY_LEFT || key == KEY_JOY_DPAD_LEFT) ? -1 : 1;

			if(ChangeSelection(direction))
			{
				EmitSound_t es("menu.roll");
				g_sounds->Emit2DSound( &es );
			}
		}
		else if(key == KEY_UP || key == KEY_JOY_DPAD_UP)
		{
redecrement:

			m_selection--;

			if(m_selection < 0)
			{
				int nItem = 0;
				m_selection = m_numElems-1;
			}

			//if(pItem->type == MIT_SPACER)
			//	goto redecrement;

			EmitSound_t ep("menu.roll");
			g_sounds->Emit2DSound(&ep);
		}
		else if(key == KEY_DOWN || key == KEY_JOY_DPAD_DOWN)
		{
reincrement:
			m_selection++;

			if(m_selection > m_numElems-1)
				m_selection = 0;

			//if(pItem->type == MIT_SPACER)
			//	goto reincrement;

			EmitSound_t ep("menu.roll");
			g_sounds->Emit2DSound(&ep);
		}
	}
	else
	{
		Director_KeyPress(key, down);

		//if(!MenuKeys( key, down ))
			g_inputCommandBinder->OnKeyEvent( key, down );
	}
}

void CState_Game::GetMouseCursorProperties(bool &visible, bool& centered)
{
	visible = m_showMenu || Director_IsActive() && !Director_FreeCameraActive() && !g_pause.GetBool();
	centered = (Director_FreeCameraActive() || GotFreeLook()) && !visible;
}

void CState_Game::HandleMouseMove( int x,  int y, float deltaX, float deltaY )
{
	if(!m_isGameRunning)
		return;

	if( IsMenuActive() )
	{
		// perform a hit test
		const IVector2D& screenSize = g_pHost->GetWindowSize();
		IVector2D halfScreen(screenSize.x / 2, screenSize.y / 2);

		IEqFont* font = m_menuDummy->GetFont();

		eqFontStyleParam_t fontParam;
		fontParam.align = m_menuDummy->GetTextAlignment();
		fontParam.styleFlag |= TEXT_STYLE_SHADOW;
		fontParam.textColor = color4_white;
		fontParam.scale = m_menuDummy->GetFontScale() * m_menuDummy->CalcScaling();

		{
			EqLua::LuaStackGuard g(GetLuaState());

			int numElems = 0;

			oolua_ipairs(m_menuElems)
				numElems++;
			oolua_ipairs_end()

			int menuPosY = halfScreen.y - numElems * font->GetLineHeight(fontParam)*0.5f;

			Vector2D mTextPos(halfScreen.x, menuPosY);

			oolua_ipairs(m_menuElems)
				int idx = _i_index_ - 1;

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			float lineWidth = 400;
			float lineHeight = font->GetLineHeight(fontParam);

			Vector2D eTextPos(halfScreen.x, menuPosY + _i_index_ * font->GetLineHeight(fontParam));

			Rectangle_t rect(eTextPos - Vector2D(lineWidth, lineHeight), eTextPos + Vector2D(lineWidth, 0));

			if (rect.IsInRectangle(Vector2D(x, y)))
				Event_SelectMenuItem(idx);

			oolua_ipairs_end()
		}

		return;
	}

	if(!g_consoleInput->IsVisible())
	{
		Director_MouseMove(x,y,deltaX,deltaY);

		if(GotFreeLook())
		{
			g_freeLookAngles -= Vector3D(deltaY,deltaX,0);
			g_freeLookAngles.x = clamp(g_freeLookAngles.x, -89.0f, 0.0f);
		}
		else
			g_freeLookAngles = vec3_zero;
	}
}

void CState_Game::Event_SelectMenuItem(int index)
{
	if (!IsMenuActive())
		return;

	if (index > m_numElems - 1)
		return;

	if (m_selection == index)
		return;

	m_selection = index;

	EmitSound_t ep("menu.roll");
	g_sounds->Emit2DSound(&ep);
}

void CState_Game::HandleMouseClick( int x, int y, int buttons, bool down )
{
	if(!m_isGameRunning)
		return;

	if(IsMenuActive())
	{
		if (buttons == MOU_B1 && !down)
		{
			if (PreEnterSelection())
				EnterSelection();
		}

		return;
	}

	if(buttons == MOU_B2)
	{
		g_freeCamProps.zAxisMove = down;
	}

	g_inputCommandBinder->OnMouseEvent(buttons, down);
}

void CState_Game::HandleMouseWheel(int x,int y,int scroll)
{
	if(!m_isGameRunning)
		return;

	g_freeCamProps.fov -= scroll;
}

void CState_Game::HandleJoyAxis( short axis, short value )
{

}
