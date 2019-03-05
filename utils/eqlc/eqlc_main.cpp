//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap compiler main
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "Platform.h"
#include "Utils/strtools.h"
#include "IDkCore.h"
#include "Utils/Align.h"
#include "DebugInterface.h"
#include "cmdlib.h"

#include <iostream>
#include <conio.h>
#include <malloc.h>

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif

#include "eqlc.h"

#include "Physics/DkBulletPhysics.h"

DKMODULE *g_matsystemmod = NULL;

IMaterialSystem*			materials = NULL;
IShaderAPI*					g_pShaderAPI = NULL;
DkList<shaderfactory_t>		pShaderRegistrators;

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",0);

IPhysics * physics = new DkPhysics();

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
			PAINTSTRUCT paint;
			BeginPaint(hwnd, &paint);
			//s_bActive = !IsRectEmpty(&paint.rcPaint);
			EndPaint(hwnd, &paint);
			break;
			/*
		case WM_KILLFOCUS:
			s_bActive = FALSE;
		case WM_SETFOCUS:
			s_bActive = TRUE;
			
		case WM_ACTIVATE:
			s_bActive = !(wParam == WA_INACTIVE);
			DefWindowProc(hwnd, message, wParam, lParam);
			ShowWindow(hwnd, SW_SHOW);
			break;
			*/
		case WM_SIZE:
			{
				int w = LOWORD(lParam);
				int h = HIWORD(lParam);

				if(w > 0 && h > 0)
				{
					//g_pEngineHost->SetWindowSize(w, h);
				}

				break;
			}
			/*
		case WM_WINDOWPOSCHANGED:
			if ((((LPWINDOWPOS) lParam)->flags & SWP_NOSIZE) == 0)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				int w = rect.right - rect.left;
				int h = rect.bottom - rect.top;

				if (w > 0 && h > 0)
				{
					g_pEngineHost->SetWindowSize(w, h);
					ShowWindow(hwnd, SW_SHOW);
				}
			}
			break;
			*/
		case WM_CREATE:
			ShowWindow(hwnd, SW_SHOW);
			break;
		case WM_CLOSE:
			//g_pEngineHost->SetQuitting(IEngine::QUIT_TODESKTOP);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

bool RegisterWindowClass(HINSTANCE hInst)
{
	WNDCLASS wincl;

	UnregisterClass( DARKTECH_WINDOW_CLASSNAME, hInst );

	// Declare window class
	wincl.hInstance = hInst;
	wincl.lpszClassName = DARKTECH_WINDOW_CLASSNAME;
	wincl.lpfnWndProc = WinProc;
	wincl.style = 0;
	wincl.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(hInst, IDI_APPLICATION);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = NULL;
	RegisterClass(&wincl);

	return true;
}

const char* OverrideShader_DepthWrite()
{
	return "EQLCDepthWriteLighting";
}

void InitMatSystem()
{
	// init matsystem
	g_matsystemmod = g_fileSystem->LoadModule("EqMatSystem.dll");

	if(!g_matsystemmod)
	{
		ErrorMsg("Cannot open EqMatSystem module!\n");
		exit(0);
	}

	materials = (IMaterialSystem*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);

	if(!materials)
	{
		ErrorMsg("ERROR! Couldn't get interface of EqMatSystem!");
		exit(0);
	}

	HWND window = CreateWindow(DARKTECH_WINDOW_CLASSNAME, "EQLC Render Window", WS_OVERLAPPEDWINDOW, 0, 0, 800, 800, HWND_DESKTOP, NULL, NULL, NULL);

	matsystem_render_config_t materials_config;

	materials_config.enableBumpmapping = false;
	materials_config.enableSpecular = false;
	materials_config.enableShadows = true;
	materials_config.wireframeMode = false;
	materials_config.threadedloader = false;
	materials_config.lowShaderQuality = false;
	materials_config.editormode = false;

	materials_config.ffp_mode = false;
	materials_config.lighting_model = MATERIAL_LIGHT_FORWARD;
	materials_config.stubMode = false;

	materials_config.shaderapi_params.windowedMode = true;
	materials_config.shaderapi_params.windowHandle = window;
	materials_config.shaderapi_params.screenFormat = FORMAT_RGB8;

	bool materialSystemStatus = materials->Init("materials/", "EqD3D9RHI", materials_config);

	if(!materialSystemStatus)
	{
		ErrorMsg("ERROR! Cannot initialize material system!");
		exit(0);
	}

	if(!materials->LoadShaderLibrary("eqBaseShaders"))
	{
		MsgError("Cannot load eqBaseShaders.dll!\n");
		exit(0);
	}

	// register all shaders of EQLC
	for(int i = 0; i < pShaderRegistrators.numElem(); i++)
		materials->RegisterShader( pShaderRegistrators[i].shader_name, pShaderRegistrators[i].dispatcher );

	materials->RegisterShaderOverrideFunction("DepthWriteLighting", OverrideShader_DepthWrite);

	g_pShaderAPI = materials->GetShaderAPI();
}

EqString	g_worldName;
int			g_lightmapSize = 1024;

int main(int argc, char **argv)
{

	//Only set debug info when connecting dll
	#ifdef _DEBUG
		#define _CRTDBG_MAP_ALLOC
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
		flag |= _CRTDBG_ALLOC_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	#endif

#ifdef _WIN32
	Install_SpewFunction();
#endif

	// Core initialization
	GetCore()->Init("eqlc",argc,argv);

	// Filesystem is first!
	if(!g_fileSystem->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}

	Msg("---------------------------------\n");
	Msg("Equilibrium Lighting Compiler\n");
	//Msg("Hardware Radiosity renderer");
	Msg(" Version 0.4\n");
	Msg("---------------------------------\n");

	RegisterWindowClass(NULL);

	// if we have -world parameter, load world file and 
	// determine the lightmap count and size, after -
	// init the matsystem in d3d9 mode with lightmap-sized window

	InitMatSystem();

	// Command line execution test was passes not successfully
	g_cmdLine->ExecuteCommandLine();

	physics->Init(MAX_COORD_UNITS);
	physics->CreateScene();

	bool bWorldSpecified = false;

	for(int i = 0; i < g_sysConsole->GetArgumentCount(); i++)
	{
		if(!stricmp("-world", g_cmdLine->GetArgumentString(i)))
		{
			g_worldName = g_cmdLine->GetArgumentString(i+1);
			bWorldSpecified = true;
		}
	}

	if(!bWorldSpecified)
		MsgError("No world specified!\n");

	if(bWorldSpecified)
	{
		Msg("Loading world '%s'\n", g_worldName.GetData());

		if(!g_pLevel->LoadLevel( g_worldName.GetData() ))
			MsgError("Cannot load '%s'! Please update SDK or rebuild!\n", g_worldName.GetData());
	}
	else
		MsgError("No world specified!\n");

	if(g_pLevel->IsLoaded())
	{
		GenerateLightmapTextures();
	}
	
	g_pLevel->UnloadLevel();
	viewrenderer->ShutdownResources();

	physics->DestroyScene();

	materials->Shutdown();

	UnregisterClass(DARKTECH_WINDOW_CLASSNAME, NULL);

	GetCore()->Shutdown();
	return 0;
}
