//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: System and module loader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IEqCPUServices.h"
#include "core/IEqParallelJobs.h"
#include "core/IFileSystem.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"

#include "imaging/ImageLoader.h"

#include "render/IDebugOverlay.h"

#include <SDL.h>
#include <SDL_syswm.h>

#include "sys_host.h"
#include "sys_state.h"
#include "sys_in_console.h"
#include "sys_in_joystick.h"
#include "sys_window.h"
#include "sys_version.h"
#include "cfgloader.h"

#include "font/IFontCache.h"
#include "equi/EqUI_Manager.h"
#include "input/InputCommandBinder.h"

#include "materialsystem1/IMaterialSystem.h"

#define DEFAULT_USERCONFIG_PATH		"cfg/user.cfg"

DECLARE_CVAR_G(user_cfg, DEFAULT_USERCONFIG_PATH, "User configuration file location", CV_PROTECTED);

DECLARE_CMD(exec_user_cfg, "Executes user configuration file (from cvar 'cfg_user' value)", 0)
{
	MsgInfo("* Executing user configuration file '%s'\n", user_cfg.GetString());
	g_consoleCommands->ParseFileToCommandBuffer( user_cfg.GetString() );
}

DECLARE_CMD_FN_RENAME(cmd_exit, "exit", CGameHost::HostExitCmd, "Cl`oses current instance of engine", 0);
DECLARE_CMD_FN_RENAME(cmd_quit, "quit", CGameHost::HostExitCmd, "Closes current instance of engine", 0);
DECLARE_CMD_FN_RENAME(cmd_quti, "quti", CGameHost::HostExitCmd, "This made for keyboard writing errors", 0);

DECLARE_CVAR(r_antialiasing, "0", "Multisample antialiasing", CV_ARCHIVE);
DECLARE_CVAR(r_showFPS, "0", "Show the framerate", CV_ARCHIVE);
DECLARE_CVAR(r_showFPSGraph, "0", "Show the framerate graph", CV_ARCHIVE);

DECLARE_CVAR(r_overdraw, "0", "Renders all materials in overdraw shader", CV_CHEAT);
DECLARE_CVAR(r_wireframe, "0", "Enables wireframe rendering", CV_CHEAT);

DECLARE_CVAR(sys_screen, "0", "Fullscreen mode screen ID", CV_ARCHIVE);
DECLARE_CVAR(sys_vmode, "1024x768", "Screen Resoulution. Resolution string format: WIDTHxHEIGHT", CV_ARCHIVE);
DECLARE_CVAR(sys_fullscreen, "0", "Enable fullscreen mode on startup", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(sys_maxfps, "0", 0.0f, 300.0f, "Frame rate limit", CV_ARCHIVE);
DECLARE_CVAR(sys_timescale, "1.0", "Frame time scale factor", CV_CHEAT);

DECLARE_CVAR(m_sensitivity, "1.0", "mouse sensitivity", CV_ARCHIVE);
DECLARE_CVAR(m_clicks_to_touch, "0", "Convert mouse clicks to touch input", CV_ARCHIVE);
DECLARE_CVAR(m_invert, 0, "Mouse inversion enabled?", CV_ARCHIVE);

DECLARE_CMD(sys_set_fullscreen, nullptr, 0)
{
	g_pHost->SetFullscreenMode(true);
}

DECLARE_CMD(sys_set_windowed, nullptr, 0)
{
	g_pHost->SetWindowedMode();
}

DECLARE_CMD(sys_vmode_list, nullptr, 0)
{
	Array<VideoMode_t> vmodes(PP_SL);
	g_pHost->GetVideoModes(vmodes);

	for(int i = 0; i < vmodes.numElem(); i++)
	{
		Msg("display: %d %d bpp %d x %d @ %dHz\n",
			vmodes[i].displayId,
			vmodes[i].bitsPerPixel, vmodes[i].width, vmodes[i].height, vmodes[i].refreshRate);
	}
}

DECLARE_CVAR(screenshotJpegQuality, "100", "JPEG Quality", CV_ARCHIVE);

static EqString requestScreenshotName;

DECLARE_CMD(screenshot, "Save screenshot", 0)
{
	if (g_matSystem == nullptr)
		return;

	// pick the best filename
	if (CMD_ARGC == 0)
	{
		int i = 0;
		do
		{
			g_fileSystem->MakeDir("screenshots", SP_ROOT);
			EqString path(EqString::Format("screenshots/screenshot_%04d.jpg", i));

			if (g_fileSystem->FileExist(path.ToCString(), SP_ROOT))
				continue;

			CombinePath(requestScreenshotName, g_fileSystem->GetBasePath(), path.ToCString());
			break;
		} while (i++ < 9999);
	}
	else
	{
		requestScreenshotName = CMD_ARGV(0) + ".jpg";
	}
}

static void Sys_SaveScreenshot()
{
	if (!requestScreenshotName.Length())
		return;

	g_matSystem->SubmitQueuedCommands();

	CImage img;
	if (!g_matSystem->CaptureScreenshot(img))
		return;

	if (img.SaveJPEG(requestScreenshotName, screenshotJpegQuality.GetInt()))
		MsgInfo("Saved screenshot to '%s'\n", requestScreenshotName.ToCString());
	else
		MsgError("Failed to save or capture screenshot\n");

	requestScreenshotName.Empty();
}

DECLARE_INTERNAL_SHADERS();

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

static EQCURSOR s_defaultCursor[20];

CStaticAutoPtr<CGameHost> g_pHost;
SyncJob*			g_beginSceneJob = nullptr;
SyncJob*			g_endSceneJob = nullptr;

static DKMODULE*	g_matsysmodule = nullptr;
IMaterialSystem*	g_matSystem = nullptr;
IShaderAPI*			g_renderAPI = nullptr;

void CGameHost::HostQuitToDesktop()
{
	g_pHost->m_quitState = CGameHost::QUIT_TODESKTOP;
}

void CGameHost::HostExitCmd(CONCOMMAND_ARGUMENTS)
{
	HostQuitToDesktop();
}

bool CGameHost::LoadModules()
{
	// first, load matsystem module
	EqString loadErr;
	g_matsysmodule = g_fileSystem->OpenModule("eqMatSystem", &loadErr);

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load eqMatSystem - %s", loadErr.ToCString());
		return false;
	}

	g_matSystem = g_eqCore->GetInterface<IMaterialSystem>();
	if(!g_matSystem)
	{
		ErrorMsg("ERROR! Couldn't get interface of eqMatSystem!");
		return false;
	}

	return true;
}

void CGameHost::SetWindowTitle(const char* windowTitle)
{
#ifdef _RETAIL
	SDL_SetWindowTitle(m_window, windowTitle);
#else
	EqString str = EqString::Format("%s | " COMPILE_CONFIGURATION " (" COMPILE_PLATFORM ") | build %d (" COMPILE_DATE ")", windowTitle, BUILD_NUMBER_ENGINE);
	SDL_SetWindowTitle(m_window, str);
#endif
}

bool CGameHost::IsWindowed() const
{
	return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) == 0;
}

void CGameHost::SetFullscreenMode(bool screenSize)
{
	int nAdjustedWide;
	int nAdjustedTall;

	if (screenSize)
	{
		SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_GetWindowSize(m_window, &nAdjustedWide, &nAdjustedTall);

		Msg("Set %dx%d mode (fullscreen)\n", nAdjustedWide, nAdjustedTall);

		OnWindowResize(nAdjustedWide, nAdjustedTall);
	}
	else
	{
		const char *str = sys_vmode.GetString();
		Array<EqString> args(PP_SL);
		xstrsplit(str, "x", args);

		nAdjustedWide = atoi(args[0].GetData());
		nAdjustedTall = atoi(args[1].GetData());

		OnWindowResize(nAdjustedWide, nAdjustedTall);

		if (g_matSystem->SetWindowed(false))
		{
			Msg("Set %dx%d mode (fullscreen)\n", nAdjustedWide, nAdjustedTall);

			SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
			SDL_SetWindowSize(m_window, nAdjustedWide, nAdjustedTall);
		}
	}
}

void CGameHost::SetWindowedMode()
{
	const char* str = sys_vmode.GetString();
	Array<EqString> args(PP_SL);
	xstrsplit(str, "x", args);

	int nAdjustedPosX = SDL_WINDOWPOS_CENTERED;
	int nAdjustedPosY = SDL_WINDOWPOS_CENTERED;
	int nAdjustedWide = atoi(args[0].GetData());
	int nAdjustedTall = atoi(args[1].GetData());

	OnWindowResize(nAdjustedWide, nAdjustedTall);
	if (g_matSystem->SetWindowed(true))
	{
		Msg("Set %dx%d mode (windowed)\n", nAdjustedWide, nAdjustedTall);

		SDL_SetWindowFullscreen(m_window, 0);
		SDL_SetWindowSize(m_window, nAdjustedWide, nAdjustedTall);
		SDL_SetWindowPosition(m_window, nAdjustedPosX, nAdjustedPosY);
	}
}

void CGameHost::ApplyVideoMode()
{
	if(sys_fullscreen.GetBool())
		SetFullscreenMode(false);
	else
		SetWindowedMode();
}

void CGameHost::GetVideoModes(Array<VideoMode_t>& displayModes)
{
#ifdef PLAT_ANDROID
	displayModes.append(VideoMode_t{ 0, 16, 1024, 768, 60 });
#else
	int display_count = SDL_GetNumVideoDisplays();

	for (int display_index = 0; display_index <= display_count; display_index++)
	{
		int modes_count = SDL_GetNumDisplayModes(display_index);

		for (int mode_index = 0; mode_index <= modes_count; mode_index++)
		{
			SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

			if (SDL_GetDisplayMode(display_index, mode_index, &mode) == 0)
				displayModes.append(VideoMode_t{display_index, SDL_BITSPERPIXEL(mode.format), mode.w, mode.h, mode.refresh_rate});
		}
	}
#endif
}

static void* Helper_GetWindowInfo(void* userData, RenderWindowInfo::Attribute attrib)
{
	// set window info
	SDL_Window* window = reinterpret_cast<SDL_Window*>(userData);
	SDL_SysWMinfo winfo;
	SDL_VERSION(&winfo.version); // initialize info structure with SDL version info
	if( !SDL_GetWindowWMInfo(window, &winfo) )
	{
		MsgError("SDL_GetWindowWMInfo failed %s\n\tWindow handle: %p", SDL_GetError(), window);
		return nullptr;
	}

	switch(winfo.subsystem)
	{
#if PLAT_LINUX
		case SDL_SYSWM_X11:
			switch(attrib)
			{
				case RenderWindowInfo::WINDOW:
					return (void*)winfo.info.x11.window;
				case RenderWindowInfo::DISPLAY:
					return (void*)winfo.info.x11.display;
			}
		case SDL_SYSWM_WAYLAND:
			switch(attrib)
			{
				case RenderWindowInfo::WINDOW:
					return (void*)winfo.info.wl.egl_window;
				case RenderWindowInfo::DISPLAY:
					return (void*)winfo.info.wl.display;
				case RenderWindowInfo::SURFACE:
					return (void*)winfo.info.wl.surface;
				case RenderWindowInfo::TOPLEVEL:
					return (void*)winfo.info.wl.xdg_toplevel;
			}
#endif // PLAT_LINUX
#if PLAT_ANDROID
		case SDL_SYSWM_ANDROID:
			switch(attrib)
			{
				case RenderWindowInfo::WINDOW:
					return (void*)winfo.info.android.window;
				case RenderWindowInfo::DISPLAY:
					return nullptr; // EGL_DEFAULT_DISPLAY
			}
#endif // PLAT_ANDROID
#if PLAT_WIN
		case SDL_SYSWM_WINDOWS:
			switch(attrib)
			{
				case RenderWindowInfo::WINDOW:
					return (void*)winfo.info.win.window;
				case RenderWindowInfo::DISPLAY:
					return (void*)winfo.info.win.hdc;
				case RenderWindowInfo::TOPLEVEL:
					return (void*)winfo.info.win.hinstance;
			}
#endif // PLAT_WIN
		default:
			ASSERT_FAIL("Not supported window type - %d", winfo.subsystem);
	}

	return nullptr;
}

bool CGameHost::InitSystems( EQWNDHANDLE pWindow )
{
	m_window = pWindow;

	MaterialsInitSettings materials_config;

	// set window info
	SDL_SysWMinfo winfo;
	SDL_VERSION(&winfo.version); // initialize info structure with SDL version info

	if( !SDL_GetWindowWMInfo(m_window, &winfo) )
	{
		MsgError("SDL_GetWindowWMInfo failed %s\n\tWindow handle: %p", SDL_GetError(), m_window);
		ErrorMsg("Can't get SDL window WM info!\n");
		return false;
	}

	s_defaultCursor[dc_none] = nullptr;
	s_defaultCursor[dc_arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	s_defaultCursor[dc_ibeam] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	s_defaultCursor[dc_hourglass] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	s_defaultCursor[dc_crosshair] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	s_defaultCursor[dc_up] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
	s_defaultCursor[dc_sizenwse] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	s_defaultCursor[dc_sizenesw] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	s_defaultCursor[dc_sizewe] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	s_defaultCursor[dc_sizens] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	s_defaultCursor[dc_sizeall] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	s_defaultCursor[dc_no] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
	s_defaultCursor[dc_hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

	// Set default cursor
	SDL_SetCursor(s_defaultCursor[dc_arrow]);

	int renderBPP = 32;

	{
		RenderWindowInfo &winInfo = materials_config.shaderApiParams.windowInfo;
		winInfo.userData = m_window;
		winInfo.get = Helper_GetWindowInfo;
		
		// needed for initialization
		switch(winfo.subsystem)
		{
			case SDL_SYSWM_X11:
				winInfo.windowType = RHI_WINDOW_HANDLE_NATIVE_X11;
				break;
			case SDL_SYSWM_WAYLAND:
				winInfo.windowType = RHI_WINDOW_HANDLE_NATIVE_WAYLAND;
				break;
			case SDL_SYSWM_ANDROID:
				winInfo.windowType = RHI_WINDOW_HANDLE_NATIVE_ANDROID;
				renderBPP = 16; // force 16 bit rendering on Android
				break;
			case SDL_SYSWM_WINDOWS:
				winInfo.windowType = RHI_WINDOW_HANDLE_NATIVE_WINDOWS;
				break;
			default:
				ASSERT_FAIL("Not supported window type - %d", winfo.subsystem);
		}
	}

	ETextureFormat screenFormat = FORMAT_RGB8;

	// Figure display format to use
	if(renderBPP == 32)
		screenFormat = FORMAT_RGB8;
	else if(renderBPP == 24)
		screenFormat = FORMAT_RGB8;
	else if(renderBPP == 16)
		screenFormat = FORMAT_RGB565;

	materials_config.shaderApiParams.screenFormat = screenFormat;

	g_parallelJobs->Init();

	if(!g_matSystem->Init(materials_config))
		return false;

	g_renderAPI = g_matSystem->GetShaderAPI();

	g_matSystem->LoadShaderLibrary("eqBaseShaders");

	// register all shaders
	REGISTER_INTERNAL_SHADERS();

	// init game states and proceed
	if (!EqStateMgr::InitRegisterStates())
		return false;

	// override configuration file by executing command line
	g_cmdLine->ExecuteCommandLine();

	if( !g_fontCache->Init() )
		return false;

	m_defaultFont = g_fontCache->GetFont("default",0);

	debugoverlay->Init();
	equi::Manager->Init();

	// finally init input and adjust bindings
	g_inputCommandBinder->Init();

	// init console
	g_consoleInput->Initialize(pWindow);

	MsgInfo("--- EqEngine systems init successfully ---\n");

#ifndef PLAT_ANDROID
	if (sys_fullscreen.GetBool())
		SetFullscreenMode(false);
	else
		SetWindowedMode();
#endif

	g_beginSceneJob = PPNew SyncJob("BeginSceneJob");
	g_endSceneJob = PPNew SyncJob("EndSceneJob");

	return true;
}

//--------------------------------------------------------------------------------------

void InputCommands_SDL(SDL_Event* event)
{
	static int lastX, lastY;

	switch(event->type)
	{
		case SDL_KEYUP:
		case SDL_KEYDOWN:
		{
			int nKey = event->key.keysym.scancode;

			static bool altState = false;
			if (nKey == SDL_SCANCODE_RALT)
			{
				altState = (event->key.type == SDL_KEYDOWN);
			}
			else if (nKey == SDL_SCANCODE_RETURN)
			{
				if (altState)
				{
					if (event->key.type == SDL_KEYDOWN)
					{
						if (g_pHost->IsWindowed())
							g_pHost->SetFullscreenMode(true);
						else
							g_pHost->SetWindowedMode();
					}
					return;
				}
			}

			// do translation
			if (nKey == SDL_SCANCODE_RSHIFT)
				nKey = SDL_SCANCODE_LSHIFT;
			else if (nKey == SDL_SCANCODE_RCTRL)
				nKey = SDL_SCANCODE_LCTRL;
			else if (nKey == SDL_SCANCODE_RALT)
				nKey = SDL_SCANCODE_LALT;
			else if (nKey == SDL_SCANCODE_AC_BACK)
				nKey = SDL_SCANCODE_ESCAPE;

			g_pHost->TrapKey_Event(nKey, (event->type == SDL_KEYUP) ? false : true);
			break;
		}
		case SDL_TEXTINPUT:
		{
			// FIXME: is it good?
			g_pHost->ProcessKeyChar( event->text.text );
			break;
		}
		case SDL_MOUSEMOTION:
		{
			const int x = event->motion.x;
			const int y = event->motion.y;
			const int dx = event->motion.xrel;
			const int dy = event->motion.yrel;

			g_pHost->TrapMouseMove_Event(x, y, dx, dy);

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

			g_pHost->TrapMouse_Event(x, y, mouseButton, (event->type == SDL_MOUSEBUTTONUP) ? false : true);

			break;
		}
		case SDL_FINGERMOTION:
		{
			g_pHost->TouchMotion_Event( event->tfinger.x,event->tfinger.y, event->tfinger.fingerId);
			break;
		}
		case SDL_FINGERUP:
		case SDL_FINGERDOWN:
		{
			g_pHost->Touch_Event( event->tfinger.x,event->tfinger.y, event->tfinger.fingerId, (event->type == SDL_FINGERUP) ? false : true);
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			int x, y;
			x = event->wheel.x;
			y = event->wheel.y;

			g_pHost->TrapMouseWheel_Event(lastX, lastY, x, y);

			break;
		}
		default:
		{
			CEqGameControllerSDL::ProcessInputEvent(event);
		}
	}
}

static DbgGraphBucket s_fpsGraph("Frames per sec", ColorRGB(1, 1, 0), 80.0f);

CGameHost::CGameHost() :
	m_winSize(0), m_prevMousePos(0), m_mousePos(0), m_window(nullptr), m_quitState(QUIT_NOTQUITTING),
	m_keyTrapMode(false), m_skipMouseMove(false), m_keyDoneTrapping(false), m_trapKey(0), m_trapButtons(0), m_cursorCentered(false),
	m_defaultFont(nullptr),
	m_accumTime(0.0)
{

}

void CGameHost::ShutdownSystems()
{
	Msg("---------  ShutdownSystems ---------\n");

	// calls OnLeave and unloads state
	EqStateMgr::ChangeState(nullptr);

	g_parallelJobs->Wait();

	// Save configuration before full unload
	WriteCfgFile( user_cfg.GetString(), true );
	
	debugoverlay->Shutdown();
	equi::Manager->Shutdown();
	g_fontCache->Shutdown();

	g_inputCommandBinder->Shutdown();

	EqStateMgr::ShutdownStates();

	g_consoleInput->Shutdown();

	g_matSystem->Shutdown();
	g_fileSystem->CloseModule( g_matsysmodule );

	g_parallelJobs->Shutdown();
	SAFE_DELETE(g_beginSceneJob);
	SAFE_DELETE(g_endSceneJob);

	SDL_DestroyWindow(g_pHost->m_window);
}

#define GAME_MAX_FRAMERATE (sys_maxfps.GetFloat()) // 60 fps limit

#define MIN_FPS 0.01
#define MAX_FPS 10000.0

#define MAX_FRAMETIME	0.1
#define MIN_FRAMETIME	0.001

bool CGameHost::FilterTime( double dTime )
{
	// Accumulate some time
	double frameTime = m_accumTime + dTime;

	// limit to minimal frame time
	frameTime = min(frameTime, MAX_FRAMETIME);
	m_accumTime = max(frameTime, MIN_FRAMETIME);

	double fps = GAME_MAX_FRAMERATE;
	if ( fps != 0 )
	{
		// Limit fps to withing tolerable range
		fps = max( MIN_FPS, fps );
		fps = min( MAX_FPS, fps );

		double minframetime = 1.0 / fps;

		if (frameTime < minframetime)
			return false;
	}

	return true;
}

void CGameHost::UpdateCursorState()
{
	bool cursorVisible = false;
	bool center = false;
	EqStateMgr::GetStateMouseCursorProperties(cursorVisible, center);

	bool previousCenterState = m_cursorCentered;

	cursorVisible = cursorVisible || g_consoleInput->IsVisible() || equi::Manager->IsWindowsVisible();
	m_cursorCentered = center && !(g_consoleInput->IsVisible() || equi::Manager->IsWindowsVisible());

	SDL_SetRelativeMouseMode(m_cursorCentered ? SDL_TRUE : SDL_FALSE);

	if (previousCenterState = previousCenterState != m_cursorCentered)
		SDL_WarpMouseInWindow(m_window, m_winSize.x / 2, m_winSize.y / 2);

	// update cursor visibility state
	SetCursorShow( cursorVisible );
}

void CGameHost::SetCursorPosition(int x, int y)
{
	SDL_WarpMouseInWindow(m_window, x, y);
	m_mousePos = IVector2D(x,y);
}

void CGameHost::SetCursorShow(bool bShow)
{
	const bool state = SDL_ShowCursor(-1);

	if(state == bShow)
		return;

	if(bShow)
		SDL_SetCursor(s_defaultCursor[dc_arrow]);

	SDL_ShowCursor(bShow);
}

bool CGameHost::Frame()
{
	double elapsedTime = m_timer.GetTime(true);

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

	accTime += elapsedTime;
	nFrames++;

	m_prevMousePos = m_mousePos;

	if (!FilterTime(elapsedTime))
		return false;

	PROF_EVENT("Host Frame");

	double gameFrameTime = m_accumTime;

	CEqGameControllerSDL::RepeatEvents(gameFrameTime);

	UpdateCursorState();

	//--------------------------------------------

	BeginScene();

	if (r_showFPSGraph.GetBool())
		debugoverlay->Graph_DrawBucket(&s_fpsGraph);

	const double timescale = (EqStateMgr::GetCurrentState() ? EqStateMgr::GetCurrentState()->GetTimescale() : 1.0f);
	
	const float stateTimeStepDelta = gameFrameTime * timescale * sys_timescale.GetFloat();
	g_matSystem->SetProxyDeltaTime(stateTimeStepDelta);

	if(!EqStateMgr::UpdateStates(stateTimeStepDelta))
	{
		m_quitState = CGameHost::QUIT_TODESKTOP;
		return false;
	}

	// submit all command buffers queued inside UpdateStates
	// this is made to display Debug Overlays and, console input 
	// and ImGui in case of some command buffers were invalid
	g_matSystem->SubmitQueuedCommands();

	debugoverlay->Text(Vector4D(1, 1, 0, 1), "-----ENGINE STATISTICS-----");

	// EqUI, console and debug stuff should be drawn as normal in overdraw mode
	// this also resets matsystem from overdraw
	g_matSystem->GetConfiguration().overdrawMode = false;

	// Engine frames status
	static float gameAccTime = 0.1f;
	static int gamefps = 0;
	static int nGameFrames = 0;

	if (gameAccTime > 0.1f)
	{
		gamefps = (int) (nGameFrames / gameAccTime + 0.5f);
		nGameFrames = 0;
		gameAccTime = 0;
	}

	gameAccTime += gameFrameTime;
	nGameFrames++;

	// game fps graph
	if (r_showFPSGraph.GetBool())
	{
		if (gamefps > 40)
			s_fpsGraph.color = ColorRGB(0, 1, 0);
		else if (gamefps > 25)
			s_fpsGraph.color = ColorRGB(1, 1, 0);
		else
			s_fpsGraph.color = ColorRGB(1, 0, 0);

		s_fpsGraph.maxValue = sys_maxfps.GetFloat();

		debugoverlay->Graph_AddValue(&s_fpsGraph, gamefps);
	}

	debugoverlay->Text(color_white, "System framerate: %i", fps);
	debugoverlay->Text(color_white, "Game framerate: %i (ft=%g)", gamefps, gameFrameTime);
	debugoverlay->Text(color_white, "DPS/DIPS: %i/%i", g_renderAPI->GetDrawCallsCount(), g_renderAPI->GetDrawIndexedPrimitiveCallsCount());
	debugoverlay->Text(color_white, "primitives: %i", g_renderAPI->GetTrianglesCount());
	
	debugoverlay->Draw(m_winSize.x, m_winSize.y, timescale * sys_timescale.GetFloat());

	g_matSystem->Setup2D(m_winSize.x, m_winSize.y);

	equi::Manager->SetViewFrame(IAARectangle(0,0,m_winSize.x,m_winSize.y));
	equi::Manager->Render();

	IGPURenderPassRecorderPtr rendPassRecorder = g_renderAPI->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(g_matSystem->GetCurrentBackbuffer())
		.End()
	);

	if (r_showFPS.GetBool())
	{
		eqFontStyleParam_t params;
		params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
		params.textColor = ColorRGBA(1, 1, 1, 1);

		if (fps < 30)
			params.textColor = ColorRGBA(1, 0, 0, 1);
		else if (fps < 60)
			params.textColor = ColorRGBA(1, 0.8f, 0, 1);

		m_defaultFont->SetupRenderText(EqString::Format("SYS/GAME FPS: %d/%d", min(fps, 1000), gamefps).ToCString(), Vector2D(15), params, rendPassRecorder);

		size_t totalMem = PPMemGetUsage();
		if (totalMem)
		{
			eqFontStyleParam_t memParams;
			memParams.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
			m_defaultFont->SetupRenderText(EqString::Format("MEM: %.2f", (totalMem / 1024.0f) / 1024.0f).ToCString(), Vector2D(15, 35), memParams, rendPassRecorder);
		}
	}

	g_inputCommandBinder->DebugDraw(m_winSize, rendPassRecorder);
	g_matSystem->QueueCommandBuffer(rendPassRecorder->End());

	// End frame from render lib
	EndScene();

	m_accumTime = 0.0f;

	return true;
}

bool CGameHost::IsInMultiplayerGame() const
{
	return EqStateMgr::IsMultiplayerGameState();
}

void CGameHost::SignalPause()
{
	if(EqStateMgr::IsInGameState())
		return;

	EqStateMgr::SignalPause();
}

void CGameHost::OnWindowResize(int width, int height)
{
	m_winSize.x = width;
	m_winSize.y = height;

	if(g_matSystem)
		g_matSystem->SetDeviceBackbufferSize( width, height );
}

void CGameHost::OnFocusChanged(bool inFocus)
{
	g_matSystem->SetDeviceFocused(inFocus);
}

void CGameHost::BeginScene()
{
	g_matSystem->BeginFrame(nullptr);

	MatSysRenderSettings& rendSettings = g_matSystem->GetConfiguration();
	rendSettings.wireframeMode = r_wireframe.GetBool();
	rendSettings.overdrawMode = r_overdraw.GetBool();

	g_consoleInput->BeginFrame();

	g_parallelJobs->AddJob(g_beginSceneJob);
}

void CGameHost::EndScene()
{
	g_parallelJobs->AddJob(g_endSceneJob);

	// save screenshots without ImGui/Console visible
	Sys_SaveScreenshot();

	g_consoleInput->EndFrame(m_winSize.x, m_winSize.y, GetFrameTime());
	g_matSystem->EndFrame();
}

void CGameHost::RequestTextInput()
{
	SDL_StartTextInput();
}

void CGameHost::ReleaseTextInput()
{
	SDL_StopTextInput();
}

bool CGameHost::IsTextInputShown() const
{
	return SDL_IsTextInputActive();
}

void CGameHost::TrapKey_Event( int key, bool down )
{
	// Only key down events are trapped
	if ( m_keyTrapMode && down )
	{
		m_keyTrapMode			= false;
		m_keyDoneTrapping		= true;

		m_trapKey			= key;
		m_trapButtons		= 0;
		return;
	}

	if(g_consoleInput->KeyPress( key, down ))
		return;

	if( equi::Manager->ProcessKeyboardEvents(key, down ? equi::UIEVENT_DOWN : equi::UIEVENT_UP ) )
		return;

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleKeyPress( key, down );
}

void CGameHost::TrapMouse_Event( float x, float y, int buttons, bool down )
{
	// buttons == 0 for mouse move events
	if ( m_keyTrapMode && buttons && !down )
	{
		m_keyTrapMode			= false;
		m_keyDoneTrapping		= true;

		m_trapKey			= 0;
		m_trapButtons		= buttons;
		return;
	}

	if( g_consoleInput->MouseEvent( Vector2D(x,y), buttons, down ) )
		return;

	if( equi::Manager->ProcessMouseEvents( x, y, buttons, down ? equi::UIEVENT_DOWN : equi::UIEVENT_UP) )
		return;

	if(m_clicks_to_touch.GetBool())
		g_pHost->Touch_Event( x/m_winSize.x, y/m_winSize.y, 0, down);

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleMouseClick( x, y, buttons, down );
}

void CGameHost::TrapMouseMove_Event(int x, int y, int dx, int dy)
{
	if(m_skipMouseMove)
	{
		m_skipMouseMove = false;
		return;
	}

	m_mousePos = IVector2D(x,y);

	g_consoleInput->MousePos( m_mousePos );

	if( equi::Manager->ProcessMouseEvents(x, y, 0, equi::UIEVENT_MOUSE_MOVE) )
		return;

	Vector2D delta(-dx, -dy);
	delta.y *= (m_invert.GetBool() ? 1.0f : -1.0f);
	delta *= 0.05f * m_sensitivity.GetFloat();

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleMouseMove(x, y, delta.x, delta.y);
}

void CGameHost::TrapMouseWheel_Event(int x, int y, int hscroll, int vscroll)
{
	if (g_consoleInput->MouseWheel(hscroll, vscroll))
		return;

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleMouseWheel(x, y, vscroll);
}

void CGameHost::TrapJoyAxis_Event( short axis, short value )
{
	g_inputCommandBinder->OnJoyAxisEvent( axis, value );

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleJoyAxis( axis, value );
}

void CGameHost::TrapJoyButton_Event( short button, bool down)
{
	g_inputCommandBinder->OnKeyEvent( JOYSTICK_START_KEYS + button, down );

	if(EqStateMgr::GetCurrentState())
		EqStateMgr::GetCurrentState()->HandleKeyPress( JOYSTICK_START_KEYS + button, down );
}

void CGameHost::TouchMotion_Event( float x, float y, int finger )
{
	// TODO: uses this!
}

void CGameHost::Touch_Event( float x, float y, int finger, bool down )
{
	g_inputCommandBinder->OnTouchEvent( Vector2D(x,y), finger, down );
}

void CGameHost::ProcessKeyChar(const char* utfChar)
{
	if(g_consoleInput->KeyChar(utfChar))
		return;

	// TODO: EqUI text input processing
}

void CGameHost::StartTrapMode()
{
	if ( m_keyTrapMode )
		return;

	m_keyDoneTrapping = false;
	m_keyTrapMode = true;
}

// Returns true on success, false on failure.
bool CGameHost::IsTrapping()
{
	return m_keyTrapMode;
}

bool CGameHost::CheckDoneTrapping( int& buttons, int& key )
{
	if ( m_keyTrapMode )
		return false;

	if ( !m_keyDoneTrapping )
		return false;

	key			= m_trapKey;
	buttons		= m_trapButtons;

	// Reset since we retrieved the results
	m_keyDoneTrapping = false;
	return true;
}
