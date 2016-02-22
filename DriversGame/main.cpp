//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entry points for various platforms
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#include <io.h>
#include <fcntl.h>

#include <crtdbg.h>

#endif // _WIN32

#include "Platform.h"
#include "IDkCore.h"
#include "system.h"
#include "EngineSpew.h"

#include "utils/DkLinkedList.h"

extern ConVar r_fullscreen;

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",0);

#if defined(ANDROID) && !defined(EQ_USE_SDL) // sdl makes our main() function ok

#include <jni.h>
#include <android_native_app_glue.h>

// android app
void android_main( struct android_app* state )
{
    app_dummy();

	// init core
	if(!GetCore()->Init("Game", 0, NULL))
		return;

	// init file system
	if(!g_fileSystem->Init(false))
		return;

	g_localizer->AddTokensFile("game");

	//SetThreadAffinityMask(GetCurrentThread(), 1);

	InstallEngineSpewFunction();

	// initialize timers
	Platform_InitTime();

	InitWindowAndRun();

	// shutdown
	GetCore()->Shutdown();
}

#else

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	//flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	flag |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
#endif
	/*
	//----------------------------------------------------------------
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
	//----------------------------------------------------------------
	*/
	// init core
	if(!GetCore()->Init("Game", lpszCmdLine))
		return -1;

#else

// posix apps
int main(int argc, char** argv)
{

	// init core
	if(!GetCore()->Init("Game", argc, argv))
		return -1;

#endif

	// init file system
	if(!g_fileSystem->Init(false))
		return -2;

	g_localizer->AddTokensFile("game");

	//SetThreadAffinityMask(GetCurrentThread(), 1);

	InstallEngineSpewFunction();

	// initialize timers
	Platform_InitTime();

	InitWindowAndRun();

	// shutdown
	GetCore()->Shutdown();

	return 0;
}

#endif // ANDROID
