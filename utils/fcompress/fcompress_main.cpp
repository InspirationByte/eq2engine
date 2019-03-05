//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: 
//////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "platform/Platform.h"
#include "DebugInterface.h"
#include "DPKFileWriter.h"
#include "DPKFileReader.h"
#include "IDkCore.h"
#include "IFileSystem.h"
#include "cmdlib.h"
#include "utils/eqtimer.h"
#include "utils/strtools.h"

void Usage()
{
	Msg("Usage: \n");
	Msg(" fcompress.exe -d <directory> -file -o <filename.eqpak>\n\n");
	Msg("-d / -dir <directory> - Adds folder to package\n");
	Msg("-f / -file <name> - Adds file to package\n");
	Msg("-o / -out <name> - Output filename\n");
	Msg("-m / -mount <directory> - Sets the mount path for package\n");
	Msg("-c / -compression <level> - Sets the compression level of archive\n");
}

int _tmain(int argc, char **argv)
{
	//Only set debug info when connecting dll
#ifdef _DEBUG
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	flag |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif

	Install_SpewFunction();

	GetCore()->Init("fcompress",argc,argv);

	Msg("Equilibrium package compressor utility\n");
	Msg(" Version 1.0\n");

	// initialize file system
	if(!g_fileSystem->Init(false))
		return 0;

	g_cmdLine->ExecuteCommandLine();
	if(g_cmdLine->GetArgumentCount() <= 1)
	{
		Usage();

		GetCore()->Shutdown();
		return 0;
	}

	EqString outFileName = "";

	CDPKFileWriter dpkWriter;

	for(int i = 0; i < g_cmdLine->GetArgumentCount(); i++)
	{
		const char* arg = g_cmdLine->GetArgumentString( i );

		if(!stricmp(arg, "-o") || !stricmp(arg, "-out"))
		{
			outFileName = g_cmdLine->GetArgumentsOf(i);
		}
		else if(!stricmp(arg, "-f") || !stricmp(arg, "-file"))
		{
			dpkWriter.AddFile( g_cmdLine->GetArgumentsOf(i) );
		}
		else if(!stricmp(arg, "-d") || !stricmp(arg, "-dir"))
		{
			dpkWriter.AddDirecory( g_cmdLine->GetArgumentsOf(i), true );
		}
		else if(!stricmp(arg, "-m") || !stricmp(arg, "-mount"))
		{
			dpkWriter.SetMountPath( g_cmdLine->GetArgumentsOf(i) );
		}
		else if(!stricmp(arg, "-c") || !stricmp(arg, "-compression"))
		{
			dpkWriter.SetCompression( atoi(g_cmdLine->GetArgumentsOf(i)) );
		}
		else if(!stricmp(arg, "-ignorecompressionext"))
		{
			DkList<EqString> splitArgs;

			EqString argsStr = g_cmdLine->GetArgumentsOf(i);
			xstrsplit(argsStr.c_str(), " ", splitArgs);

			for(int j = 0; j < splitArgs.numElem(); j++)
			{
				dpkWriter.AddIgnoreCompressionExtension( splitArgs[j].c_str() );
			}
		}
	}

	dpkWriter.BuildAndSave( outFileName.c_str() );

	GetCore()->Shutdown();

	return 0;
}