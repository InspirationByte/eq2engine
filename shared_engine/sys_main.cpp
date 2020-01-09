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
#include "sys_window.h"

#include "ConVar.h"
#include "ILocalize.h"
#include "IFileSystem.h"

#include "sys_in_console.h"

#include "utils/DkLinkedList.h"

#ifdef _WIN32
//#include "Resources/resource.h"
#endif // _WIN32

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

	CEqConsoleInput::SpewInit();

	// init core
	if(!GetCore()->Init("Game", lpszCmdLine))
		return -1;
#else

#include <unistd.h>

// posix apps
int main(int argc, char** argv)
{
#ifdef ANDROID
	const char* androidStoragePath = "/storage/6135-3937/eqengine"; //SDL_AndroidGetInternalStoragePath();

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

	// init file system
	if(!g_fileSystem->Init(false))
		return -2;

	kvkeybase_t* fileSystemSettings = GetCore()->GetConfig()->FindKeyBase("FileSystem");

	// Search for mods folder
	if (fileSystemSettings && KV_GetValueBool(fileSystemSettings->FindKeyBase("EnableMods"), 0, false))
	{
		DKFINDDATA* findData = nullptr;
		char* modPath = (char*)g_fileSystem->FindFirst("Mods/*.*", &findData, SP_ROOT);

		if (modPath)
		{
			int count = 0;

			while (modPath = (char*)g_fileSystem->FindNext(findData))
			{
				if (g_fileSystem->FindIsDirectory(findData) && stricmp(modPath, "..") && stricmp(modPath, "."))
				{
					MsgInfo("*** Registered Mod '%s' ***\n", modPath);
					g_fileSystem->AddSearchPath(varargs("$MOD$_%d", count), varargs("Mods/%s", modPath));
					count++;
				}
			}

			g_fileSystem->FindClose(findData);
		}
	}

	g_localizer->AddTokensFile("game");

	// initialize timers
	Platform_InitTime();

	if(!Host_Init())
		return -3;

	Host_GameLoop();

	CEqConsoleInput::SpewClear();

	Host_Terminate();

	// shutdown
	GetCore()->Shutdown();

	return 0;
}
