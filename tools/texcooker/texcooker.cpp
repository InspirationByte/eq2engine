//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - core and control
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/IFileSystem.h"
#include "core/IEqCPUServices.h"
#include "core/platform/eqjobmanager.h"
#include "texcooker_defs.h"

void Usage()
{
	MsgWarning("USAGE:\n	texcooker -target <target name>\n");
}

int main(int argc, char* argv[])
{
	Install_SpewFunction();

	CoreAppInitParameters appInitParams;
	appInitParams.appName = "texCooker";
	appInitParams.commandLine = ArrayCRef(argv, argc);
	g_eqCore->Init(appInitParams);

	MsgInfo("TexCooker - Platform-Specific material/texture converter utility\n\n\n");
	MsgWarning("Copyright (c) Inspiration Byte 2009-2026\n");

	if(!g_fileSystem->Init(false))
		return -1;

	ArrayCRef<EqString> args = g_cmdLine->GetParameters();
	if(args.numElem() <= 1)
	{
		Usage();
		getchar();
		return 0;
	}

	CEqJobManager jobMng("shadersJobs", max(4, g_cpuCaps->GetCPUCount()), 16384);
	for (int i = 0; i < args.numElem(); i++)
	{
		EqStringRef argStr = args[i];
		if(argStr[0] == '+')
			continue;

		if (!argStr.CompareCaseIns("-target"))
			CookTarget(g_cmdLine->GetArgumentsOf(i), jobMng);
	}

	g_eqCore->Shutdown();

	return 0;
}

