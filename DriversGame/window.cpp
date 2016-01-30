//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers window handler
//////////////////////////////////////////////////////////////////////////////////

#include "Platform.h"
#include "DebugInterface.h"
#include "system.h"
#include "sys_console.h"
#include "imaging/ImageLoader.h"

#include "EngineVersion.h"

#ifdef _WIN32
#include "Mmsystem.h"
#endif

#define GAME_WINDOW_TITLE varargs("Driver Syndicate Alpha [%s] build %d", __DATE__, GetEngineBuildNumber())

// Renderer
DECLARE_CVAR		(r_mode,1024x768,"Screen Resoulution. Resolution string format: WIDTHxHEIGHT" ,CV_ARCHIVE);
DECLARE_CVAR		(r_bpp,32,"Screen bits per pixel",CV_ARCHIVE);

ConVar r_fullscreen("r_fullscreen", "0", "Fullscreen" ,CV_ARCHIVE);

DECLARE_CVAR_NONSTATIC	(r_antialiasing,0,"Antialiasing",CV_ARCHIVE);
extern ConVar		r_renderer;

DECLARE_CVAR		(screenshotJpegQuality,100,"JPEG Quality",CV_ARCHIVE);

void CC_Screenshot_f(DkList<EqString> *args)
{
	if(materials != NULL)
	{
		FILE *file = NULL;

		int i = 0;
		do
		{
			GetFileSystem()->MakeDir("screenshots", SP_ROOT);
			EqString path = _Es(varargs("screenshots/screenshot_%04d.jpg", i));

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

				return;
			}
			i++;
		}
		while (i < 9999);

		return;
	}
}
ConCommand cc_screenshot("screenshot",CC_Screenshot_f,"Save screenshot");

DECLARE_CVAR		(sys_sleep,0,"Sleep time for every frame",CV_ARCHIVE);

EQWNDHANDLE CreateEngineWindow()
{
	Msg(" \n--------- CreateEngineWindow --------- \n");

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

	int sdlFlags = SDL_WINDOW_SHOWN/* | SDL_WINDOW_OPENGL*/; // SDL_WINDOW_ALLOW_HIGHDPI

	if(isWindowed)
	{
		sdlFlags |= SDL_WINDOW_RESIZABLE;
	}
	else
	{
		sdlFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
	}

	if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0)
	{
		MsgError( "Failed to init SDL system!\n" );
		return NULL;
	}

	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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

bool s_bActive = false;
bool s_bProcessInput = true;

static CGameHost s_Host;
CGameHost* g_pHost = &s_Host;

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
			g_pHost->m_nQuitState = CGameHost::QUIT_TODESKTOP;
			break;
		}
		case SDL_WINDOWEVENT:
		{
			if(event->window.event == SDL_WINDOWEVENT_CLOSE)
			{
				g_pHost->m_nQuitState = CGameHost::QUIT_TODESKTOP;
			}
			else if(event->window.event == SDL_WINDOWEVENT_RESIZED)
			{
				if(event->window.data1 > 0 && event->window.data2 > 0)
				{
					g_pHost->SetWindowSize(event->window.data1, event->window.data2);
				}
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				s_bActive = true;
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				s_bActive = false;
			}

			break;
		}
		default:
		{
			if(s_bActive && s_bProcessInput)
			{
				InputCommands_SDL(event);
			}
		}
	}
}

void InitWindowAndRun()
{
	if(!g_pHost->LoadModules())
		exit(0);

	// execute configuration files and command line after all libraries are loaded.
	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->ParseFileToCommandBuffer("cfg/config_default.cfg");
	GetCommandAccessor()->ExecuteCommandBuffer();

	EQWNDHANDLE mainWindow = CreateEngineWindow();

	SDL_GetWindowSize(mainWindow, &g_pHost->m_nWidth, &g_pHost->m_nHeight);

	g_pHost->m_pWindow = mainWindow;

	if(!g_pHost->InitSystems( mainWindow, !r_fullscreen.GetBool() ))
		exit(0);

	InitSDLJoysticks();

	GetCmdLine()->ExecuteCommandLine(true, true);

	SDL_SetWindowTitle(mainWindow, GAME_WINDOW_TITLE);

	SDL_Event event;

#ifdef _WIN32
	timeBeginPeriod(1);
#endif

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
	while(g_pHost->m_nQuitState != CGameHost::QUIT_TODESKTOP);

#ifdef _WIN32
	timeEndPeriod(1);
#endif

	g_pHost->ShutdownSystems();

	SDL_DestroyWindow(g_pHost->m_pWindow);

	SDL_Quit();
}
