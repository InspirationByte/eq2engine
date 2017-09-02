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

#include "platform/MessageBox.h"

#include "IDkCore.h"
#include "window.h"
#include "EngineSpew.h"

#include "utils/DkLinkedList.h"

#ifdef _WIN32
#include "Resources/resource.h"
#endif // _WIN32

extern ConVar r_fullscreen;

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",0);

#if defined(ANDROID) && defined(EQ_USE_SDL)

void EQSDLMessageBoxCallback(const char* messageStr, EMessageBoxType type )
{
	switch(type)
	{
		case MSGBOX_INFO:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "INFO", messageStr, NULL);
			break;
		case MSGBOX_WARNING:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "WARNING", messageStr, NULL);
			break;
		case MSGBOX_ERROR:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", messageStr, NULL);
			break;
		case MSGBOX_CRASH:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FATAL ERROR", messageStr, NULL);
			break;
	}

}

#endif // ANDROID && EQ_USE_SDL

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

#include <unistd.h>

// posix apps
int main(int argc, char** argv)
{
#ifdef ANDROID
	const char* androidStoragePath = "/storage/external_SD/eqengine"; //SDL_AndroidGetInternalStoragePath();

	// preconfigure game base path
	g_fileSystem->SetBasePath( androidStoragePath );

	SetMessageBoxCallback(EQSDLMessageBoxCallback);

#endif // ANDROID

	// init core
	if(!GetCore()->Init("Game", argc, argv))
		return -1;

#ifdef ANDROID
	MsgInfo("Android storage path: %s\n", androidStoragePath);
#endif // ANDROID

#endif

#ifdef _WIN32
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
	}
#endif // _WIN32

	// init file system
	if(!g_fileSystem->Init(false))
		return -2;

	g_localizer->AddTokensFile("game");

	InstallEngineSpewFunction();

	// initialize timers
	Platform_InitTime();

	if(!Host_Init())
		return -3;

#ifdef _WIN32
	if(Splash_HWND)
		DestroyWindow(Splash_HWND);
#endif // _WIN32

	Host_GameLoop();

	UninstallEngineSpewFunction();

	Host_Terminate();

	// shutdown
	GetCore()->Shutdown();

	return 0;
}
