//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: State of game
//////////////////////////////////////////////////////////////////////////////////

//
// TODO:
//		- General code refactoring from C-style to better C++ style
//		- Move replay director to separate source files and as a state
//		- Finally make CDrvSynHUDManager useful in native and Lua code
//		- Cleanup from unused/junk/bad code
//		- Make CState_Game state object initialized from here as g_pState_Game to make it more accessible
//

#include "state_game.h"
#include "CameraAnimator.h"

#include "session_stuff.h"
#include "Rain.h"

#include "KeyBinding/Keys.h"

#include "sys_console.h"

#include "system.h"
#include "FontCache.h"

#include "DrvSynHUD.h"

#include "Shiny.h"

static CCameraAnimator	s_cameraAnimator;
CCameraAnimator*		g_pCameraAnimator = &s_cameraAnimator;

CGameSession*			g_pGameSession = NULL;

extern ConVar			net_server;

ConVar					g_pause("g_pause", "0", "Pauses the game");

ConVar					g_freecam("g_freecam", "0", "Enable free camera");
ConVar					g_freecam_speed("g_freecam_speed", "10", "free camera speed", CV_ARCHIVE);

ConVar					g_mouse_sens("g_mouse_sens", "1.0", "mouse sensitivity", CV_ARCHIVE);

ConVar					g_director("g_director", "0", "Enable director mode when replay playing");

int						g_nOldControlButtons	= 0;
int						g_nDirectorCameraType	= CAM_MODE_TRIPOD_ZOOM;

#define					DIRECTOR_DEFAULT_CAMERA_FOV	 (60.0f) // old: 52

struct freeCameraProps_t
{
	freeCameraProps_t()
	{
		fov = DIRECTOR_DEFAULT_CAMERA_FOV;
		position = vec3_zero;
		angles = vec3_zero;
		velocity = vec3_zero;
	}

	Vector3D	position;
	Vector3D	angles;
	Vector3D	velocity;
	float		fov;
} g_freeCamProps;


void Game_ShutdownSession();
void Game_InitializeSession();

void Game_QuickRestart(bool demo)
{
	if(GetCurrentStateType() != GAME_STATE_GAME)
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

DECLARE_CMD(fastseek, "Does instant replay. You can fetch to frame if specified", 0)
{
	if(g_pGameSession == NULL)
		return;

	int replayTo = 0;
	if(CMD_ARGC > 0)
		replayTo = atoi(CMD_ARGV(0).c_str());

	g_replayData->Stop();
	g_replayData->m_tick = 0;
	g_replayData->m_state = REPL_INIT_PLAYBACK;

	Game_QuickRestart(true);

	const float frameRate = 1.0f / 60.0f;

	while(replayTo > 0)
	{
		// TODO: use g_replayData->m_demoFrameRate
		Game_OnPhysicsUpdate(frameRate, 0);

		replayTo--;
	}

	g_pCameraAnimator->Reset();
}

void Game_InstantReplay(int replayTo)
{
	if(g_pGameSession == NULL)
		return;

	if(replayTo == 0 && g_replayData->m_state == REPL_PLAYING)
	{
		g_replayData->Stop();
		g_replayData->m_tick = 0;
		g_replayData->m_state = REPL_INIT_PLAYBACK;

		Game_QuickRestart(true);
	}
	else
	{
		if(replayTo >= g_replayData->m_tick)
		{
			replayTo -= g_replayData->m_tick;
		}
		else
		{
			g_replayData->Stop();
			g_replayData->m_tick = 0;
			g_replayData->m_state = REPL_INIT_PLAYBACK;

			Game_QuickRestart(true);
		}
	}

	g_pGameWorld->m_level.WaitForThread();

	g_pCameraAnimator->Reset();

	const float frameRate = 1.0f / 60.0f;

	while(replayTo > 0)
	{
		// TODO: use g_replayData->m_demoFrameRate
		g_pPhysics->Simulate(frameRate, PHYSICS_ITERATION_COUNT, Game_OnPhysicsUpdate);

		replayTo--;
		replayTo--;
	}
}

DECLARE_CMD(instantreplay, "Does instant replay (slowly). You can fetch to frame if specified", 0)
{
	int replayTo = 0;
	if(CMD_ARGC > 0)
		replayTo = atoi(CMD_ARGV(0).c_str());

	Game_InstantReplay( replayTo );
}

DECLARE_CMD(start, "loads a level or starts mission", 0)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: start <name> - starts game with specified level or mission\n");
		return;
	}

	// always set level name
	g_pGameWorld->SetLevelName( CMD_ARGV(0).c_str() );

	if( !LoadMissionScript(CMD_ARGV(0).c_str()) )
	{
		// fail-safe mode

	}

	SetCurrentState( g_states[GAME_STATE_GAME], true);
}


//------------------------------------------------------------------------------

void fnMaxplayersTest(ConVar* pVar,char const* pszOldValue)
{
	if(g_pGameSession != NULL && g_pGameSession->GetSessionType() == SESSION_NETWORK)
		Msg("maxplayers will be changed upon restart\n");
}

ConVar sv_maxplayers("maxplayers", "1", fnMaxplayersTest, "Maximum players allowed on the server\n");

//------------------------------------------------------------------------------
// Loads new game world
//------------------------------------------------------------------------------

bool Game_LoadWorld()
{
	Msg("-- LoadWorld --\n");

	g_pGameWorld->Init();
	return g_pGameWorld->LoadLevel();
}

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
			g_pGameSession = new CGameSession();
	}

#ifndef __INTELLISENSE__

	OOLUA::set_global(GetLuaState(), "gameses", g_pGameSession);
	OOLUA::set_global(GetLuaState(), "gameHUD", g_pGameHUD);

#endif // __INTELLISENSE__

	if(g_replayData->m_state != REPL_INIT_PLAYBACK)
		g_replayData->Clear();

	g_pGameSession->Init();

	//reset cameras
	g_nDirectorCameraType = 0;

	g_pCameraAnimator->Reset();

	// reset buttons
	g_nClientButtons = 0;
}

void Game_ShutdownSession()
{
	Msg("-- ShutdownSession --\n");
	g_parallelJobs->Wait();

	effectrenderer->RemoveAllEffects();

	if(g_pGameSession)
		g_pGameSession->Shutdown();

	g_pGameSession = NULL;
}

void Game_DirectorControlKeys(int key, bool down);

void Game_HandleKeys(int key, bool down)
{
	if( g_pGameSession && g_pGameSession->GetPlayerCar() )
	{
		CCar* playerCar = g_pGameSession->GetPlayerCar();

		if((g_nClientButtons & IN_ACCELERATE) != (g_nOldControlButtons & IN_ACCELERATE))
		{
			playerCar->m_accelRatio = 1023;
			playerCar->m_brakeRatio = 1023;
		}
		else if((g_nClientButtons & IN_BRAKE) != (g_nOldControlButtons & IN_BRAKE))
		{
			playerCar->m_accelRatio = 1023;
			playerCar->m_brakeRatio = 1023;
		}
		else if((g_nClientButtons & IN_TURNLEFT) != (g_nOldControlButtons & IN_TURNLEFT) ||
				(g_nClientButtons & IN_TURNRIGHT) != (g_nOldControlButtons & IN_TURNRIGHT))
		{
			playerCar->m_steerRatio = 1024;
			g_nClientButtons &= ~IN_ANALOGSTEER;
		}
	}

	if(g_director.GetBool())
		Game_DirectorControlKeys(key, down);
}

void Game_JoyAxis( short axis, short value )
{
	if( g_pGameSession && g_pGameSession->GetPlayerCar() )
	{
		CCar* playerCar = g_pGameSession->GetPlayerCar();

		int buttons = g_nClientButtons;

		if( axis == 3 )
		{
			if(value == 0)
			{
				buttons &= ~IN_ACCELERATE;
				buttons &= ~IN_BRAKE;

				playerCar->m_accelRatio = 1023;
				playerCar->m_brakeRatio = 1023;
			}
			else if(value > 0)
			{
				float val = (float)value / (float)SHRT_MAX;
				playerCar->m_brakeRatio = val*1023.0f;
				buttons |= IN_BRAKE;
				buttons &= ~IN_ACCELERATE;
			}
			else
			{
				float val = (float)value / (float)SHRT_MAX;
				playerCar->m_accelRatio = val*-1023.0f;
				buttons |= IN_ACCELERATE;
				buttons &= ~IN_BRAKE;
			}
		}
		else if( axis == 0 )
		{
			float val = (float)value / (float)SHRT_MAX;

			if(!(buttons & IN_EXTENDTURN))
				val *= 0.6f;

			playerCar->m_steerRatio = val*1023.0f;

			buttons |= IN_ANALOGSTEER;
			buttons &= ~IN_TURNLEFT;
			buttons &= ~IN_TURNRIGHT;

			//if(value == 0)
			//	buttons &= ~IN_ANALOGSTEER;
		}

		g_nClientButtons = buttons;
	}
}

void Game_UpdateFreeCamera(float fDt)
{
	Vector3D f, r;
	AngleVectors(g_freeCamProps.angles, &f, &r);

	Vector3D camMoveVec(0.0f);

	if(g_nClientButtons & IN_FORWARD)
		camMoveVec += f;
	else if(g_nClientButtons & IN_BACKWARD)
		camMoveVec -= f;

	if(g_nClientButtons & IN_LEFT)
		camMoveVec -= r;
	else if(g_nClientButtons & IN_RIGHT)
		camMoveVec += r;

	g_freeCamProps.velocity += camMoveVec * 200.0f * fDt;

	float camSpeed = length(g_freeCamProps.velocity);

	// limit camera speed
	if(camSpeed > g_freecam_speed.GetFloat())
	{
		float speedDiffScale = g_freecam_speed.GetFloat() / camSpeed;
		g_freeCamProps.velocity *= speedDiffScale;
	}

	btSphereShape collShape(0.5f);

	// update camera collision
	if(camSpeed > 1.0f)
	{
		g_freeCamProps.velocity -= normalize(g_freeCamProps.velocity) * 90.0f * fDt;

		CollisionData_t coll;
		g_pPhysics->TestConvexSweep(&collShape, Quaternion(0,0,0,0), g_freeCamProps.position, g_freeCamProps.position+g_freeCamProps.velocity, coll, 0xFFFFFFFF);

		if(coll.fract == 0.0f)
		{
			float nDot = dot(coll.normal, g_freeCamProps.velocity);
			g_freeCamProps.velocity -= coll.normal*nDot;
		}
	}
	else
	{
		g_freeCamProps.velocity = vec3_zero;
	}

	
	g_pPhysics->m_physics.SetDebugRaycast(true);

	// test code, must be removed after fixing raycast broadphase
	CollisionData_t coll;
	g_pPhysics->TestConvexSweep(&collShape, Quaternion(0,0,0,0), g_freeCamProps.position, g_freeCamProps.position+f*2000.0f, coll, 0xFFFFFFFF);

	debugoverlay->Box3D(coll.position - 0.5f, coll.position + 0.5f, ColorRGBA(0,1,0,0.25f), 0.1f);
	debugoverlay->Line3D(coll.position, coll.position + coll.normal, ColorRGBA(0,0,1,0.25f), ColorRGBA(0,0,1,0.25f) );
	
	g_pPhysics->m_physics.SetDebugRaycast(false);

	g_freeCamProps.position += g_freeCamProps.velocity * fDt;
}

static const wchar_t* cameraTypeStrings[] = {
	L"Outside car",
	L"In car",
	L"Tripod",
	L"Tripod (fixed zoom)",
	L"Static",
};

void Game_DirectorControlKeys(int key, bool down)
{
	CCar* viewedCar = g_pGameSession->GetViewCar();

	if(down)
	{
		//Msg("Director mode keypress: %d\n", key);

		if(key == KEY_ADD)
		{
			replaycamera_t cam;

			cam.fov = g_freeCamProps.fov;
			cam.origin = g_freeCamProps.position;
			cam.rotation = g_freeCamProps.angles;
			cam.startTick = g_replayData->m_tick;
			cam.targetIdx = viewedCar->m_replayID;
			cam.type = g_nDirectorCameraType;

			int camIndex = g_replayData->AddCamera(cam);
			g_replayData->m_currentCamera = camIndex;

			// set camera after keypress
			g_freecam.SetBool(false);

			Msg("Add camera at tick %d\n", cam.startTick);
		}
		else if(key == KEY_KP_ENTER)
		{
			Msg("Set camera\n");
		}
		else if(key == KEY_SPACE)
		{
			Msg("Add camera keyframe\n");
		}
		else if(key >= KEY_1 && key <= KEY_5)
		{
			g_nDirectorCameraType = key - KEY_1;
		}
		else if(key == KEY_LEFT)
		{

		}
		else if(key == KEY_RIGHT)
		{

		}
	}
}

DECLARE_CMD(director_pick_ray, "Director mode - picks object with ray", 0)
{
	if(!g_director.GetBool())
		return;

	Vector3D start = g_freeCamProps.position;
	Vector3D dir;
	AngleVectors(g_freeCamProps.angles, &dir);

	Vector3D end = start + dir*1000.0f;

	CollisionData_t coll;
	g_pPhysics->TestLine(start, end, coll, OBJECTCONTENTS_VEHICLE);

	if(coll.hitobject != NULL && (coll.hitobject->m_flags & BODY_ISCAR))
	{
		CCar* car = (CCar*)coll.hitobject->GetUserData();

		if(car)
			g_pGameSession->SetPlayerCar( car );
	}
}

void Game_DrawDirectorUI( float fDt )
{
	const IVector2D& screenSize = g_pHost->GetWindowSize();

	materials->Setup2D(screenSize.x,screenSize.y);

	wchar_t* str = varargs_w(L"INSERT NEW CAMERA = &#FFFF00;KP_PLUS&;\n"
		L"SET CAMERA = &#FFFF00;KP_ENTER&;\n"
		L"SET CAMERA KEYFRAME = &#FFFF00;SPACE&;\n"
		L"CHANGE CAMERA TYPE = &#FFFF00;1-5&; (Current is &#FFFF00;'%s'&;)\n"
		L"CAMERA ZOOM = &#FFFF00;MOUSE WHEEL&; (%.2f deg.)\n"
		L"SET TARGET OBJECT = &#FFFF00;LEFT MOUSE CLICK ON OBJECT&;\n"
		L"PLAY/PAUSE = &#FFFF00;O&;\n"
		L"FREE CAMERA = &#FFFF00;I&;\n"
		L"SEEK = &#FFFF00;fastseek <frame>&; (in console)\n", cameraTypeStrings[g_nDirectorCameraType], g_freeCamProps.fov);

	eqFontStyleParam_t params;
	params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
	params.textColor = color4_white;

	Vector2D directorTextPos(15, screenSize.y/3);

	g_pHost->GetDefaultFont()->RenderText(str, directorTextPos, params);

	replaycamera_t* cam = g_replayData->GetCurrentCamera();

	wchar_t* framesStr = varargs_w(L"FRAME: &#FFFF00;%d / %d&;\nCAMERA: &#FFFF00;%d&; (%d) / &#FFFF00;%d&;", g_replayData->m_tick, g_replayData->m_numFrames, g_replayData->m_currentCamera+1, cam ? cam->startTick : 0, g_replayData->m_cameras.numElem());

	params.align = TEXT_ALIGN_HCENTER;

	Vector2D frameInfoTextPos(screenSize.x/2, screenSize.y - (screenSize.y/6));
	g_pHost->GetDefaultFont()->RenderText(framesStr, frameInfoTextPos, params);

	if(g_freecam.GetBool())
	{
		Vector2D halfScreen = Vector2D(screenSize)*0.5f;

		Vertex2D_t tmprect[] =
		{
			Vertex2D_t(halfScreen+Vector2D(0,-3), vec2_zero),
			Vertex2D_t(halfScreen+Vector2D(3,3), vec2_zero),
			Vertex2D_t(halfScreen+Vector2D(-3,3), vec2_zero)
		};

		// Draw crosshair
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, tmprect, elementsOf(tmprect), NULL, ColorRGBA(1,1,1,0.45));
	}
}

ConVar eq_profiler_display("eqProfiler_display", "0", "Display profiler on screen");
extern ConVar g_pause;

//-------------------------------------------------------------------------------

CState_Game* g_State_Game = new CState_Game();

CState_Game::CState_Game() : CBaseStateHandler()
{
	m_demoMode = false;
	m_isGameRunning = false;
	m_fade = 1.0f;
	m_doLoadingFrames = 0;
}

CState_Game::~CState_Game()
{

}

void CState_Game::UnloadGame()
{
	g_pGameHUD->Cleanup();

	StopStreams();

	// renderer must be reset
	g_pShaderAPI->Reset(STATE_RESET_ALL);
	g_pShaderAPI->Apply();

	g_pGameWorld->Cleanup();

	g_pPhysics->SceneShutdown();

	Game_ShutdownSession();

	g_pModelCache->ReleaseCache();

	ses->Shutdown();
}

void CState_Game::LoadGame()
{
	soundsystem->SetVolumeScale( 0.0f );

	UnloadGame();

	ses->Init(EQ_DRVSYN_DEFAULT_SOUND_DISTANCE);

	PrecacheStudioModel( "models/error.egf" );
	PrecacheScriptSound( "menu.back" );
	PrecacheScriptSound( "menu.roll" );

	g_pPhysics->SceneInit();

	g_pGameHUD->Init();

	if( Game_LoadWorld() )
	{
		Game_InitializeSession();
	}
	else
	{
		SetNextState(g_states[GAME_STATE_TITLESCREEN]);
		m_loadingError = true;
	}
}

void CState_Game::StopStreams()
{
	// stop any voices
	ISoundPlayable* voiceChan = soundsystem->GetStaticStreamChannel(CHAN_VOICE);
	if(voiceChan)
		voiceChan->Stop();

	// stop music
	ISoundPlayable* musicChan = soundsystem->GetStaticStreamChannel(CHAN_STREAM);
	if(musicChan)
		musicChan->Stop();
}

void CState_Game::QuickRestart(bool replay)
{
	g_pGameHUD->Cleanup();

	StopStreams();

	m_isGameRunning = false;
	m_exitGame = false;
	m_fade = 1.0f;

	// renderer must be reset
	g_pShaderAPI->Reset(STATE_RESET_ALL);
	g_pShaderAPI->Apply();

	g_pGameWorld->Cleanup(false);

	Game_ShutdownSession();

	g_pGameWorld->LoadLevel();

	g_pGameHUD->Init();

	g_pGameWorld->Init();

	Game_InitializeSession();

	//-------------------------

	if(!replay)
		SetupMenuStack( m_gameMenuName.c_str() );
}

void CState_Game::OnEnterSelection( bool isFinal )
{
	if(isFinal)
	{
		m_fade = 0.0f;
		m_exitGame = true;
		m_showMenu = false;
	}
}

void CState_Game::SetupMenuStack( const char* name )
{
	OOLUA::Table mainMenuStack;
	if(!OOLUA::get_global(GetLuaState(), name, mainMenuStack))
		WarningMsg("Failed to get %s table (DrvSynMenus.lua ???)!\n", name);
	else
		SetMenuObject( mainMenuStack );
}

void CState_Game::OnMenuCommand( const char* command )
{
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
		m_showMenu = false;
		m_exitGame = false;
		m_fade = 0.0f;

		if(g_pGameSession->GetMissionStatus() == MIS_STATUS_INGAME)
			g_pGameSession->SignalMissionStatus(MIS_STATUS_FAILED, 0.0f);

		m_scheduledRestart = true;
	}
	else if(!stricmp(command, "quickReplay"))
	{
		m_showMenu = false;
		m_exitGame = false;
		m_fade = 0.0f;

		if(g_pGameSession->GetMissionStatus() == MIS_STATUS_INGAME)
		{
			SetupMenuStack("MissionEndMenuStack");
			g_pGameSession->SignalMissionStatus(MIS_STATUS_FAILED, 0.0f);
		}

		m_scheduledQuickReplay = true;
	}
	else if(!stricmp(command, "goToDirector"))
	{
		Msg("TODO: go to the director\n");
	}
}

// when changed to this state
// @from - used to transfer data
void CState_Game::OnEnter( CBaseStateHandler* from )
{
	if(m_isGameRunning)
		return;

	m_loadingError = false;
	m_exitGame = false;
	m_showMenu = false;

	m_scheduledRestart = false;
	m_scheduledQuickReplay = false;

	m_doLoadingFrames = 2;

	m_fade = 1.0f;

	m_menuTitleToken = g_localizer->GetToken("MENU_GAME_TITLE_PAUSE");
}

bool CState_Game::DoLoadingFrame()
{
	LoadGame();

    if(!g_pGameSession)
        return false;	// no game session causes a real problem

	if(g_pGameSession->GetSessionType() == SESSION_SINGLE)
		m_gameMenuName = "GameMenuStack";
	else if(g_pGameSession->GetSessionType() == SESSION_NETWORK)
		m_gameMenuName = "MPGameMenuStack";

	SetupMenuStack( m_gameMenuName.c_str() );

	return true;
}

// when the state changes to something
// @to - used to transfer data
void CState_Game::OnLeave( CBaseStateHandler* to )
{
	m_demoMode = false;

	if(!g_pGameSession)
		return;

	UnloadGame();

	if(	m_isGameRunning )
	{
		m_isGameRunning = false;
	}
}

int CState_Game::GetPauseMode() const
{
	if(g_pGameSession->IsGameDone())
		return g_pGameSession->GetMissionStatus() == MIS_STATUS_SUCCESS ? PAUSEMODE_COMPLETE : PAUSEMODE_GAMEOVER;

	if((g_pause.GetBool() || m_showMenu) && g_pGameSession->GetSessionType() == SESSION_SINGLE)
		return PAUSEMODE_PAUSE;

	return PAUSEMODE_NONE;
}

void CState_Game::SetPauseState( bool state )
{
	if(!m_exitGame)
		m_showMenu = state;

	if(m_showMenu)
		m_selection = 0;

	// update pause state
	UpdatePauseState();
}

void CState_Game::StartReplay( const char* path )
{
	g_replayData->LoadFromFile( path );

	SetCurrentState( this, true );
}

void CState_Game::DrawLoadingScreen()
{
	const IVector2D& screenSize = g_pHost->GetWindowSize();

	materials->Setup2D(screenSize.x, screenSize.y);
	g_pShaderAPI->Clear( true,true, false );

	IEqFont* font = g_fontCache->GetFont("Roboto Condensed", 30, TEXT_STYLE_BOLD+TEXT_STYLE_ITALIC);

	const wchar_t* loadingStr = LocalizedString("#GAME_IS_LOADING");

	eqFontStyleParam_t param;
	param.styleFlag |= TEXT_STYLE_SHADOW;

	font->RenderText(loadingStr, Vector2D(100,screenSize.y - 100), param);
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
		
		m_doLoadingFrames--;

		if( m_doLoadingFrames > 0 )
			return true;
		else if( m_doLoadingFrames == 0 )	
			return DoLoadingFrame(); // actual level loading happened here

		if(g_pGameWorld->m_level.IsWorkDone() && materials->GetLoadingQueue() == 0)
			m_isGameRunning = true;

		return true;
	}

	float fGameFrameDt = fDt;

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
		if(m_demoMode)
		{
			m_exitGame = true;
			m_fade = 0.0f;
			SetNextState(g_states[GAME_STATE_TITLESCREEN]);
		}
		else if(!m_showMenu)
		{
			// set other menu
			m_showMenu = !m_scheduledRestart && !m_scheduledQuickReplay;

			SetupMenuStack("MissionEndMenuStack");
		}
	}

	// update pause state
	if( !UpdatePauseState() )
		fGameFrameDt = 0.0f;

	// reset buttons
	if(m_showMenu)
		g_nClientButtons = 0;

	//
	// Update, Render, etc
	//
	DoGameFrame( fGameFrameDt );

	if(m_exitGame || m_scheduledRestart || m_scheduledQuickReplay)
	{
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

			if(m_scheduledQuickReplay)
				Game_InstantReplay(0);

			m_scheduledRestart = false;
			m_scheduledQuickReplay = false;

			return !m_exitGame;
		}

		soundsystem->SetVolumeScale(1.0f-m_fade);
	}
	else
	{
		if( m_fade > 0.0f )
		{
			ColorRGBA blockCol(0.0,0.0,0.0,1.0f);

			Vertex2D_t tmprect1[] = { MAKETEXQUAD(0, 0,screenSize.x, screenSize.y*m_fade*0.5f, 0) };
			Vertex2D_t tmprect2[] = { MAKETEXQUAD(0, screenSize.y*0.5f + screenSize.y*(1.0f-m_fade)*0.5f,screenSize.x, screenSize.y, 0) };

			materials->Setup2D(screenSize.x, screenSize.y);
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect1,elementsOf(tmprect1), NULL, blockCol);
			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect2,elementsOf(tmprect2), NULL, blockCol);

			m_fade -= fDt*5.0f;

			soundsystem->SetVolumeScale(1.0f-m_fade);
		}
		else
			soundsystem->SetVolumeScale( 1.0f );
	}

	DrawMenu(fDt);

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
	else
	{
		if(m_pauseState != (pauseMode > 0))
		{
			ISoundPlayable* musicChan = soundsystem->GetStaticStreamChannel(CHAN_STREAM);
			if(musicChan)
				musicChan->Play();

			ISoundPlayable* voiceChan = soundsystem->GetStaticStreamChannel(CHAN_VOICE);
			if(voiceChan)
				voiceChan->Play();
		}
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

	IVector2D halfScreen(screenSize.x/2, screenSize.y/2);

	IEqFont* font = g_fontCache->GetFont("Roboto", 30);

	eqFontStyleParam_t fontParam;
	fontParam.align = TEXT_ALIGN_HCENTER;
	fontParam.styleFlag |= TEXT_STYLE_SHADOW;
	fontParam.textColor = color4_white;

	{
		lua_State* state = GetLuaState();

		EqLua::LuaStackGuard g(state);

		int numElems = 0;

		oolua_ipairs(m_menuElems)
			numElems++;
		oolua_ipairs_end()

		int menuPosY = halfScreen.y - numElems*font->GetLineHeight()*0.5f;

		Vector2D mTextPos(halfScreen.x, menuPosY);

		fontParam.textColor = ColorRGBA(0.7f,0.7f,0.7f,1.0f);
		font->RenderText(m_menuTitleToken ? m_menuTitleToken->GetText() : L"Undefined token", mTextPos, fontParam);

		oolua_ipairs(m_menuElems)
			int idx = _i_index_-1;

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			const wchar_t* token = NULL;

			ILocToken* tok = NULL;
			if(elem.safe_at("label", tok))
				token = tok ? tok->GetText() : L"Undefined token";

			EqLua::LuaTableFuncRef labelValue;
			if(labelValue.Get(elem, "labelValue", true) && labelValue.Push() && labelValue.Call(0, 1))
			{
				int val = 0;
				OOLUA::pull(state, val);

				token = tok ? varargs_w(tok->GetText(), val) : L"Undefined token";
			}

			if(m_selection == idx)
				fontParam.textColor = ColorRGBA(1,0.7f,0.0f,1.0f);
			else
				fontParam.textColor = ColorRGBA(1,1,1,1.0f);

			Vector2D eTextPos(halfScreen.x, menuPosY+_i_index_*font->GetLineHeight());

			font->RenderText(token ? token : L"No token", eTextPos, fontParam);
		oolua_ipairs_end()

	}
}

CCar* CState_Game::GetViewCar() const
{
	CCar* viewedCar = g_pGameSession->GetViewCar();

	if(g_replayData->m_state == REPL_PLAYING && g_replayData->m_cameras.numElem() > 0)
	{
		// replay controls camera
		replaycamera_t* replCamera = g_replayData->GetCurrentCamera();

		if(replCamera)
			viewedCar = g_replayData->GetCarByReplayIndex( replCamera->targetIdx );
	}

	return viewedCar;
}

Vector3D CState_Game::GetViewVelocity() const
{
	CCar* viewedCar = GetViewCar();

	Vector3D cam_velocity = vec3_zero;

	// animate the camera if car is present
	if( viewedCar && g_pCameraAnimator->GetMode() <= CAM_MODE_INCAR && !g_freecam.GetBool() )
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

	// render
	PROFILE_CODE(g_pGameWorld->Draw( 0 ));
}

void CState_Game::RenderMainView2D( float fDt )
{
	const IVector2D& screenSize = g_pHost->GetWindowSize();

	// draw HUD
	if( g_replayData->m_state != REPL_PLAYING )
		g_pGameHUD->Render( fDt, screenSize );

	if(	g_director.GetBool() && g_replayData->m_state == REPL_PLAYING)
		Game_DrawDirectorUI( fDt );

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

	// session update
	g_pGameSession->UpdateLocalControls( g_nClientButtons );
	g_pGameSession->Update(fDt);

	//Game_UpdateCamera(fDt);
	DoCameraUpdates( fDt );

	// render all
	RenderMainView3D( fDt );
	RenderMainView2D( fDt );

	g_nOldControlButtons = g_nClientButtons;
}

void CState_Game::DoCameraUpdates( float fDt )
{
	int camControls = (g_replayData->m_state == REPL_PLAYING) ? 0 : g_nClientButtons;

	CViewParams* curView = g_pGameWorld->GetView();

	if( g_freecam.GetBool() )
	{
		Game_UpdateFreeCamera( g_pHost->GetFrameTime() );

		curView->SetOrigin(g_freeCamProps.position);
		curView->SetAngles(g_freeCamProps.angles);
		curView->SetFOV(g_freeCamProps.fov);

		g_pCameraAnimator->SetOrigin(g_freeCamProps.position);
	}
	else
	{
		if(!g_pCameraAnimator->IsScripted())
		{
			CCar* viewedCar = g_pGameSession->GetViewCar();

			if(g_replayData->m_state == REPL_PLAYING && g_replayData->m_cameras.numElem() > 0)
			{
				// replay controls camera
				replaycamera_t* replCamera = g_replayData->GetCurrentCamera();

				if(replCamera)
				{
					// Process camera
					viewedCar = g_replayData->GetCarByReplayIndex( replCamera->targetIdx );

					g_pCameraAnimator->SetMode( (ECameraMode)replCamera->type );
					g_pCameraAnimator->SetOrigin( replCamera->origin );
					g_pCameraAnimator->SetAngles( replCamera->rotation );
					g_pCameraAnimator->SetFOV( replCamera->fov );

					g_pCameraAnimator->Update(fDt, 0, viewedCar);
				}
			}

			g_pCameraAnimator->Update(fDt, camControls, viewedCar);
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

	// all positions and velocity props
	soundsystem->SetListener(curView->GetOrigin(), f, u, viewVelocity);
	effectrenderer->SetViewSortPosition(curView->GetOrigin());
	g_pRainEmitter->SetViewVelocity(viewVelocity);
}

void CState_Game::HandleKeyPress( int key, bool down )
{
	if(!m_isGameRunning)
		return;

	if( m_demoMode )
	{
		if(m_fade <= 0.0f)
		{
			m_fade = 0.0f;
			m_exitGame = true;
			SetNextState(g_states[GAME_STATE_TITLESCREEN]);
		}

		return;
	}

	if(key == KEY_ESCAPE && down)
	{
		if(m_showMenu && IsCanPopMenu())
		{
			EmitSound_t es("menu.back");
			ses->Emit2DSound( &es );

			PopMenu();

			return;
		}

		SetPauseState( !m_showMenu );
	}

	if( m_showMenu )
	{
		if(!down)
			return;

		if(key == KEY_ENTER)
		{
			PreEnterSelection();
			EnterSelection();
		}
		else if(key == KEY_LEFT || key == KEY_RIGHT)
		{
			if(ChangeSelection(key == KEY_LEFT ? -1 : 1))
			{
				EmitSound_t es("menu.roll");
				ses->Emit2DSound( &es );
			}
		}
		else if(key == KEY_UP)
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
			ses->Emit2DSound(&ep);
		}
		else if(key == KEY_DOWN)
		{
reincrement:
			m_selection++;

			if(m_selection > m_numElems-1)
				m_selection = 0;

			//if(pItem->type == MIT_SPACER)
			//	goto reincrement;

			EmitSound_t ep("menu.roll");
			ses->Emit2DSound(&ep);
		}
	}
	else
	{
		Game_HandleKeys(key, down);

		//if(!MenuKeys( key, down ))
			GetKeyBindings()->OnKeyEvent( key, down );
	}
}

void CState_Game::HandleMouseMove( int x, int y, float deltaX, float deltaY )
{
	if(!m_isGameRunning)
		return;

	g_pHost->SetCenterMouseEnable( g_freecam.GetBool() );
	g_pHost->SetCursorShow( g_pSysConsole->IsVisible() );

	if( m_showMenu )
		return;

	if(g_freecam.GetBool() && !g_pSysConsole->IsVisible()) // && g_pHost->m_hasWindowFocus)
	{
		g_freeCamProps.angles.x += deltaY * g_mouse_sens.GetFloat();
		g_freeCamProps.angles.y += deltaX * g_mouse_sens.GetFloat();
	}
}

void CState_Game::HandleMouseWheel(int x,int y,int scroll)
{
	if(!m_isGameRunning)
		return;

	g_freeCamProps.fov -= scroll;
}

void CState_Game::HandleJoyAxis( short axis, short value )
{
	if(!m_isGameRunning)
		return;

	Game_JoyAxis(axis,value);
}
