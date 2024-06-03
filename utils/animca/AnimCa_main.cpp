//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ICommandLine.h"
#include "core/ConVar.h"

#include "egf/MotionPackageGenerator.h"

class IShaderAPI* g_renderAPI = nullptr;
class IMaterialSystem* g_matSystem = nullptr;

DECLARE_CVAR(__cheats, "1", "Enable cheats", CV_PROTECTED | CV_INVISIBLE);
DECLARE_CVAR_RENAME(c_filename, "filename", "none", "script file name", 0);

int main(int argc, char **argv)
{
	//Only set debug info when connecting dll
	#ifdef CRT_DEBUG_ENABLED
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

	g_eqCore->Init("animCa",argc,argv);

	MsgWarning("ANIMCA, a command-line utility to compile motion packages for EGF models\n");
	MsgWarning("Copyright © Inspiration Byte 2009-2014\n");
	MsgWarning("Only used on EGF version %d\n", EQUILIBRIUM_MODEL_VERSION);

	// Filesystem is first!
	if(!g_fileSystem->Init(false))
	{
		g_eqCore->Shutdown();
		return 0;
	}

	//Sleep(5000); //wait 5 seconds

	g_cmdLine->ExecuteCommandLine();

	if(!CString::CompareCaseIns("none", c_filename.GetString()))
	{
		MsgError("example: animca +filename <asc_script.asc>\n");
	}
	else
	{
		CMotionPackageGenerator generator;

		if(generator.CompileScript(c_filename.GetString()))
		{
			MsgAccept("Compilation success\n");
		}
		else
		{
			MsgError("Compilation failed\n  Please run with +developer 1\n");
		}
	}

	g_eqCore->Shutdown();
	return 0;
}
