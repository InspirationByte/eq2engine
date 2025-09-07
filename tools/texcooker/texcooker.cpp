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
	CoreAppInitParameters appInitParams;
	appInitParams.appName = "texCooker";
	appInitParams.commandLine = ArrayCRef(argv, argc);
	g_eqCore->Init(appInitParams);

	Install_SpewFunction();

	if(!g_fileSystem->Init(false))
		return -1;

	MsgInfo("TexCooker - Platform-Specific material/texture converter utility\n\n\n");

	ArrayCRef<EqString> args = g_cmdLine->GetParameters();
	if(args.numElem() <= 1)
		Usage();

	CEqJobManager jobMng("shadersJobs", max(4, g_cpuCaps->GetCPUCount()), 16384);
	for (int i = 0; i < args.numElem(); i++)
	{
		if (!args[i].CompareCaseIns("-target"))
			CookTarget(g_cmdLine->GetArgumentsOf(i), jobMng);
	}

	g_eqCore->Shutdown();

	return 0;
}

