//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: Equilibrium Engine Editor Initialization
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#include <direct.h>
#include <crtdbg.h>
#include <iostream>

#include <io.h>
#include <fcntl.h>

#include "DebugInterface.h"

#include "Sys_Radiant.h"
#include "EditorHeader.h"


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

extern void SaveModel();

DKMODULE* g_matsysmodule = NULL;

int main (int argc, char *argv[])
{
	#ifdef _DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	//	flag |= _CRTDBG_DELAY_FREE_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif
/*
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
	*/
	// initialize core
	GetCore()->Init("egf_view", argc, argv);

	GetCmdLine()->ExecuteCommandLine(true, true);

	// first, load matsystem module
	g_matsysmodule = GetFileSystem()->LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		exit(0);
	}


	ASSERT(GetFileSystem()->Init(false));

	// initialize MX toolkit
	mx::init(argc, argv);

	// intialize radiant
	Sys_InitRadiant();

	// run MX toolkit
	int ret = mx::run ();

	GetFileSystem()->RemoveFile("egfview_temp.egf", SP_ROOT);

	SaveModel();

	materials->Shutdown();

	GetFileSystem()->FreeModule(g_matsysmodule);

	// shutdown core
	GetCore()->Shutdown();

	return ret;
}