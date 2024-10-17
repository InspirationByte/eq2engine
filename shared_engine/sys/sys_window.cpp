//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: 
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include <SDL.h>

#include "sys_version.h"
#include "sys_host.h"
#include "sys_in_console.h"
#include "sys_in_joystick.h"

#include "materialsystem1/IMaterialSystem.h"

// Renderer
DECLARE_CVAR(sys_sleep, "1", "Sleep time for every frame", CV_ARCHIVE);
DECLARE_CVAR(vid_screen, "0", "Fullscreen mode screen ID", CV_ARCHIVE);
DECLARE_CVAR(vid_mode, "1024x768", "Screen Resoulution. Resolution string format: WIDTHxHEIGHT", CV_ARCHIVE);
DECLARE_CVAR(vid_fullscreen, "0", "Enable fullscreen mode on startup", CV_ARCHIVE);

#define DEFAULT_WINDOW_TITLE "Initializing..."

void Sys_GetWindowConfig(bool& fullscreen, int& screen, int& wide, int& tall)
{
	const char* str = vid_mode.GetString();

	Array<EqString> args(PP_SL);
	StringSplit(str, "x", args);

	wide = atoi(args[0].GetData());
	tall = atoi(args[1].GetData());

	screen = vid_screen.GetInt();
	fullscreen = vid_fullscreen.GetBool();
}

EQWNDHANDLE Sys_CreateWindow()
{
	EQWNDHANDLE handle = nullptr;

#ifdef PLAT_SDL

	int adjustedPosX = SDL_WINDOWPOS_CENTERED;
	int adjustedPosY = SDL_WINDOWPOS_CENTERED;
	int adjustedWide = 800;
	int adjustedTall = 600;
	int screen = 0;
	bool fullscreen = false;
	Sys_GetWindowConfig(fullscreen, screen, adjustedWide, adjustedTall);

	int sdlFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (fullscreen)
		sdlFlags |= SDL_WINDOW_FULLSCREEN;

#ifdef PLAT_ANDROID
	adjustedPosX = adjustedPosY = SDL_WINDOWPOS_UNDEFINED;

	//Get device display mode
	SDL_Rect displayRect;
	if (SDL_GetDisplayBounds(0, &displayRect) == 0)
	{
		adjustedWide = displayRect.w;
		adjustedTall = displayRect.h;
	}

	sdlFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN; // SDL on android WUT
#endif // PLAT_ANDROID

	SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "1");
	
	handle = SDL_CreateWindow(DEFAULT_WINDOW_TITLE, adjustedPosX, adjustedPosY, adjustedWide, adjustedTall, sdlFlags);
	if(handle == nullptr)
	{
		ASSERT_MSG("Can't create window!\n%s\n",SDL_GetError());
		return nullptr;
	}

#endif // PLAT_SDL

	return handle;
}

bool s_bActive = true;
bool s_bProcessInput = true;

void InputCommands_SDL(SDL_Event* event);

void Host_HandleSDLEvents(SDL_Event* event)
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
		ASSERT_MSG( "Failed to init SDL system: %s\n", SDL_GetError());
		return false;
	}

	if(!g_pHost->LoadModules())
		return false;

	if(!g_pHost->InitSystems())
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
			Host_HandleSDLEvents( &event );

		if (s_bActive || g_pHost->IsPauseAllowed())
		{
			g_pHost->Frame();
		}
		else
		{
			g_pHost->SignalPause();
			Platform_Sleep( 1 );
			Threading::YieldCurrentThread();
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
