//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"

#include "egf/EGFGenerator.h"

class IShaderAPI* g_renderAPI = nullptr;
class IMaterialSystem* g_matSystem = nullptr;

DECLARE_CVAR(__cheats, "1", "Enable cheats", CV_PROTECTED | CV_INVISIBLE);

static bool CompileESCScript(const char* filename)
{
	CEGFGenerator generator;

	// preprocess scripts
	if(!generator.InitFromKeyValues(filename))
		return false;

	// generate EGF file
	if(!generator.GenerateEGF())
		return false;

	// generate POD file
	if(!generator.GeneratePOD())
		return false;

	return true;
}

int main(int argc, char **argv)
{
	Install_SpewFunction();

	CoreAppInitParameters appInitParams;
	appInitParams.appName = "egfCa";
	appInitParams.commandLine = ArrayCRef(argv, argc);
	g_eqCore->Init(appInitParams);

	MsgInfo("EGFCA, a command-line utility to compile  model scripts (esc)\n");
	MsgInfo("Generates EGF of version %d\n", EQUILIBRIUM_MODEL_VERSION);
	MsgWarning("Copyright (c) Inspiration Byte 2009-2026\n");

	if(!g_fileSystem->Init(false))
		return -1;

	g_cmdLine->ExecuteCommandLine();

	ArrayCRef<EqString> args = g_cmdLine->GetParameters();
	if (args.numElem() <= 1)
	{
		MsgError("example: egfca <esc_script.esc> <esc_script2.asc> [<...>]\n");
		getchar();
		return 0;
	}

	for (EqStringRef argStr : args)
	{
		if(argStr[0] == '+')
			continue;

		if(!CompileESCScript(argStr))
		{
			getchar();
			break;
		}
	}

	g_eqCore->Shutdown();
	return 0;
}
