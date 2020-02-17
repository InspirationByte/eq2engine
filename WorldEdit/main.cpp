//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

#include "ConVar.h"

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