//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers window handler
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "system.h"
#include "sys_console.h"
#include "imaging/ImageLoader.h"
#include "IConCommandFactory.h"
#include "utils/strtools.h"

#include "EngineVersion.h"

#ifdef _WIN32
#include "Mmsystem.h"
#endif

#define GAME_WINDOW_TITLE "The Driver Syndicate" //varargs("Driver Syndicate Alpha [%s] build %d", __DATE__, GetEngineBuildNumber())

// Renderer
DECLARE_CVAR(r_mode,1024x768,"Screen Resoulution. Resolution string format: WIDTHxHEIGHT" ,CV_ARCHIVE);
DECLARE_CVAR(r_bpp,32,"Screen bits per pixel",CV_ARCHIVE);
DECLARE_CVAR(sys_sleep,0,"Sleep time for every frame",CV_ARCHIVE);

DECLARE_CVAR(screenshotJpegQuality,100,"JPEG Quality",CV_ARCHIVE);

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

DECLARE_CMD(kv3test, "Test", 0)
{
	kvkeybase_t destSec;
	long fileSize = 0;
	char* dataStr = g_fileSystem->GetFileBuffer("kv3test.txt", &fileSize, SP_ROOT);

	if(dataStr)
	{
		KV_ParseSectionV3(dataStr, fileSize, "test", &destSec);

		// look how it's been parsed
		KV_PrintSection( &destSec );

		PPFree(dataStr);
	}
	else
		MsgError("Can't open kv3test.txt\n");
}

ConVar r_fullscreen("r_fullscreen", "0", "Fullscreen" ,CV_ARCHIVE);

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

	int sdlFlags = SDL_WINDOW_SHOWN;// | SDL_WINDOW_OPENGL; // SDL_WINDOW_ALLOW_HIGHDPI

	if(isWindowed)
	{
		sdlFlags |= SDL_WINDOW_RESIZABLE;
	}
	else
	{
		sdlFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
	}

	handle = SDL_CreateWindow(GAME_WINDOW_TITLE, nAdjustedPosX, nAdjustedPosY, nAdjustedWide, nAdjustedTall, sdlFlags);

	if(handle == NULL)
	{
		ErrorMsg("Can't create window!\n%s\n",SDL_GetError());
		return NULL;
	}

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

bool s_bActive = true;
bool s_bProcessInput = true;

static CGameHost s_Host;
CGameHost* g_pHost = &s_Host;


// TODO: always query joystick count

SDL_Joystick* g_mainJoystick = NULL;

void InitSDLJoysticks()
{
	int numPads = SDL_NumJoysticks();

	if(numPads)
		MsgInfo("Found %d joysticks/gamepads.\n", SDL_NumJoysticks());

	if (numPads > 0)
	{
		g_mainJoystick = SDL_JoystickOpen(0);
	}
}

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
				s_bActive = true;
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				s_bActive = false;
			}
			else if(event->window.event == SDL_JOYDEVICEADDED)
			{
				Msg("TODO: add new joystick\n");
			}
			else if(event->window.event == SDL_JOYDEVICEREMOVED)
			{
				Msg("TODO: remove joystick\n");
			}

			break;
		}
		default:
		{
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
	if( SDL_Init(/*SDL_INIT_VIDEO | */SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0)
	{
		ErrorMsg( "Failed to init SDL system!\n" );
		return false;
	}

	if(!g_pHost->LoadModules())
		return false;

	// execute configuration files and command line after all libraries are loaded.
	g_sysConsole->ClearCommandBuffer();
	g_sysConsole->ParseFileToCommandBuffer("cfg/config_default.cfg");
	g_sysConsole->ExecuteCommandBuffer();

	EQWNDHANDLE mainWindow = Sys_CreateWindow();

	if(!g_pHost->InitSystems( mainWindow, !r_fullscreen.GetBool() ))
		return false;

	g_cmdLine->ExecuteCommandLine(true, true);

	InitSDLJoysticks();

	return true;
}

//
// Runs game engine loop
//
void Host_GameLoop()
{
#ifdef _WIN32
	// make windows do higher tick rate in kernel.
	// I know this is bad
	timeBeginPeriod(1);
#endif

	SDL_Event event;

    // TODO:    use this in right place
    //          e.g. g_pHost->RequestTextInput()
	SDL_StartTextInput();

	do
	{
		while(SDL_PollEvent(&event))
			EQHandleSDLEvents( &event );

		if(!s_bActive) g_pHost->SignalPause();

		if (s_bActive || g_pHost->IsInMultiplayerGame())
		{
			g_pHost->Frame();
		}
		else
			Threading::Yield();

		// or yield
		if(sys_sleep.GetInt() > 0)
            Platform_Sleep( sys_sleep.GetInt() );
	}
	while(g_pHost->GetQuitState() != CGameHost::QUIT_TODESKTOP);

#ifdef _WIN32
	// restore period
	timeEndPeriod(1);
#endif
}

//
// Shutdowns all system in order
//
void Host_Terminate()
{
	g_pHost->ShutdownSystems();
	SDL_Quit();
}
