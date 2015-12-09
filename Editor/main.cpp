//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Editor Initialization
//////////////////////////////////////////////////////////////////////////////////

#include <direct.h>
#include <crtdbg.h>
#include <iostream>

#include <io.h>
#include <fcntl.h>

#include "DebugInterface.h"

#include "Sys_Radiant.h"
#include "EditorHeader.h"

#include "wx_inc.h"

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",0);

//-----------------------------------------------------------------------------
// Returns the directory where this .exe is running from
//-----------------------------------------------------------------------------
static char *GetBaseDir( const char *pszBuffer )
{
	static char	basedir[ MAX_PATH ];
	char szBuffer[ MAX_PATH ];
	int j;
	char *pBuffer = NULL;

	strcpy_s( szBuffer,sizeof(szBuffer), pszBuffer );

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

extern HWND Splash_HWND;

bool InitCore(HINSTANCE hInstance, char *pCmdLine);

ONLY_EXPORTS int CreateEditorFn(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow, HWND splashHWND)
{
	Splash_HWND = splashHWND;

	if(!InitCore(hThisInst, lpszCmdLine))
		return -1;

	return wxEntry(hThisInst, hLastInst, NULL, 0);
}

/*

There is no main function, all does the WX Widgets

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
	#ifdef _DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	//	flag |= _CRTDBG_DELAY_FREE_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

	g_instance = hThisInst;

	char* pPath = getenv("PATH");

	char szBuffer[ 4096 ];
	memset(szBuffer,0,sizeof(szBuffer));
	char moduleName[ MAX_PATH ];
	if ( !GetModuleFileName( NULL, moduleName, MAX_PATH ) )
	{
		MessageBox( 0, "Failed calling GetModuleFileName", "Launcher Error", MB_OK );
		return 0;
	}

	// Get the root directory the .exe is in
	char* pRootDir = GetBaseDir( moduleName );
	sprintf( szBuffer, "PATH=%s\\bin32\\;%s", pRootDir, pPath );
	_putenv( szBuffer );

	// initialize core
	GetCore()->Init("leveleditor", lpszCmdLine);

	if(!GetFileSystem()->Init(false))
		return -1;

	GetFileSystem()->AddSearchPath("EqEditor");

	// first, load matsystem module
	g_matsysmodule = FS_LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		return -1;
	}

	// intialize radiant
	Editor_Init();

	InstallSpewFunction();

	int ret = Editor_Run();

	SetSpewFunction(nullspew);

	WriteCfgFile("cfg/editor.cfg");
	
	// shutdown material system
	materials->Shutdown();
	FS_UnloadModule(g_matsysmodule);

	// shutdown core
	GetCore()->Shutdown();

	return ret;
}
*/