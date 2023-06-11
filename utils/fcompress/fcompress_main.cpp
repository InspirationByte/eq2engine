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

#include "dpk/DPKFileWriter.h"

#include <zlib.h>
#include <lz4.h>

#if defined(_WIN32)
#include <direct.h>
#if defined(_DEBUG)
#include <crtdbg.h>
#endif
#endif

#define REPACK_SUPPORT 0
#define UNPACK_SUPPORT 0

static void Usage()
{
	MsgWarning("USAGE:\n	fcompress -target <target name>\n");
#if REPACK_SUPPORT
	MsgWarning("USAGE:\n	fcompress -repack <EPK v6 filename>\n");
#endif
#if REPACK_SUPPORT
	MsgWarning("USAGE:\n	fcompress -devunpack <EPK v7 filename>\n");
#endif
}

#if REPACK_SUPPORT
#define DPK_VERSION_PREV			6

// data package file info
struct dpkfile_v6_info_s
{
	int		filenameHash;

	uint64	offset;
	uint32	size;				// The real file size

	short	numBlocks;			// number of blocks
	short	flags;
};
ALIGNED_TYPE(dpkfile_v6_info_s, 2) dpkfile_v6_info_t;

static bool UpdatePackage(const char* targetName)
{
	FILE* dpkFile = fopen(targetName, "rb");

	// Now fill the header data and create object table
	if (!dpkFile)
	{
		MsgError("Cannot open package '%s'\n", targetName);
		return false;
	}
	defer{
		fclose(dpkFile);
	};

	dpkheader_t m_header;

	fread(&m_header, sizeof(dpkheader_t), 1, dpkFile);

	if (m_header.signature != DPK_SIGNATURE)
	{
		MsgError("'%s' is not a package!!!\n", targetName);
		return false;
	}

	if (m_header.version != DPK_VERSION_PREV)
	{
		MsgError("package '%s' has wrong version!!!\n", targetName);
		return false;
	}

	char dpkMountPath[DPK_STRING_SIZE];
	fread(dpkMountPath, DPK_STRING_SIZE, 1, dpkFile);

	Msg("Mount path '%s'\n", dpkMountPath);

	// Let set the file info data origin
	fseek(dpkFile, m_header.fileInfoOffset, SEEK_SET);

	Array<dpkfile_v6_info_t> m_dpkFiles{ PP_SL };
	m_dpkFiles.setNum(m_header.numFiles);
	fread(m_dpkFiles.ptr(), sizeof(dpkfile_v6_info_t), m_header.numFiles, dpkFile);

	{
		Msg("Unpacking %d files\n", m_header.numFiles);

		g_fileSystem->MakeDir("repack_tmp", SP_ROOT);

		// unpack as .epk.blob
		for (int i = 0; i < m_dpkFiles.numElem(); ++i)
		{
			const dpkfile_v6_info_t& finfo = m_dpkFiles[i];

			ASSERT_MSG(!(finfo.flags & DPKFILE_FLAG_ENCRYPTED), "Sorry, encrypted packages are not repackageable atm");

			IFile* file = g_fileSystem->Open(EqString::Format("repack_tmp/%u.epk_blob", finfo.filenameHash), "wb", SP_ROOT);
			if (!file)
				continue;

			ubyte* tmpFileData = (ubyte*)PPAlloc(finfo.size);

			ubyte* tmpBlock = (ubyte*)PPAlloc(DPK_BLOCK_MAXSIZE + 128);

			fseek(dpkFile, finfo.offset, SEEK_SET);
			for (int j = 0; j < finfo.numBlocks; ++j)
			{
				dpkblock_t blockHdr;
				fread(&blockHdr, 1, sizeof(blockHdr), dpkFile);

				// read and decompress block
				fread(tmpBlock, 1, blockHdr.compressedSize, dpkFile);

				unsigned long destLen = blockHdr.size;
				int status = uncompress(tmpFileData + DPK_BLOCK_MAXSIZE * j, &destLen, tmpBlock, blockHdr.compressedSize);

				if (status != Z_OK)
					ASSERT_FAIL(EqString::Format("Cannot decompress file block - %d!\n", status).ToCString());

				ASSERT(destLen == blockHdr.size);
			}
			PPFree(tmpBlock);

			file->Write(tmpFileData, finfo.size, 1);
			PPFree(tmpFileData);

			g_fileSystem->Close(file);
		}

		fclose(dpkFile);
		dpkFile = nullptr;
	}

	{
		// rename into BAK file
		g_fileSystem->Rename(targetName, _Es(targetName) + ".bak", SP_ROOT);

		// now repack the to the latest version
		CDPKFileWriter dpkWriter;

		// add files
		for (int i = 0; i < m_dpkFiles.numElem(); ++i)
		{
			const dpkfile_v6_info_t& finfo = m_dpkFiles[i];
			dpkWriter.AddFile(EqString::Format("repack_tmp/%u.epk_blob", finfo.filenameHash), finfo.filenameHash);
		}

		dpkWriter.SetMountPath(dpkMountPath);
		dpkWriter.AddIgnoreCompressionExtension("lev");
		dpkWriter.SetCompression(m_header.compressionLevel);
		dpkWriter.BuildAndSave(targetName);

		CFileSystemFind find("repack_tmp/*", SP_ROOT);
		while (find.Next())
			g_fileSystem->FileRemove(EqString::Format("repack_tmp/%s", find.GetPath()), SP_ROOT);
		g_fileSystem->RemoveDir("repack_tmp", SP_ROOT);
	}
}

#endif // REPACK_SUPPORT

#if UNPACK_SUPPORT

static bool DevUnpackPackage(const char* targetName)
{
	FILE* dpkFile = fopen(targetName, "rb");

	// Now fill the header data and create object table
	if (!dpkFile)
	{
		MsgError("Cannot open package '%s'\n", targetName);
		return false;
	}
	defer{
		fclose(dpkFile);
	};

	dpkheader_t m_header;

	fread(&m_header, sizeof(dpkheader_t), 1, dpkFile);

	if (m_header.signature != DPK_SIGNATURE)
	{
		MsgError("'%s' is not a package!!!\n", targetName);
		return false;
	}

	if (m_header.version != DPK_VERSION)
	{
		MsgError("package '%s' has wrong version!!!\n", targetName);
		return false;
	}

	char dpkMountPath[DPK_STRING_SIZE];
	fread(dpkMountPath, DPK_STRING_SIZE, 1, dpkFile);

	Msg("Mount path '%s'\n", dpkMountPath);

	// Let set the file info data origin
	fseek(dpkFile, m_header.fileInfoOffset, SEEK_SET);

	Array<dpkfileinfo_t> m_dpkFiles{ PP_SL };
	m_dpkFiles.setNum(m_header.numFiles);
	fread(m_dpkFiles.ptr(), sizeof(dpkfileinfo_t), m_header.numFiles, dpkFile);

	{
		Msg("Unpacking %d files\n", m_header.numFiles);

		g_fileSystem->MakeDir("unpack", SP_ROOT);

		// unpack as .epk.blob
		for (int i = 0; i < m_dpkFiles.numElem(); ++i)
		{
			const dpkfileinfo_t& finfo = m_dpkFiles[i];

			ASSERT_MSG(!(finfo.flags & DPKFILE_FLAG_ENCRYPTED), "Sorry, encrypted packages are not unpackable atm");

			IFile* file = g_fileSystem->Open(EqString::Format("unpack/%u.epk_blob", finfo.filenameHash), "wb", SP_ROOT);
			if (!file)
				continue;

			ubyte* tmpFileData = (ubyte*)PPAlloc(finfo.size);

			ubyte* tmpBlock = (ubyte*)PPAlloc(DPK_BLOCK_MAXSIZE + 128);

			fseek(dpkFile, finfo.offset, SEEK_SET);
			for (int j = 0; j < finfo.numBlocks; ++j)
			{
				dpkblock_t blockHdr;
				fread(&blockHdr, 1, sizeof(blockHdr), dpkFile);

				// read and decompress block
				fread(tmpBlock, 1, blockHdr.compressedSize, dpkFile);

				const int destLen = LZ4_decompress_safe((char*)tmpBlock, (char*)tmpFileData + DPK_BLOCK_MAXSIZE * j, blockHdr.compressedSize, blockHdr.size);

				ASSERT(destLen == blockHdr.size);
			}
			PPFree(tmpBlock);

			file->Write(tmpFileData, finfo.size, 1);
			PPFree(tmpFileData);

			g_fileSystem->Close(file);
		}
	}
}

#endif // REPACK_SUPPORT

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
		{
			MsgInfo("Output package is encrypted with key\n");
			dpkWriter.SetEncryption(1, encryption);
		}

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
#if REPACK_SUPPORT
		else if (!argStr.CompareCaseIns("-repack"))
		{
			UpdatePackage(g_cmdLine->GetArgumentsOf(i));
		}
#endif
#if UNPACK_SUPPORT
		else if (!argStr.CompareCaseIns("-devunpack"))
		{
			DevUnpackPackage(g_cmdLine->GetArgumentsOf(i));
		}
#endif
	}

	g_eqCore->Shutdown();

	return 0;
}
