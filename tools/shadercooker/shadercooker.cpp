//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader compiler batch utility
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/IFileSystem.h"
#include "core/IEqCPUServices.h"
#include "core/platform/eqjobmanager.h"

void Usage()
{
	MsgWarning("USAGE:\n	shadercooker -target <target name>\n");
}

extern void CookTarget(CEqJobManager& jobMng, const char* pszTargetName, const char* shaderNameFilter);

int main(int argc, char* argv[])
{
	CoreAppInitParameters appInitParams;
	appInitParams.appName = "shaderCooker";
	appInitParams.commandLine = ArrayCRef(argv, argc);
	g_eqCore->Init(appInitParams);

	Install_SpewFunction();

	if(!g_fileSystem->Init(false))
		return -1;

	MsgInfo("ShaderCooker - Eq2 offline shader compiler\n\n\n");

	ArrayCRef<EqString> args = g_cmdLine->GetParameters();
	if (args.numElem() <= 1)
		Usage();

	{
		EqString shaderFilter;

		CEqJobManager jobMng("shadersJobs", max(4, g_cpuCaps->GetCPUCount() * 2), 16384);
		for (int i = 0; i < args.numElem(); i++)
		{
			EqStringRef argStr = args[i];
			if (!argStr.CompareCaseIns("-target"))
				CookTarget(jobMng, g_cmdLine->GetArgumentsOf(i), shaderFilter);
			else if (!argStr.CompareCaseIns("-filter"))
				shaderFilter = g_cmdLine->GetArgumentsOf(i);
		}
	}

	g_eqCore->Shutdown();

	return 0;
}

