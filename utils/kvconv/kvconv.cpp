//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Key-Values file converter
//////////////////////////////////////////////////////////////////////////////////

#include "core_base_header.h"
#include "DebugInterface.h"
#include "IVirtualStream.h"

//#include "KeyValuesA.h"
#include "KeyValues.h"

void Usage()
{
	MsgWarning("USAGE:\n	kvconv +convert <input file> [optional output file]");
}

void ConvertOldToNewKeyValues(const char* pszInputFileName, const char* pszOutputFileName);

DECLARE_CMD(convert, "Converts a file", 0)
{
	if(args && args->numElem())
	{
		EqString outname = CMD_ARGV(0);

		if(CMD_ARGC > 1)
			outname = CMD_ARGV(1);

		//outname = "kvout.txt";

		KeyValues kv;
		if( kv.LoadFromFile( CMD_ARGV(0).GetData() ) )
		{
			Msg( "'%s' already converted\n", CMD_ARGV(0).GetData() );
		}
		else
		{
			Msg("converting '%s' to new format, output: '%s'\n", CMD_ARGV(0).GetData(), outname.GetData());
			ConvertOldToNewKeyValues(CMD_ARGV(0).GetData(), outname.GetData());
		}
	}
	else
		Usage();
}

int main(int argc, char* argv[])
{
	GetCore()->Init("kvconv", argc, argv);

	if(!GetFileSystem()->Init(false))
		return -1;

	GetCmdLine()->ExecuteCommandLine( true, true );

	GetCore()->Shutdown();

	return 0;
}

