//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>

#include "Platform.h"
#include "Utils/strtools.h"
#include "IDkCore.h"
#include "Utils/Align.h"
#include "DebugInterface.h"
#include "cmdlib.h"

#include <iostream>
#include <malloc.h>
#include "model.h"

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif


ConVar cv_cheats("__cheats","1","Enable cheats",CV_INITONLY | CV_INVISIBLE);

extern bool CompileESCScript(const char* name);

ConVar c_filename("filename","none","script file name", 0);
//ConVar c_convert("convert","0","converts to the latest model version", 0);

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

	GetCore()->Init("egfCa", argc, argv);

	MsgWarning("EGFCA, a command-line utility to compile  model scripts (esc)\n");
	MsgWarning("Copyright © Inspiration Byte 2009-2014\n");
	MsgWarning("Generates EGF of version %d\n", EQUILIBRIUM_MODEL_VERSION);

	// Filesystem is first!
	if(!GetFileSystem()->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}

	GetCmdLine()->ExecuteCommandLine(true,true);

	if(!stricmp("none", c_filename.GetString()))
	{
		MsgError("example: egfca +filename <esc_script.esc>\n");
	}
	else
	{
		Msg("string = %s\n", c_filename.GetString());

		if( CompileESCScript( c_filename.GetString() ) )
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
