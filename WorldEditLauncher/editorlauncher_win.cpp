//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Windows platform launcher
//////////////////////////////////////////////////////////////////////////////////

#include <direct.h>
#include <crtdbg.h>

#include "platform/Platform.h"
#include "platform/MessageBox.h"

#include "IDkCore.h"
#include "IFileSystem.h"
#include "Resource.h"
#include <iostream>

#include <io.h>
#include <fcntl.h>
//#include "fs_module.h"

typedef int (*CreateEngineFunc)(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow, HWND splashHWND);
typedef IDkCore* (*GetCoreFunc)();

static HMODULE s_hCoreModule;
static GetCoreFunc s_hGetCoreFunc;
static IFileSystem* g_pFileSystem;

static DKMODULE* s_pEngineModule;
static CreateEngineFunc s_pEngineInitFunc;

//-----------------------------------------------------------------------------
// Returns the directory where this .exe is running from
//-----------------------------------------------------------------------------
static char *GetBaseDir( const char *pszBuffer )
{
	static char	basedir[ MAX_PATH ];
	char szBuffer[ MAX_PATH ];
	int j;
	char *pBuffer = NULL;

	strcpy( szBuffer, pszBuffer );

	pBuffer = strrchr( szBuffer,'\\' );
	if ( pBuffer )
	{
		*(pBuffer+1) = '\0';
	}

	strcpy( basedir, szBuffer );

	j = strlen( basedir );
	if (j > 0)
	{
		if ( ( basedir[ j-1 ] == '\\' ) || 
			 ( basedir[ j-1 ] == '/' ) )
		{
			basedir[ j-1 ] = 0;
		}
	}

	return basedir;
}

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
	#ifdef _DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	//	flag |= _CRTDBG_DELAY_FREE_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

	/*
	// Used to show console
	AllocConsole();

	HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
	FILE* hf_out = _fdopen(hCrt, "w");
	setvbuf(hf_out, NULL, _IONBF, 1);
	*stdout = *hf_out;

	HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
	hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
	FILE* hf_in = _fdopen(hCrt, "r");
	setvbuf(hf_in, NULL, _IONBF, 128);
	*stdin = *hf_in;
	*/
	

	//Show splash!
	HWND Splash_HWND;

	RECT rect;

	Splash_HWND = CreateDialog(hThisInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

	if (Splash_HWND)
	{
		if (GetWindowRect(Splash_HWND, &rect))
		{
			if (rect.left > (rect.top * 2))
			{
				SetWindowPos (Splash_HWND, 0,
					(rect.left / 2) - ((rect.right - rect.left) / 2),
					rect.top, 0, 0,
					SWP_NOZORDER | SWP_NOSIZE);
			}
		}

		ShowWindow(Splash_HWND, SW_NORMAL);
		UpdateWindow(Splash_HWND);
		SetForegroundWindow (Splash_HWND);

		//Platform_Sleep(1000);

#ifdef LONG_LAUNCH
	// 6 seconds for debugging purposes
	Platform_Sleep(6000);
#endif
	}

	char* pPath = getenv("PATH");

	char szBuffer[ 4096 ];
	memset(szBuffer,0,sizeof(szBuffer));
	char moduleName[ MAX_PATH ];
	if ( !GetModuleFileName( hThisInst, moduleName, MAX_PATH ) )
	{
		MessageBox( 0, "Failed calling GetModuleFileName", "Launcher Error", MB_OK );
		return 0;
	}

	// Get the root directory the .exe is in
	char* pRootDir = GetBaseDir( moduleName );
	sprintf( szBuffer, "PATH=%s\\bin32\\;%s", pRootDir, pPath );
	_putenv( szBuffer );


	//Load Core Dll. if it can't load then check core memory!
	s_hCoreModule = LoadLibrary("eqCore.dll");
	if(!s_hCoreModule)
	{
		ErrorMsg("FATAL ERROR! Failed to load DkCore library!");
		return -1;
	}

	s_hGetCoreFunc = reinterpret_cast<GetCoreFunc>(GetProcAddress(reinterpret_cast<HMODULE>(s_hCoreModule), "GetCore"));
	if(!s_hGetCoreFunc)
	{
		FreeLibrary((HMODULE)s_hCoreModule);
		ErrorMsg("Failed to create Core functions!");
		return -1;
	}

	//Init cpu, time and others
	(s_hGetCoreFunc)()->Init("eqedit", lpszCmdLine);

	g_pFileSystem = (IFileSystem*)(s_hGetCoreFunc)()->GetInterface(FILESYSTEM_INTERFACE_VERSION);

	if(!g_pFileSystem)
	{
		ErrorMsg("FATAL ERROR! Couldn't get core filesystem interface!\n");
		return -1;
	}

	s_pEngineModule = g_pFileSystem->LoadModule("WorldEdit.dll");
	if(!s_pEngineModule)
	{
		ErrorMsg("FATAL ERROR! Failed to load EqEditor library!");

		(s_hGetCoreFunc)()->Shutdown();
		FreeLibrary((HMODULE)s_hCoreModule);
		return -1;
	}

	s_pEngineInitFunc = (CreateEngineFunc)g_pFileSystem->GetProcedureAddress(s_pEngineModule, "CreateEditorFn");

	if(!s_pEngineInitFunc)
	{
		ErrorMsg("FATAL ERROR! Couldn't get WorldEdit factory!");

		g_pFileSystem->FreeModule(s_pEngineModule);
		(s_hGetCoreFunc)()->Shutdown();
		FreeLibrary((HMODULE)s_hCoreModule);
		return -1;
	}

	//if(Splash_HWND)
	//	DestroyWindow(Splash_HWND);

	int result = -1;
	result = s_pEngineInitFunc(hThisInst, hLastInst, lpszCmdLine, nCmdShow, Splash_HWND);

	g_pFileSystem->FreeModule(s_pEngineModule);

	(s_hGetCoreFunc)()->Shutdown();

	FreeLibrary(s_hCoreModule);

	return result;
}