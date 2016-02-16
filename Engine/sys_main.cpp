//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech main entry point
//////////////////////////////////////////////////////////////////////////////////

#include "Platform.h"
#include "EngineVersion.h" // Engine version
#include "EngineSpew.h"
#include "sys_enginehost.h"

#include "sys_enginehost.h"

#include <crtdbg.h>

//---------------------------------------------------------------------------
// Purpose: DllMain function
//---------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	//Only set debug info when connecting dll
	#ifdef _DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
		flag |= _CRTDBG_ALLOC_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	#endif

	return TRUE;
}

extern void EngineMessage();

//---------------------------------------------------------------------------
// Purpose: Entry point for engine dll to run under normal mode
//---------------------------------------------------------------------------
ONLY_EXPORTS int CreateEngineFn(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow, HWND splashHWND)
{
	//InitCRTMemDebug();

	char filename[MAX_PATH];
	GetModuleFileName(hThisInst, filename, MAX_PATH);

	char modNamefromExe[MAX_PATH];
	ExtractFileBase(filename, modNamefromExe);

	bool shouldUseExeName = true;
	
	if(!stricmp("Eq32", modNamefromExe) || !stricmp("Eq64", modNamefromExe) || !stricmp("Equilibrium", modNamefromExe))
		shouldUseExeName = false;

	if(!g_fileSystem->Init( shouldUseExeName ))
		return -1;

	// if executable compiled especially for game, we start named mod
	if(shouldUseExeName)
		g_fileSystem->AddSearchPath( modNamefromExe );

	Msg("Initializing Engine...\n \n");

	InstallEngineSpewFunction();

	// tell engine information
	EngineMessage();

	// Initialize time and query perfomance
	Platform_InitTime();

	// Initial status
	int iStatus = -1;

#if !defined(PLAT_SDL) && defined(PLAT_WIN)
	RegisterWindowClass(hThisInst);
#endif // PLAT_SDL

	//GenerateBitmapFont("Tahoma", 18, 0, false,false,false);

	// Initialize engine
	if( InitEngineHost() )
	{
		DestroyWindow(splashHWND);
		iStatus = RunEngine(hThisInst);
	}

	return iStatus;
}