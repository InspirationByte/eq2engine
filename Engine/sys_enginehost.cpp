//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine system class
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "IEngineGame.h"
#include "sys_console.h"


#include "Imaging/ImageLoader.h"

#include "materialsystem/IMaterialSystem.h"
#include "DebugOverlay.h"
#include "ISoundSystem.h"
#include "IViewRenderer.h"
#include "Physics/DkBulletPhysics.h"
//#include "network/net_thread.h"

#include "sys_enginehost.h"
#include "modules.h"
#include "cfgloader.h"
#include <winuser.h>
#include <direct.h>
#include "IGameLibrary.h"
#include "Font.h"
#include "FontCache.h"

#ifdef PLAT_SDL
#include "SDL_mouse.h"
#endif

/*
enum HostEmulationMode_e
{
	HOSTEMU_EQUILIBRIUM = 0,
	HOSTEMU_EQUILIBRIUMFX,
};
*/

//------------------
// Generic settings
//------------------

// Renderer
DECLARE_CVAR		(r_mode,1024x768,"Screen Resoulution. Resolution string format: WIDTHxHEIGHT" ,CV_ARCHIVE);
DECLARE_CVAR		(r_bpp,32,"Screen bits per pixel",CV_ARCHIVE);

ConVar r_fullscreen("r_fullscreen", "0", "Fullscreen mode" ,CV_ARCHIVE);

DECLARE_CVAR		(r_clear,0,"Clear the backbuffer",CV_ARCHIVE);
DECLARE_CVAR		(r_vSync,0,"Vertical syncronization",CV_ARCHIVE);
DECLARE_CVAR		(r_antialiasing,0,"Antialiasing",CV_ARCHIVE);

extern ConVar		r_renderer;

// Mouse
DECLARE_CVAR		(m_sensitivity,1.0f,"Mouse sensetivity",CV_ARCHIVE);
DECLARE_CVAR		(m_invert,0,"Mouse inversion enabled?",CV_ARCHIVE);

DECLARE_CVAR		(screenshotJpegQuality,100,"JPEG Quality",CV_ARCHIVE);

void CC_Exit_F(DkList<EqString> *args)
{
	g_pEngineHost->SetQuitting( IEngineHost::QUIT_TODESKTOP );
}

ConCommand cc_exit("exit",CC_Exit_F,"Closes current instance of engine");
ConCommand cc_quit("quit",CC_Exit_F,"Closes current instance of engine");
ConCommand cc_quti("quti",CC_Exit_F,"This made for keyboard writing errors");

void CC_Error_f(DkList<EqString> *args)
{
	ErrorMsg("FATAL ERROR");
}

ConCommand cc_error("test_error",CC_Error_f,"Test error");

void cc_printres_f(DkList<EqString> *args)
{
	materials->PrintLoadedMaterials();
	g_pModelCache->PrintLoadedModels();
}
ConCommand cc_printres("sys_print_resources",cc_printres_f,"Prints all loaded resources to the console");

ConVar con_enable("con_enable","1","Enable console",CV_ARCHIVE | CV_CHEAT);

DECLARE_CMD(toggleconsole, "Toggles console", 0)
{
	if(g_pSysConsole->IsVisible() && g_pSysConsole->IsShiftPressed())
	{
		g_pSysConsole->SetLogVisible(g_pSysConsole->IsLogVisible());
		return;
	}

	//if(engine->GetGameState() == IEngineGame::GAME_NOTRUNNING)
	//	return;

	if(!con_enable.GetBool())
	{
		g_pSysConsole->SetVisible(false);
		g_pEngineHost->SetCursorShow(false, true);
		return;
	}

	if(g_pSysConsole->IsVisible())
	{
		g_pSysConsole->SetVisible(false);
		g_pEngineHost->SetCursorShow(false, true);
	}
	else
	{
		g_pSysConsole->SetVisible(true);
		g_pEngineHost->SetCursorShow(true, true);
	}

	g_pEngineHost->SetCursorShow(g_pSysConsole->IsVisible(), true);
}

static EQCURSOR staticDefaultCursor[20];

// TODO: Move this to GUI
enum CursorCode
{
	dc_user,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_crosshair,
	dc_up,
	dc_sizenwse,
	dc_sizenesw,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_last,
};

#ifdef PLAT_SDL

void InputCommands_SDL(SDL_Event* event)
{
	static int lastX, lastY;

	switch(event->type)
	{
		case SDL_KEYUP:
		case SDL_KEYDOWN:
		{
			int nKey = event->key.keysym.scancode;

			// do translation
			if(nKey == SDL_SCANCODE_RSHIFT)
				nKey = SDL_SCANCODE_LSHIFT;
			else if(nKey == SDL_SCANCODE_RCTRL)
				nKey = SDL_SCANCODE_LCTRL;
			else if(nKey == SDL_SCANCODE_RALT)
				nKey = SDL_SCANCODE_LALT;

			g_pEngineHost->TrapKey_Event( nKey, (event->type == SDL_KEYUP) ? false : true );
		}
		case SDL_TEXTINPUT:
		{
			// FIXME: is it good?
			g_pEngineHost->ProcessKeyChar( event->text.text[0] );
			break;
		}
		case SDL_MOUSEMOTION:
		{
			int x, y;
			x = event->motion.x;
			y = event->motion.y;

			g_pEngineHost->TrapMouseMove_Event(x, y, x - lastX, y - lastY);

			lastX = x;
			lastY = y;

			break;
		}
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		{
			int x, y;
			x = event->button.x;
			y = event->button.y;

			int mouseButton = MOU_B1;

			if(event->button.button == 1)
				mouseButton = MOU_B1;
			else if(event->button.button == 2)
				mouseButton = MOU_B3;
			else if(event->button.button == 3)
				mouseButton = MOU_B2;
			else if(event->button.button == 4)
				mouseButton = MOU_B4;
			else if(event->button.button == 5)
				mouseButton = MOU_B5;

			g_pEngineHost->TrapMouse_Event(x, y, mouseButton, (event->type == SDL_MOUSEBUTTONUP) ? false : true);

			break;
		}
		case SDL_MOUSEWHEEL:
		{
			int x, y;
			x = event->wheel.x;
			y = event->wheel.y;

			g_pEngineHost->TrapMouseWheel_Event(lastX, lastY, y);

			break;
		}
	}
}

#elif PLAT_WIN

#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)

#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))

void InputCommands_MSW(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
		static int lastX, lastY;
		int x, y;
		x = GETX(lParam);
		y = GETY(lParam);

		g_pEngineHost->TrapMouseMove_Event(x, y, x - lastX, y - lastY);
		lastX = x;
		lastY = y;
		break;
	case WM_KEYDOWN:
		g_pEngineHost->TrapKey_Event((unsigned int) wParam, true);
		break;
	case WM_KEYUP:
		g_pEngineHost->TrapKey_Event((unsigned int) wParam, false);
		break;
	case WM_CHAR:
		g_pEngineHost->ProcessKeyChar( (ubyte)wParam );
		break;
	case WM_LBUTTONDOWN:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B1, true);
		break;
	case WM_RBUTTONDOWN:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B2, true);
		break;
	case WM_MBUTTONDOWN:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B3, true);
		break;
	case WM_XBUTTONDOWN:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B4, true);
		break;
	case WM_LBUTTONUP:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B1, false);
		break;
	case WM_RBUTTONUP:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B2, false);
		break;
	case WM_MBUTTONUP:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B3, false);
		break;
	case WM_XBUTTONUP:
		g_pEngineHost->TrapMouse_Event(GETX(lParam), GETY(lParam), MOU_B4, false);
		break;
	case WM_MOUSEWHEEL:
		static int scroll;
		int s;

		scroll += GET_WHEEL_DELTA_WPARAM(wParam);
		s = scroll / WHEEL_DELTA;
		scroll %= WHEEL_DELTA;

		POINT point;
		point.x = GETX(lParam);
		point.y = GETY(lParam);
		ScreenToClient(hwnd, &point);

		if (s != 0)
			g_pEngineHost->TrapMouseWheel_Event(point.x, point.y, s);

		break;
	case WM_SYSKEYDOWN:
		g_pEngineHost->TrapKey_Event((unsigned int) wParam, true);
		break;
	case WM_SYSKEYUP:
		g_pEngineHost->TrapKey_Event((unsigned int) wParam, false);
		break;
	}
}
#endif // PLAT_SDL

static CEngineHost g_Engine;
IEngineHost *g_pEngineHost = ( IEngineHost * )&g_Engine;

// Initializes engine (look at sys_main.cpp)
bool InitEngineHost()
{
	return g_Engine.Init();
}

// Initializes engine (look at sys_main.cpp)
void ShutdownEngineHost()
{
	g_Engine.Shutdown();
}

// The main engine class pointers
IShaderAPI *g_pShaderAPI = NULL;
IEngineSceneRenderer *scenerenderer = NULL;
IPhysics *physics = NULL;
IMaterialSystem *materials = NULL;

static CDebugOverlay g_DebugOverlays;
IDebugOverlay *debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

IGameLibrary* gamedll = NULL;

void CC_Screenshot_f(DkList<EqString> *args)
{
	if(materials != NULL)
	{
		g_pEngineHost->EnterResourceLoading();

		FILE *file = NULL;

		int i = 0;
		do 
		{
			EqString path = GetFileSystem()->GetCurrentGameDirectory() + _Es(varargs("/screenshots/screenshot_%04d.jpg", i));

			if ((file = fopen(path.GetData(), "r")) != NULL)
			{
				fclose(file);
			} 
			else 
			{
				CImage img;
				if (materials->CaptureScreenshot(img))
				{
					MsgInfo("Saving screenshot to: %s\n",path.GetData());
					img.SaveJPEG(path.GetData(), screenshotJpegQuality.GetInt());
				}

				g_pEngineHost->EndResourceLoading();
				return;
			}
			i++;
		} 
		while (i < 9999);

		g_pEngineHost->EndResourceLoading();

		return;
	}
}
ConCommand cc_screenshot("screenshot",CC_Screenshot_f,"Save screenshot");

DKMODULE *pMatSystem = NULL;
DKMODULE *pGameDll = NULL;

int CEngineHost::GetResourceLoadingTimes()
{
	return m_nLoadingResourceTimes;
}

void CEngineHost::EnterResourceLoading()
{
	if(!m_nLoadingResourceTimes)
	{
		m_fResourceLoadTimes = Platform_GetCurrentTime();
		//MsgWarning("EnterResourceLoading\n");
	}

	m_nLoadingResourceTimes++;
}

void CEngineHost::EndResourceLoading()
{
	m_nLoadingResourceTimes--;

	if(!m_nLoadingResourceTimes)
	{
		float this_time = Platform_GetCurrentTime();
		float elapsed_time = this_time - m_fResourceLoadTimes;

		// this thing will reduce frame time.
		m_fOldTime += elapsed_time;

		//MsgWarning("EndResourceLoading (load time: %g s)\n", elapsed_time);
	}
}

void CEngineHost::SetCursorPosition(const IVector2D &pos)
{
#ifdef PLAT_SDL
	SDL_WarpMouseInWindow(m_hHWND, pos.x, pos.y);

	IVector2D realpos;

	SDL_GetMouseState(&realpos.x, &realpos.y);
	m_vCursorPosition = IVector2D(realpos.x, realpos.y);
#elif PLAT_WIN
	POINT point = { (int)pos.x, (int)pos.y };
	ClientToScreen(m_hHWND, &point);
	SetCursorPos(point.x, point.y);

	m_vCursorPosition = IVector2D(point.x, point.y);
#endif // PLAT_SDL
}

IVector2D CEngineHost::GetCursorPosition()
{
	return m_vCursorPosition;
}

//-----------------------------------------------------------------------------

IShaderAPI* CEngineHost::GetRenderer()
{
	if(!g_pShaderAPI)
		return materials->GetShaderAPI();
	else
		return g_pShaderAPI;
}

IPhysics* CEngineHost::GetPhysics()
{
	if(!physics)
		return new DkPhysics();
	else
		return physics;

	return NULL;
}


IMaterialSystem* CEngineHost::GetMaterialSystem()
{
	return materials;
}


EQWNDHANDLE CEngineHost::GetWindowHandle()
{
	return m_hHWND;
}

IEqFont* CEngineHost::GetDefaultFont()
{
	return m_pDefaultFont;
}

IEqFont* CEngineHost::LoadFont(const char* pszFontName)
{
	return InternalLoadFont(pszFontName);
}

bool CEngineHost::LoadRenderer()
{
	pMatSystem = GetFileSystem()->LoadModule("EqMatSystem.dll");

	if(!pMatSystem)
	{
		ErrorMsg("Error! Can't load EqMatSystem.dll. Please reinstall the game\n");
		return false;
	}

	materials = (IMaterialSystem*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);
	if(!materials)
	{
		ErrorMsg("ERROR! Couldn't get interface of EqMatSystem!");
		return false;
	}

	return true;
}

typedef void* (*CreateInterfaceFunc)();

bool CEngineHost::LoadGameLibrary()
{
	pGameDll = LoadGameModule();
	if(pGameDll)
	{
		CreateInterfaceFunc gameLibDispatcher = (CreateInterfaceFunc)GetFileSystem()->GetProcedureAddress( pGameDll, "GameLibFactory");

		if(gameLibDispatcher)
		{
			gamedll = (IGameLibrary*)gameLibDispatcher();
		}
		else
		{
			MsgError("Cannot get factory of game library!\n");
			ErrorMsg("Cannot get factory of game library!\n");
		}

		if(!gamedll)
		{
			MsgError("GameDLL is invalid!\n");
			ErrorMsg("GameDLL is invalid!\n");
			return false;
		}
	}
	else
	{
		MsgError("Cannot load game_win_x86.dll!\n Please reinstall or update game!\n");
		ErrorMsg("Cannot load game_win_x86.dll!\n Please reinstall or update game!\n");
		return false;
	}
	return true;

	return true;
}

void CEngineHost::SetCursorShow(bool bShow, bool console)
{
	bool bIsGameRunning = engine->GetGameState() == IEngineGame::GAME_IDLE;
	bool bIsPaused = (engine->GetGameState() == IEngineGame::GAME_PAUSE);

	bool mustShowCursor = ((!bIsGameRunning || bIsPaused) || console) && bShow;

	if(!bIsGameRunning && !bIsPaused)
		mustShowCursor = true;

#ifdef PLAT_SDL
	
	if(mustShowCursor)
	{
		SDL_SetCursor(staticDefaultCursor[dc_arrow]);
	}

	SDL_ShowCursor(mustShowCursor);
#elif PLAT_WIN
	SetCursor(mustShowCursor ? staticDefaultCursor[dc_arrow] : NULL);
#endif // PLAT_SDL
}

bool CEngineHost::LoadPhysics()
{
	physics = GetPhysics();
	if(!physics)
		return false;

	bool stat = physics->Init(MAX_COORD_UNITS);
	if(!stat)
		return false;
		
	if(physics->IsSupportsHardwareAcceleration())
	{
		MsgAccept("* Hardware physics support: true\n \n");
	}
	else
	{
		MsgWarning("* Hardware physics support: false\n \n");
	}

	return true;//stat;
}

//-----------------------------------------------------------------------------

CEngineHost::CEngineHost()
{
	m_fFrameTime			= 0.0f;
	m_fOldTime				= 0.0f;

	m_bTrapMode				= false;
	m_bDoneTrapping			= false;
	m_nTrapKey				= 0;
	m_nTrapButtons			= 0;

	m_nWidth				= 640;
	m_nHeight				= 480;

	m_pDefaultFont			= NULL;

	m_nQuitting				= QUIT_NOTQUITTING;

	m_bDisableMouseUpdatePerFrame = false;

	m_nLoadingResourceTimes = 0;
	m_fResourceLoadTimes = 0.0f;

	m_vCursorPosition		= IVector2D(0);
}

CEngineHost::~CEngineHost()
{

}

EQWNDHANDLE CreateEngineWindow()
{
	Msg(" \n--------- CreateEngineWindow --------- \n");

	bool isWindowed = !r_fullscreen.GetBool();

	const char *str = r_mode.GetString();
	DkList<EqString> args;
	xstrsplit(str, "x", args);

	int nAdjustedPosX = 0;
	int nAdjustedPosY = 0;
	int nAdjustedWide = atoi(args[0].GetData());
	int nAdjustedTall = atoi(args[1].GetData());

	EQWNDHANDLE handle = NULL;

#ifdef PLAT_SDL

	int sdlFlags = SDL_WINDOW_SHOWN; // SDL_WINDOW_ALLOW_HIGHDPI

	if(isWindowed)
	{
		sdlFlags |= SDL_WINDOW_RESIZABLE;

		const int window_border = 32;

		nAdjustedPosX = window_border/2;
		nAdjustedPosY = window_border/2;
		nAdjustedWide -= window_border;
		nAdjustedTall -= window_border;

	}
	else
	{
		sdlFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
	}

	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_JOYSTICK);

	handle = SDL_CreateWindow("Engine Window", nAdjustedPosX, nAdjustedPosY, nAdjustedWide, nAdjustedTall, sdlFlags);

#elif PLAT_WIN

	DWORD nWindowFlags = 0;

	nWindowFlags = WS_POPUP | WS_CLIPSIBLINGS;

	if(isWindowed)
	{
		nWindowFlags |= WS_OVERLAPPEDWINDOW;

		//UNSETFLAG(nWindowFlags, WS_MAXIMIZEBOX );
		//UNSETFLAG(nWindowFlags, WS_THICKFRAME );

		RECT wRect;
		wRect.left = nAdjustedPosX;
		wRect.right = nAdjustedWide;
		wRect.top = nAdjustedPosY;
		wRect.bottom = nAdjustedTall;
		AdjustWindowRect(&wRect, nWindowFlags, FALSE);

		//nAdjustedPosX = wRect.left;
		//nAdjustedPosY = wRect.top;
		nAdjustedWide = wRect.right;
		nAdjustedTall = wRect.bottom;
	} 

	handle = CreateWindow(	DARKTECH_WINDOW_CLASSNAME, 
							"Engine Window",
							nWindowFlags,
							nAdjustedPosX, nAdjustedPosY, 
							nAdjustedWide, nAdjustedTall, 
							HWND_DESKTOP, NULL, NULL, NULL);
#endif // PLAT_SDL

	Msg("Created render window, %dx%d\n", nAdjustedWide, nAdjustedTall);

	return handle;
}

void Eng_BeginResourceLoading()
{
	g_pEngineHost->EnterResourceLoading();
}

void Eng_EndResourceLoading()
{
	g_pEngineHost->EndResourceLoading();
}

void OnEngineError()
{
#ifdef PLAT_SDL
	SDL_DestroyWindow(g_pEngineHost->GetWindowHandle());
#elif PLAT_WIN
	CloseWindow(g_pEngineHost->GetWindowHandle());
#endif // PLAT_SDL
}

extern ConVar r_advancedLighting;
extern ConVar r_bumpMapping;
extern ConVar r_specular;
extern ConVar r_shadows;
extern ConVar r_useLigting;

ConVar r_useffp("r_useffp", "0", "Forces no shaders to matsystem", CV_CHEAT);
ConVar r_shaderquality("r_shaderquality", "0", "Shader quality, 0 is best", CV_ARCHIVE);
ConVar sys_threadedmatsystem("sys_threadedmaterialloader", "0", "Use threaded material loading", CV_ARCHIVE);

bool CEngineHost::InitSubSystems()
{
	Msg(" \n--------- InitSubSystems --------- \n");

	bool isWindowed = !r_fullscreen.GetBool();
	m_hHWND = CreateEngineWindow();

	SDL_GetWindowSize(m_hHWND, &m_nWidth, &m_nHeight);

	SetPreErrorCallback(OnEngineError);

	int bpp = r_bpp.GetInt();

	ETextureFormat format = FORMAT_RGB8;

	// Figure display format to use
	if(bpp == 32) 
	{
		format = FORMAT_RGB8;
	}
	else if(bpp == 24) 
	{
		format = FORMAT_RGB8;
	}
	else if(bpp == 16) 
	{
		format = FORMAT_RGB565;
	}

	matsystem_render_config_t materials_config;

	materials_config.enableBumpmapping = r_bumpMapping.GetBool();
	materials_config.enableSpecular = r_specular.GetBool();
	materials_config.enableShadows = r_shadows.GetBool();

	materials_config.ffp_mode = r_useffp.GetBool();

	MaterialLightingMode_e lightingModel = MATERIAL_LIGHT_UNLIT;
	
	if(r_useLigting.GetBool())
	{
		lightingModel = MATERIAL_LIGHT_FORWARD;
		if(r_advancedLighting.GetBool())
			lightingModel = MATERIAL_LIGHT_DEFERRED;
	}

	materials_config.lighting_model = lightingModel;
	materials_config.stubMode = false;
	materials_config.editormode = false;
	materials_config.lowShaderQuality = r_shaderquality.GetBool();
	materials_config.threadedloader = sys_threadedmatsystem.GetBool();

	DefaultShaderAPIParameters(&materials_config.shaderapi_params);
	materials_config.shaderapi_params.bIsWindowed = isWindowed;

	// set window
#ifdef PLAT_SDL
	SDL_SysWMinfo winfo;
	SDL_VERSION(&winfo.version); // initialize info structure with SDL version info

	if( !SDL_GetWindowWMInfo(m_hHWND, &winfo) )
	{
		MsgError("SDL_GetWindowWMInfo failed %s", SDL_GetError());
		ErrorMsg("Can't get SDL window WM info!\n");
		return false;
	}

#ifdef _WIN32
	materials_config.shaderapi_params.hWindow = winfo.info.win.window;
#elif LINUX
	// TODO: LINUX VERSION
#endif

#elif PLAT_WIN // pure windows
	materials_config.shaderapi_params.hWindow = m_hHWND;
#endif // PLAT_SDL
	
	materials_config.shaderapi_params.nScreenFormat = format;
	materials_config.shaderapi_params.bEnableVerticalSync = r_vSync.GetBool();
	materials_config.shaderapi_params.nMultisample = r_antialiasing.GetInt();

	bool materialSystemStatus = materials->Init("materials/", (char*)r_renderer.GetString(), materials_config);

	materials->SetResourceBeginEndLoadCallback(Eng_BeginResourceLoading, Eng_EndResourceLoading);

	if(!materialSystemStatus)
		return false;

	g_pShaderAPI = materials->GetShaderAPI();

	// Initialize sound system
	soundsystem->Init();

	if(!g_fontCache->Init())
		return false;

	g_pSysConsole->Initialize();

	debugoverlay->Init();

	engine->Init();

	Msg("--------- GameDllInit --------- \n");
	if(!gamedll->Init(soundsystem, physics, debugoverlay, g_pModelCache, viewrenderer))
		return false;

	// add default shaders library first
	materials->LoadShaderLibrary("eqBaseShaders.dll");

	return true;
}

bool CEngineHost::Init()
{
	Msg(" \n--------- InitEngineHost --------- \n");

	bool initStatus = false;
	
	EqString gameName("Equilibrium Engine");

	KeyValues gameInfoData;

	bool infoloaded = gameInfoData.LoadFromFile("GameInfo.txt");

	kvkeybase_t* pGameInfoSec = NULL;

	if(infoloaded)
	{
		pGameInfoSec = gameInfoData.GetRootSection()->FindKeyBase("GameInfo", KV_FLAG_SECTION);

		if(!pGameInfoSec)
		{
			infoloaded = false;
		}
		else
		{
			gameName = KV_GetValueString(pGameInfoSec->FindKeyBase( "GameName" ), 0, "Equilibrium Engine");

			for(int i = 0; i < pGameInfoSec->keys.numElem(); i++)
			{
				if(!stricmp(pGameInfoSec->keys[i]->name,"AddSearchPath"))
				{
					GetFileSystem()->AddSearchPath(pGameInfoSec->keys[i]->values[0]);
				}
				else if(!stricmp(pGameInfoSec->keys[i]->name,"AddPackage"))
				{
					GetFileSystem()->AddPackage(pGameInfoSec->keys[i]->values[0], SP_MOD);
				}
			}
		}
	}
	
	//mkdir("screenshots");

	GetFileSystem()->MakeDir("Cfg", SP_MOD);
	GetFileSystem()->MakeDir("SavedGames", SP_MOD);
	GetFileSystem()->MakeDir("Screenshots", SP_MOD);
	
	// Parse the rs_renderer from configuration files
	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->ParseFileToCommandBuffer("cfg/config_default.cfg","r_renderer");
	GetCommandAccessor()->ExecuteCommandBuffer();
	
	// register our host
	GetCore()->RegisterInterface( IENGINEHOST_INTERFACE_VERSION, this );
	
	// Init renderer
	initStatus = LoadRenderer();
	if(!initStatus)
		return initStatus;
	
	// Init physics
	initStatus = LoadPhysics();
	if(!initStatus)
		return initStatus;

	// register physics interface
	GetCore()->RegisterInterface( IPHYSICS_INTERFACE_VERSION, physics );

	// Init game DLL
	initStatus = LoadGameLibrary();
	if(!initStatus)
		return initStatus;
		
	// execute configuration files and command line after all libraries are loaded.
	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->ParseFileToCommandBuffer("cfg/config_default.cfg");
	GetCommandAccessor()->ExecuteCommandBuffer();

	GetCmdLine()->ExecuteCommandLine(true,true);

	// initialize subsystems
	if(!InitSubSystems())
		return false;
	
#ifdef PLAT_SDL

	SDL_SetWindowTitle(m_hHWND, gameName.c_str());

	staticDefaultCursor[dc_none]     = NULL;
	staticDefaultCursor[dc_arrow]    =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	staticDefaultCursor[dc_ibeam]    =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	staticDefaultCursor[dc_hourglass]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	staticDefaultCursor[dc_crosshair]=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	staticDefaultCursor[dc_up]       =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
	staticDefaultCursor[dc_sizenwse] =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	staticDefaultCursor[dc_sizenesw] =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	staticDefaultCursor[dc_sizewe]   =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	staticDefaultCursor[dc_sizens]   =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	staticDefaultCursor[dc_sizeall]  =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	staticDefaultCursor[dc_no]       =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
	staticDefaultCursor[dc_hand]     =SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

	// Set default cursor
	SDL_SetCursor( staticDefaultCursor[dc_arrow] );

#elif PLAT_WIN
	SetWindowText(m_hHWND, gameName.c_str());

	staticDefaultCursor[dc_none]     = NULL;
	staticDefaultCursor[dc_arrow]    =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_ARROW);
	staticDefaultCursor[dc_ibeam]    =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_IBEAM);
	staticDefaultCursor[dc_hourglass]=(HICON)LoadCursor(NULL,(LPCTSTR)IDC_WAIT);
	staticDefaultCursor[dc_crosshair]=(HICON)LoadCursor(NULL,(LPCTSTR)IDC_CROSS);
	staticDefaultCursor[dc_up]       =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_UPARROW);
	staticDefaultCursor[dc_sizenwse] =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_SIZENWSE);
	staticDefaultCursor[dc_sizenesw] =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_SIZENESW);
	staticDefaultCursor[dc_sizewe]   =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_SIZEWE);
	staticDefaultCursor[dc_sizens]   =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_SIZENS);
	staticDefaultCursor[dc_sizeall]  =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_SIZEALL);
	staticDefaultCursor[dc_no]       =(HICON)LoadCursor(NULL,(LPCTSTR)IDC_NO);
	staticDefaultCursor[dc_hand]     =(HICON)LoadCursor(NULL,(LPCTSTR)32649);

	// Set default cursor
	SetCursor(staticDefaultCursor[dc_arrow]);

#endif // PLAT_SDL

	if(infoloaded)
	{
		for(int i = 0; i < pGameInfoSec->keys.numElem(); i++)
		{
			if(!stricmp( pGameInfoSec->keys[i]->name,"AddShaderLib"))
			{
				materials->LoadShaderLibrary( pGameInfoSec->keys[i]->values[0] );
			}
		}

		kvkeybase_t* pDefaultWorld = pGameInfoSec->FindKeyBase("DefaultWorld");

		if( pDefaultWorld && engine->GetGameState() != IEngineGame::GAME_LEVEL_LOAD)
		{
			engine->SetLevelName( KV_GetValueString( pDefaultWorld ) );
			engine->SetGameState( IEngineGame::GAME_LEVEL_LOAD );
		}
	}

	m_pDefaultFont = LoadFont("default");

	//InitNetworking();
	
	return initStatus;
}

void CEngineHost::Shutdown()
{
	engine->Shutdown();

	// Save configuration before full unload
	WriteCfgFile("cfg/config.cfg",true);

	// shutdown engine things
	engine->UnloadGame( true, true );

	// shutdown game
	GetFileSystem()->FreeModule( pGameDll );

	// shutdown cached sounds
	soundsystem->ReleaseSamples();

	// Shutdown sound system
	soundsystem->Shutdown();

	// Destroying renderer
	materials->Shutdown();
	GetFileSystem()->FreeModule(pMatSystem);

	// unregister interfaces
	GetCore()->UnregisterInterface(MATSYSTEM_INTERFACE_VERSION);
	GetCore()->UnregisterInterface(IPHYSICS_INTERFACE_VERSION);
	GetCore()->UnregisterInterface(IENGINEHOST_INTERFACE_VERSION);
	GetCore()->UnregisterInterface(IENGINEGAME_INTERFACE_VERSION);

	//ShutdownNetworking();
}

void CEngineHost::StartTrapMode()
{
	if ( m_bTrapMode )
		return;

	m_bDoneTrapping = false;
	m_bTrapMode = true;
}

// Returns true on success, false on failure.
bool CEngineHost::IsTrapping()
{
	return m_bTrapMode;
}

bool CEngineHost::CheckDoneTrapping( int& buttons, int& key )
{
	if ( m_bTrapMode )
		return false;

	if ( !m_bDoneTrapping )
		return false;

	key			= m_nTrapKey;
	buttons		= m_nTrapButtons;

	// Reset since we retrieved the results
	m_bDoneTrapping = false;
	return true;
}

// Flag that we are in the process of quiting
void CEngineHost::SetQuitting( int quittype )
{
	m_nQuitting = quittype;
}

// Purpose: Check whether we are ready to exit
int CEngineHost::GetQuitting()
{
	return m_nQuitting;
}

double CEngineHost::GetFrameTime()
{
	return m_fFrameTime;
}

double CEngineHost::GetCurTime()
{
	return m_fCurTime;
}

static float copyTimeOut = -1;
extern bool s_bActive;

int CEngineHost::Frame()
{
	m_fCurTime		= Platform_GetCurrentTime();

	// Set frame time
	m_fFrameTime	= m_fCurTime - m_fOldTime;

	// If the time is < 0, that means we've restarted. 
	// Set the new time high enough so the engine will run a frame
	if ( m_fFrameTime < 0.001f ) // 512 FPS
	{
		return 0;
	}

	// Remember old time
	m_fOldTime		= m_fCurTime;

	bool can_do_game_frame = g_pShaderAPI->IsDeviceActive() && s_bActive;

	if(!can_do_game_frame)
	{
		if(engine->GetGameState() == IEngineGame::GAME_IDLE)
			engine->SetGameState(IEngineGame::GAME_PAUSE);

		soundsystem->Update();

		BeginScene();
		EndScene();

		//Platform_Sleep(1000);

		return 0;
	}

	// go back
	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// Reset all!
	g_pShaderAPI->Reset();

	// Reset statistics (draw calls num, etc)
	g_pShaderAPI->ResetCounters();

	g_pShaderAPI->Clear(r_clear.GetBool(),true,false,ColorRGBA(0.5f,0.5f,0.5f, 0));

	// Begin frame from render lib
	BeginScene();

	if(!engine->EngineRunFrame( m_fFrameTime ))
		return 0;

	static IEqFont* pFont = LoadFont("console");

	// Draw debug overlays
	debugoverlay->Draw(viewrenderer->GetMatrix(MATRIXMODE_PROJECTION), viewrenderer->GetMatrix(MATRIXMODE_VIEW), m_nWidth, m_nHeight);

	materials->Setup2D(m_nWidth, m_nHeight);

	bool fullscreen_console = (	engine->GetGameState() == IEngineGame::GAME_IDLE ||
								engine->GetGameState() == IEngineGame::GAME_PAUSE || 
								engine->GetGameState() == IEngineGame::GAME_RUNNING_MENU || 
								engine->GetGameState() == IEngineGame::GAME_LEVEL_LOAD);


	g_pSysConsole->DrawSelf(fullscreen_console, m_nWidth, m_nHeight, m_fCurTime);

	// End frame from render lib
	EndScene();

	float measuresound = MEASURE_TIME_BEGIN();

	// Update sound system
	soundsystem->Update();
	float measureresult = MEASURE_TIME_STATS(measuresound);

	// Engine frames status
	static float accTime = 0.1f;
	static int fps = 0;
	static int nFrames = 0;

	if (accTime > 0.1f)
	{
		fps = (int) (nFrames / accTime + 0.5f);
		nFrames = 0;
		accTime = 0;
	}

	accTime += m_fFrameTime;
	nFrames++;

	debugoverlay->Text(Vector4D(1,1,0,1), "-----ENGINE STATISTICS-----");
	debugoverlay->Text(Vector4D(1), "Engine framerate: %i", fps);
	debugoverlay->Text(Vector4D(1), "DPS/DIPS: %i/%i", g_pShaderAPI->GetDrawCallsCount(), g_pShaderAPI->GetDrawIndexedPrimitiveCallsCount());
	debugoverlay->Text(Vector4D(1), "primitives: %i", g_pShaderAPI->GetTrianglesCount());
	debugoverlay->Text(Vector4D(1), "sound: %g ms", measureresult);
	debugoverlay->Text(Vector4D(1,1,1,1), "engtime: %.2f\n", m_fCurTime);

	if(m_nLoadingResourceTimes)
		debugoverlay->Text(Vector4D(1,0,0,1), "WARNING! Resource loading is not ended (%d)!\n", m_nLoadingResourceTimes);

	// every new frame unlocks the mouse
	m_bDisableMouseUpdatePerFrame = false;

	return 1;
}

void CEngineHost::BeginScene()
{
	// reset viewport
	g_pShaderAPI->SetViewport(0,0, m_nWidth, m_nHeight);

	// Begin frame from render lib
	materials->BeginFrame();
}

void CEngineHost::EndScene()
{
	// End frame from render lib
	materials->EndFrame(NULL);
}

void CEngineHost::SetWindowSize(int width, int height)
{
	m_nWidth = width;
	m_nHeight = height;

	if(materials)
	{
		materials->SetDeviceBackbufferSize(width,height);
		// Platform_Sleep(1000);
	}
}

IVector2D CEngineHost::GetWindowSize()
{
	return IVector2D(m_nWidth,m_nHeight);
}

void CEngineHost::TrapKey_Event( int key, bool down )
{
	if(m_nLoadingResourceTimes > 0)
		return;

	// Only key down events are trapped
	if ( m_bTrapMode && down )
	{
		m_bTrapMode			= false;
		m_bDoneTrapping		= true;
		
		m_nTrapKey			= key;
		m_nTrapButtons		= 0;
		return;
	}

	// if console doesn't blocks key pressing
	if(!g_pSysConsole->KeyPress(key,down))
		engine->Key_Event(key, down);
}

void CEngineHost::TrapMouse_Event( float x, float y, int buttons, bool down )
{
	if(m_nLoadingResourceTimes > 0)
		return;

	// buttons == 0 for mouse move events
	if ( m_bTrapMode && buttons && !down )
	{
		m_bTrapMode			= false;
		m_bDoneTrapping		= true;
		
		m_nTrapKey			= 0;
		m_nTrapButtons		= buttons;
		return;
	}

	g_pSysConsole->MouseEvent(Vector2D(x,y),buttons,down);

	if(!g_pSysConsole->IsVisible())
		engine->Mouse_Event( x,y, buttons, down);
}

void CEngineHost::TrapMouseMove_Event( int x, int y, float deltaX, float deltaY )
{
	if(m_nLoadingResourceTimes > 0)
		return;

	if(m_bDisableMouseUpdatePerFrame)
		return;

	//m_bDisableMouseUpdatePerFrame = true;

	g_pSysConsole->MousePos(Vector2D(x,y));

	m_vCursorPosition = IVector2D(x,y);

	float xChange = (m_invert.GetBool() ? 1.0f : -1) * (GetWindowSize().y / 2.0f - y);
	float yChange = (GetWindowSize().x / 2.0f - x);

	xChange *= 0.01f;
	yChange *= 0.01f;

	if(!g_pSysConsole->IsVisible())
		engine->MouseMove_Event(x, y, xChange, yChange);
}

void CEngineHost::TrapMouseWheel_Event(int x, int y, int scroll)
{
	if(m_nLoadingResourceTimes > 0)
		return;

	engine->MouseWheel_Event(x, y, scroll);
}

void CEngineHost::ProcessKeyChar( ubyte chr )
{
	if(m_nLoadingResourceTimes > 0)
		return;

	g_pSysConsole->KeyChar(chr);
	engine->ProcessKeyChar(chr);
}
