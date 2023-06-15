//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Configuration file loader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "input/InputCommandBinder.h"

void WriteCfgFile(const char *pszFilename, bool bWriteKeyConfiguration /*= true*/)
{
	IFile *cfgfile = g_fileSystem->Open(pszFilename,"w");
	if(!cfgfile)
	{
		MsgError("Failed to write configuraton file '%s'\n", pszFilename);
		return;
	}

	MsgInfo("Writing configuraton file '%s'\n",pszFilename);

	cfgfile->Print("// Auto-generated by WriteCfgFile()\n");

	if(bWriteKeyConfiguration)
		g_inputCommandBinder->WriteBindings(cfgfile);

	const ConCommandListRef cmdList = g_consoleCommands->GetAllCommands();

	for(int i = 0; i < cmdList.numElem();i++)
	{
		if(cmdList[i]->IsConVar())
		{
			ConVar *cv = (ConVar*)cmdList[i];
			if(cv->GetFlags() & CV_ARCHIVE)
				cfgfile->Print("seti %s %s\n",cv->GetName(),cv->GetString());
		}
	}

	g_fileSystem->Close(cfgfile);
}

DECLARE_CMD(writecfg,"Saves the confirugation file", 0)
{
	if(CMD_ARGC > 0)
	{
		WriteCfgFile(CMD_ARGV(0).ToCString(),true);
	}
	else
		WriteCfgFile("user.cfg",true);
}
