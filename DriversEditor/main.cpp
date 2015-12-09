//**************** Copyright (C) Parallel Prevision, L.L.C 2011 *****************
//
// Description: Equilibrium Engine Editor Initialization
//
//***************************************************************************

#include <direct.h>
#include <crtdbg.h>
#include <iostream>

#include <io.h>
#include <fcntl.h>

#include "DebugInterface.h"

#include "Sys_Radiant.h"
#include "EditorHeader.h"

extern void SaveModel();

DKMODULE* g_matsysmodule = NULL;

int main(int argc, char **argv)
{
	#ifdef _DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	//	flag |= _CRTDBG_DELAY_FREE_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

	// initialize core
	GetCore()->Init("egf_view", argc, argv);

	
	GetCmdLine()->ExecuteCommandLine(true, true);

	ASSERT(GetFileSystem()->Init(false));

	// first, load matsystem module
	g_matsysmodule = GetFileSystem()->LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		exit(0);
	}

	// initialize MX toolkit
	mx::init(argc, argv);

	// intialize radiant
	Sys_InitRadiant();

	// run MX toolkit
	int ret = mx::run ();

	SaveModel();

	materials->Shutdown();

	GetFileSystem()->FreeModule( g_matsysmodule );

	// shutdown core
	GetCore()->Shutdown();

	return ret;
}