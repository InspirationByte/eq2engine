//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: 
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ICommandLine.h"

#include "utils/KeyValues.h"

#include "DPKFileWriter.h"

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

static void Usage()
{
	MsgWarning("USAGE:\n	fcompress -target <target name>\n");
}

static void CookPackageTarget(const char* targetName)
{
	// load all properties
	KeyValues kvs;
	if (!kvs.LoadFromFile("PackageCooker.CONFIG", SP_ROOT))
	{
		MsgError("Failed to load 'PackageCooker.CONFIG' file!\n");
		return;
	}

	EqString outputFileName(targetName);
	if (outputFileName.Path_Extract_Ext().Length() == 0)
		outputFileName.Append(".epk");

	CDPKFileWriter dpkWriter;
	dpkWriter.AddKeyValueFileExtension("mat");
	dpkWriter.AddKeyValueFileExtension("res");
	dpkWriter.AddKeyValueFileExtension("txt");
	dpkWriter.AddKeyValueFileExtension("def");

	{
		// load target info
		KVSection* packages = kvs.FindSection("Packages");
		if (!packages)
		{
			MsgError("Missing 'Packages' section in 'PackageCooker.CONFIG'\n");
			return;
		}

		KVSection* currentTarget = packages->FindSection(targetName);
		if (!currentTarget)
		{
			MsgError("Cannot find package section '%s'\n", targetName);
			return;
		}

		const int targetCompression = KV_GetValueInt(currentTarget->FindSection("compression"), 0, 0);
		const char* targetFilename = KV_GetValueString(currentTarget->FindSection("output"), 0, nullptr);
		const char* mountPath = KV_GetValueString(currentTarget->FindSection("mountPath"), 0, nullptr);
		const char* encryption = KV_GetValueString(currentTarget->FindSection("encryption"), 0, nullptr);

		if (!mountPath)
		{
			MsgError("Package '%s' missing 'mountPath' value\n", targetName);
			return;
		}

		if(encryption)
			dpkWriter.SetEncryption(1, encryption);

		if (targetFilename)
			outputFileName = targetFilename;

		dpkWriter.SetCompression(targetCompression);
		dpkWriter.SetMountPath(mountPath);

		for (int i = 0; i < currentTarget->KeyCount(); ++i)
		{
			KVSection* kvSec = currentTarget->KeyAt(i);
			if (!stricmp(kvSec->GetName(), "add"))
			{
				EqString wildcard = KV_GetValueString(kvSec);
				EqString packageDir = KV_GetValueString(kvSec, 1, wildcard);

				const int wildcardStart = wildcard.Find("*");
				if (wildcardStart == -1 && wildcard.Path_Extract_Name().Length() > 0)
				{
					// try adding file directly
					if(dpkWriter.AddFile(wildcard, packageDir))
						Msg("Adding file '%s' as '%s' to package\n", wildcard.ToCString(), packageDir.ToCString());
				}
				else
				{
					const int fileCount = dpkWriter.AddDirectory(wildcard, packageDir, true);
					Msg("Adding %d files in '%s' as '%s' to package\n", fileCount, wildcard.ToCString(), packageDir.ToCString());
				}
			}
			else if (!stricmp(kvSec->GetName(), "ignoreCompressionExt"))
			{
				const char* extName = KV_GetValueString(kvSec);
				MsgInfo("Ignoring compression for '%s' files\n", extName);
				dpkWriter.AddIgnoreCompressionExtension(extName);
			}
		}
	}
	
	// also make a folder
	outputFileName.Path_FixSlashes();
	const EqString packagePath = outputFileName.Path_Extract_Path().TrimChar(CORRECT_PATH_SEPARATOR);
	if (packagePath.Length() > 0 && packagePath.Path_Extract_Ext().Length() == 0)
		g_fileSystem->MakeDir(packagePath, SP_ROOT);

	dpkWriter.BuildAndSave(outputFileName.ToCString());
}

int main(int argc, char **argv)
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

	g_eqCore->Init("fcompress",argc,argv);

	Msg("FCompress - Equilibrium PakFile generator\n");
	Msg(" Version 2.0\n");

	// initialize file system
	if(!g_fileSystem->Init(false))
		return 0;

	g_cmdLine->ExecuteCommandLine();
	if(g_cmdLine->GetArgumentCount() <= 1)
	{
		Usage();

		g_eqCore->Shutdown();
		return 0;
	}

	EqString outFileName = "";

	for (int i = 0; i < g_cmdLine->GetArgumentCount(); i++)
	{
		EqString argStr = g_cmdLine->GetArgumentString(i);

		if (!argStr.CompareCaseIns("-target"))
		{
			CookPackageTarget(g_cmdLine->GetArgumentsOf(i));
		}
	}

	g_eqCore->Shutdown();

	return 0;
}