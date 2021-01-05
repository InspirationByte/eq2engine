//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>

#include "platform/Platform.h"
#include "utils/strtools.h"
#include "utils/align.h"
#include "IDkCore.h"

#include "DebugInterface.h"
#include "cmdlib.h"

#include <iostream>
#include <malloc.h>
#include "egf/model.h"
#include "IConCommandFactory.h"
#include "IFileSystem.h"

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif

ConVar cv_cheats("__cheats","1","Enable cheats",CV_INITONLY | CV_INVISIBLE);

extern bool CompileScript(const char* name);

ConVar c_filename("filename","none","script file name", 0);

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

	GetCore()->Init("animCa",argc,argv);

	MsgWarning("ANIMCA, a command-line utility to compile motion packages for EGF models\n");
	MsgWarning("Copyright © Inspiration Byte 2009-2014\n");
	MsgWarning("Only used on EGF version %d\n", EQUILIBRIUM_MODEL_VERSION);

	// Filesystem is first!
	if(!g_fileSystem->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}

	//Sleep(5000); //wait 5 seconds

	g_cmdLine->ExecuteCommandLine();

	if(!stricmp("none", c_filename.GetString()))
	{
		MsgError("example: animca +filename <asc_script.asc>\n");
	}
	else
	{
		if(CompileScript(c_filename.GetString()))
		{
			MsgAccept("Compilation success\n");
		}
		else
		{
			MsgError("Compilation failed\n  Please run with +developer 1\n");
		}
	}

	GetCore()->Shutdown();
	return 0;
}
