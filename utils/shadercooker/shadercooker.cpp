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
#include "core/IEqParallelJobs.h"

static eqJobThreadDesc_t s_jobTypes[] = {
	{JOB_TYPE_ANY, 16},
};

void Usage()
{
	MsgWarning("USAGE:\n	shadercooker -target <target name>\n");
}

extern void CookTarget(const char* pszTargetName);

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

	g_parallelJobs->Init(s_jobTypes);

	for (int i = 0; i < g_cmdLine->GetArgumentCount(); i++)
	{
		EqString argStr = g_cmdLine->GetArgumentString(i);

		if (!argStr.CompareCaseIns("-target"))
		{
			CookTarget(g_cmdLine->GetArgumentsOf(i));
		}
	}

	g_parallelJobs->Shutdown();
	g_eqCore->Shutdown();

	return 0;
}

