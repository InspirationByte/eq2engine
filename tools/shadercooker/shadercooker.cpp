//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader compiler batch utility
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ICommandLine.h"
#include "core/IFileSystem.h"
#include "core/IEqCPUServices.h"
#include "core/platform/eqjobmanager.h"
#include "utils/KeyValues.h"

DECLARE_CVAR(__cheats, "1", "Enable cheats", CV_PROTECTED | CV_INVISIBLE);

void Usage()
{
	MsgWarning("USAGE:\n	shadercooker [-set <PARAMETER> <VALUE>] -target <target name>\n");
}

extern void CookTarget(CEqJobManager& jobMng, const char* pszTargetName, const char* shaderNameFilter);
extern void SetVariable(const char* key, const char* value);

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

	g_cmdLine->ExecuteCommandLine();

	ArrayCRef<EqString> args = g_cmdLine->GetParameters();
	if (args.numElem() <= 1)
		Usage();

	SetVariable("ENGINE_DIR", g_fileSystem->GetCurrentDataDirectory());
	SetVariable("GAME_DIR", g_fileSystem->GetCurrentGameDirectory());

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
			else if (!argStr.CompareCaseIns("-set"))
			{
				const char* keyValue[2];
				const int numValues = g_cmdLine->GetArgumentsOf(i, keyValue, elementsOf(keyValue));
				if (numValues != 2)
				{
					Msg("-set: key and value are required\n");
					continue;
				}
				SetVariable(keyValue[0], keyValue[1]);
			}
		}
	}

	g_eqCore->Shutdown();

	return 0;
}

