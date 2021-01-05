//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - core and control
//////////////////////////////////////////////////////////////////////////////////

#include "core_base_header.h"
#include "DebugInterface.h"
#include "utils/IVirtualStream.h"

#include "cmdlib.h"

void Usage()
{
	MsgWarning("USAGE:\n	atlasgen +makeatlas <input atl file> <output image file>\n");
}

extern void ProcessNewAtlas(const char* pszAtlasSrcName, const char* pszOutputName);

DECLARE_CMD(makeatlas, "Generates atlas from a key-values file list", 0)
{
	if(CMD_ARGC > 0)
	{
		EqString outname = CMD_ARGV(1);

		ProcessNewAtlas(CMD_ARGV(0).GetData(), outname.GetData());
	}
	else
		Usage();
}

int main(int argc, char* argv[])
{
	GetCore()->Init("atlasgen", argc, argv);

	Install_SpewFunction();

	if(!g_fileSystem->Init(false))
		return -1;

	Msg("atlasgen - the texture atlas generator for particles and decals\n");

	if(g_cmdLine->GetArgumentCount() <= 1)
	{
		Usage();
	}

	g_cmdLine->ExecuteCommandLine();

	GetCore()->Shutdown();

	return 0;
}

