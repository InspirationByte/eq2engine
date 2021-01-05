//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers window handler
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"

#include "sys_host.h"

#include "sys_in_console.h"
#include "sys_in_joystick.h"

#include "imaging/ImageLoader.h"
#include "IConCommandFactory.h"
#include "utils/strtools.h"
#include "utils/eqthread.h"

#include "EngineVersion.h"
/*
#ifdef _WIN32
#include "Mmsystem.h"
#include "Resources/resource.h"
#endif*/

#define DEFAULT_CONFIG_PATH			"cfg/config_default.cfg"

// Renderer
DECLARE_CVAR(r_mode,1024x768,"Screen Resoulution. Resolution string format: WIDTHxHEIGHT" ,CV_ARCHIVE);
DECLARE_CVAR(r_bpp,32,"Screen bits per pixel",CV_ARCHIVE);
DECLARE_CVAR(sys_sleep,0,"Sleep time for every frame",CV_ARCHIVE);

DECLARE_CVAR(screenshotJpegQuality,100,"JPEG Quality",CV_ARCHIVE);

DECLARE_CMD(vid_listmodes, "Shows available video modes for your screen", 0)
{
	int display_count = SDL_GetNumVideoDisplays();
	Msg("--- number of displays: %d ---", display_count);

	for (int i = 0; i < display_count; i++)
	{
		Msg("* Display %d:", i);
		int modes_count = SDL_GetNumDisplayModes(i);

		for (int modeIdx = 0; modeIdx < modes_count; modeIdx++)
		{
			SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

			if (SDL_GetDisplayMode(i, modeIdx, &mode) == 0)
			{
				Msg("  %dx%d %dbpp @ %dHz",
					mode.w, mode.h, SDL_BITSPERPIXEL(mode.format), mode.refresh_rate);
			}
		}
	}
}

DECLARE_CMD(screenshot, "Save screenshot", 0)
{
	if(materials == NULL)
		return;

	CImage img;
	if( !materials->CaptureScreenshot(img) )
		return;

	// pick the best filename
	if(CMD_ARGC == 0)
	{
		int i = 0;
		do
		{
			g_fileSystem->MakeDir("screenshots", SP_ROOT);
			EqString path(varargs("screenshots/screenshot_%04d.jpg", i));

			if(g_fileSystem->FileExist(path.c_str(), SP_ROOT))
				continue;

			MsgInfo("Writing screenshot to '%s'\n", path.c_str());
			img.SaveJPEG(path.c_str(), screenshotJpegQuality.GetInt());
			break;
		}
		while (i++ < 9999);
	}
	else
	{
		EqString path(CMD_ARGV(0) + ".jpg");
		img.SaveJPEG(path.c_str(), screenshotJpegQuality.GetInt());
	}
}

ConVar r_fullscreen("r_fullscreen", "0", "Fullscreen" ,CV_ARCHIVE);

#define DEFAULT_WINDOW_TITLE "Initializing..."

EQWNDHANDLE Sys_CreateWindow()
{
	Msg(" \n--------- CreateWindow --------- \n");

	bool isWindowed = !r_fullscreen.GetBool();

	const char *str = r_mode.GetString();
	DkList<EqString> args;
	xstrsplit(str, "x", args);

	int nAdjustedPosX = SDL_WINDOWPOS_CENTERED;
	int nAdjustedPosY = SDL_WINDOWPOS_CENTERED;
	int nAdjustedWide = atoi(args[0].GetData());
	int nAdjustedTall = atoi(args[1].GetData());

	EQWNDHANDLE handle = NULL;

#ifdef PLAT_SDL

	int sdlFlags = SDL_WINDOW_SHOWN;

#ifdef ANDROID
	nAdjustedPosX = nAdjustedPosY = SDL_WINDOWPOS_UNDEFINED;

	//Get device display mode
	SDL_Rect displayRect;
	if (SDL_GetDisplayBounds(0, &displayRect) == 0)
	{
		nAdjustedWide = displayRect.w;
		nAdjustedTall = displayRect.h;
	}

	// HACK: use SDL_WINDOW_VULKAN to ensure that SDL will not create EGL surface
	sdlFlags |= SDL_WINDOW_FULLSCREEN;
#else

	if (isWindowed)
	{
		sdlFlags |= SDL_WINDOW_RESIZABLE;
	}
	else
	{
		sdlFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
	}
#endif // ANDROID
	
	handle = SDL_CreateWindow(DEFAULT_WINDOW_TITLE, nAdjustedPosX, nAdjustedPosY, nAdjustedWide, nAdjustedTall, sdlFlags);

	if(handle == NULL)
	{
		ErrorMsg("Can't create window!\n%s\n",SDL_GetError());
		return NULL;
	}

#endif // PLAT_SDL

	Msg("Created render window, %dx%d\n", nAdjustedWide, nAdjustedTall);

	return handle;
}

bool s_bActive = true;
bool s_bProcessInput = true;

static CGameHost s_Host;
CGameHost* g_pHost = &s_Host;

void InputCommands_SDL(SDL_Event* event);

void EQHandleSDLEvents(SDL_Event* event)
{
	switch (event->type)
	{
		case SDL_APP_TERMINATING:
		case SDL_QUIT:
		{
			CGameHost::HostQuitToDesktop();

			break;
		}
		case SDL_WINDOWEVENT:
		{
			if(event->window.event == SDL_WINDOWEVENT_CLOSE)
			{
				CGameHost::HostQuitToDesktop();
			}
			else if(event->window.event == SDL_WINDOWEVENT_RESIZED)
			{
				if(event->window.data1 > 0 && event->window.data2 > 0)
					g_pHost->OnWindowResize(event->window.data1, event->window.data2);
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				g_pHost->OnFocusChanged(true);
				s_bActive = true;
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				g_pHost->OnFocusChanged(false);
				s_bActive = false;
			}

			break;
		}
		default:
		{
			CEqGameControllerSDL::ProcessConnectionEvent(event);

			if(s_bActive && s_bProcessInput)
				InputCommands_SDL(event);
		}
	}
}

//
// Initializes engine system
//
bool Host_Init()
{
	if( SDL_Init(SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) != 0)
	{
		ErrorMsg( "Failed to init SDL system: %s\n", SDL_GetError());
		return false;
	}

	if(!g_pHost->LoadModules())
		return false;

	int userCfgIdx = g_cmdLine->FindArgument("-user_cfg");

	if(userCfgIdx != -1)
	{
		extern ConVar user_cfg;

		EqString cfgFileName(g_cmdLine->GetArgumentsOf(userCfgIdx));

		user_cfg.SetValue(cfgFileName.TrimChar('\"').c_str());
	}

	// execute configuration files and command line after all libraries are loaded.
	g_sysConsole->ClearCommandBuffer();
	g_sysConsole->ParseFileToCommandBuffer( DEFAULT_CONFIG_PATH );
	g_sysConsole->ExecuteCommandBuffer();

	EQWNDHANDLE mainWindow = Sys_CreateWindow();

	if(!g_pHost->InitSystems( mainWindow, !r_fullscreen.GetBool() ))
		return false;

	CEqGameControllerSDL::Init();

	return true;
}

//
// Runs game engine loop
//
void Host_GameLoop()
{
	SDL_Event event;

	do
	{
		while(SDL_PollEvent(&event))
			EQHandleSDLEvents( &event );

		if (s_bActive || g_pHost->IsInMultiplayerGame())
		{
			g_pHost->Frame();
		}
		else
		{
			g_pHost->SignalPause();
			Platform_Sleep( 1 );
			Threading::Yield();
		}

		// or yield
		if(sys_sleep.GetInt() > 0)
            Platform_Sleep( sys_sleep.GetInt() );
	}
	while(g_pHost->GetQuitState() != CGameHost::QUIT_TODESKTOP);
}

//
// Shutdowns all system in order
//
void Host_Terminate()
{
	CEqGameControllerSDL::Shutdown();

	g_pHost->ShutdownSystems();

	SDL_Quit();
}
