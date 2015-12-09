//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - core and control
//////////////////////////////////////////////////////////////////////////////////

#include "core_base_header.h"
#include "DebugInterface.h"
#include "IVirtualStream.h"

#include "cmdlib.h"

void Usage()
{
	MsgWarning("USAGE:\n	atlasgen +makeatlas <input atl file> <output image file>\n");
}

extern void ProcessNewAtlas(const char* pszAtlasSrcName, const char* pszOutputName);

DECLARE_CMD(makeatlas, "Generates atlas from a key-values file list", 0)
{
	if(args && args->numElem())
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

	if(!GetFileSystem()->Init(false))
		return -1;

	Msg("atlasgen - the texture atlas generator for particles and decals\n");

	if(GetCmdLine()->GetArgumentCount() <= 1)
	{
		Usage();
	}

	GetCmdLine()->ExecuteCommandLine( true, true );

	GetCore()->Shutdown();

	return 0;
}

