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
#include "egf/model.h"

class IShaderAPI* g_renderAPI = nullptr;
class IMaterialSystem* g_matSystem = nullptr;

DECLARE_CVAR(__cheats, "1", "Enable cheats", CV_PROTECTED | CV_INVISIBLE);

int main(int argc, char **argv)
{
	Install_SpewFunction();

	CoreAppInitParameters appInitParams;
	appInitParams.appName = "animCa";
	appInitParams.commandLine = ArrayCRef(argv, argc);
	g_eqCore->Init(appInitParams);

	Install_SpewFunction();

	MsgInfo("ANIMCA, a command-line utility to compile motion packages for EGF models\n");
	MsgWarning("Copyright (c) Inspiration Byte 2009-2026\n");

	// Filesystem is first!
	if(!g_fileSystem->Init(false))
		return -1;

	g_cmdLine->ExecuteCommandLine();

	ArrayCRef<EqString> args = g_cmdLine->GetParameters();
	if (args.numElem() <= 1)
	{
		MsgError("example: animca <asc_script.asc> <asc_script2.asc> [<...>]\n");
		getchar();
		return 0;
	}

	for (EqStringRef argStr : args)
	{
		if(argStr[0] == '+')
			continue;

		CMotionPackageGenerator generator;
		if(!generator.CompileScript(argStr))
		{
			getchar();
			break;
		}
	}
	
	g_eqCore->Shutdown();
	return 0;
}
