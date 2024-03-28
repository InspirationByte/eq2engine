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

extern void CookTarget(const char* pszTargetName, CEqJobManager& jobMng);

int main(int argc, char* argv[])
{
	g_eqCore->Init("shadercooker", argc, argv);

	Install_SpewFunction();

	if(!g_fileSystem->Init(false))
		return -1;

	MsgInfo("ShaderCooker - Eq2 MatSystem offline shader compiler\n\n\n");

	if(g_cmdLine->GetArgumentCount() <= 1)
	{
		Usage();
	}

	CEqJobManager jobMng("shadersJobs", max(4, g_cpuCaps->GetCPUCount()), 16384);

	for (int i = 0; i < g_cmdLine->GetArgumentCount(); i++)
	{
		EqString argStr = g_cmdLine->GetArgumentString(i);

		if (!argStr.CompareCaseIns("-target"))
		{
			CookTarget(g_cmdLine->GetArgumentsOf(i), jobMng);
		}
	}

	g_eqCore->Shutdown();

	return 0;
}

