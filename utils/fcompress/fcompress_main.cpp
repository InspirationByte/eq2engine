//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: 
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/platform/OSFindData.h"
#include "core/cmd_pacifier.h"
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
	MsgWarning("USAGE:\n	fcompress -target <target name> -set <key> <value>\n");
#if REPACK_SUPPORT
	MsgWarning("			fcompress -repack <EPK v6 filename>\n");
#endif
#if REPACK_SUPPORT
	MsgWarning("			fcompress -devunpack <EPK v7 filename>\n");
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

			IFilePtr file = g_fileSystem->Open(EqString::Format("repack_tmp/%u.epk_blob", finfo.filenameHash), "wb", SP_ROOT);
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

			IFilePtr file = g_fileSystem->Open(EqString::Format("unpack/%u.epk_blob", finfo.filenameHash), "wb", SP_ROOT);
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
		}
	}
}

#endif // REPACK_SUPPORT

static KVSection s_variables;

static void ProcessVariableString(EqString& string)
{
	for (const KVSection* key : s_variables.Keys())
	{
		int found = 0;
		do {
			found = string.ReplaceSubstr(EqString::Format("%%%s%%", key->name), KV_GetValueString(key), true, found);
		} while (found != -1);
	}
}

// TODO: separate source file
class CFileListBuilder
{
public:
	struct FileInfo
	{
		EqString fileName;
		EqString aliasName;
	};

	bool				AddFile(const char* fileName, const char* aliasName);
	int					AddDirectory(const char* pathAndWildcard, const char* aliasPrefix);

	ArrayCRef<FileInfo>	GetFiles() const { return m_files; }
private:

	Array<FileInfo>	m_files{ PP_SL };
};


bool CFileListBuilder::AddFile(const char* fileName, const char* aliasName)
{
	if (!g_fileSystem->FileExist(fileName, SP_ROOT))
		return false;

	m_files.append({ fileName , aliasName });
	return true;
}

int CFileListBuilder::AddDirectory(const char* pathAndWildcard, const char* aliasPrefix)
{
	EqString aliasPrefixTrimmed(aliasPrefix);
	aliasPrefixTrimmed = aliasPrefixTrimmed.TrimChar("\\/.");
	aliasPrefixTrimmed = aliasPrefixTrimmed;

	EqString wildcard;
	EqString nonWildcardFolder(pathAndWildcard);
	const int wildcardStart = nonWildcardFolder.Find("*");
	bool isRecursiveWildcard = false;

	if (wildcardStart != -1)
	{
		wildcard = (nonWildcardFolder.Mid(wildcardStart, nonWildcardFolder.Length() - wildcardStart));
		isRecursiveWildcard = (wildcard[1] == '*');
	}

	if (wildcardStart != -1)
		nonWildcardFolder = nonWildcardFolder.Left(wildcardStart).TrimChar('/');

	int fileCount = 0;

	{
		// first walk files
		OSFindData fileFind;
		if (fileFind.Init(nonWildcardFolder + "/" + (isRecursiveWildcard ? (wildcard.ToCString() + 1) : wildcard.ToCString())))
		{
			do
			{
				if (fileFind.IsDirectory())
					continue;

				const char* name = fileFind.GetCurrentPath();

				EqString targetFilename = EqString::Format("%s/%s", aliasPrefixTrimmed.ToCString(), name);
				if (!fileFind.IsDirectory())
				{
					if (AddFile(EqString::Format("%s/%s", nonWildcardFolder.ToCString(), name), targetFilename))
						++fileCount;
				}
			} while (fileFind.GetNext());
		}
	}

	// then walk directories
	if (wildcardStart == -1 || isRecursiveWildcard)
	{
		OSFindData folderFind;
		if (folderFind.Init(nonWildcardFolder + "/*"))
		{
			do
			{
				if (!folderFind.IsDirectory())
					continue;

				EqStringRef name = folderFind.GetCurrentPath();
				if (name == ".."|| name == ".")
					continue;

				EqString targetFilename = EqString::Format("%s/%s", aliasPrefixTrimmed.ToCString(), name.ToCString());

				fileCount += AddDirectory(EqString::Format("%s/%s/%s", nonWildcardFolder.ToCString(), name, wildcard.ToCString()), targetFilename);

			} while (folderFind.GetNext());
		}
	}

	return fileCount;
}

static bool CheckExtensionList(Array<EqString>& extList, const char* ext)
{
	for (EqString& fromListExt : extList)
	{
		if (!fromListExt.CompareCaseIns(ext))
			return true;
	}
	return false;
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
	if (fnmPathExtractExt(outputFileName).Length() == 0)
		outputFileName = fnmPathApplyExt(outputFileName, s_dpkPackageDefaultExt);

	// load target info
	const KVSection* packages = kvs["Packages"];
	if (!packages)
	{
		MsgError("Missing 'Packages' section in 'PackageCooker.CONFIG'\n");
		return;
	}

	const KVSection* currentTarget = packages->FindSection(targetName);
	if (!currentTarget)
	{
		MsgError("Cannot find package section '%s'\n", targetName);
		return;
	}

	const int targetCompression = KV_GetValueInt(currentTarget->FindSection("compression"), 0, 0);
	EqString targetFilename = KV_GetValueString(currentTarget->FindSection("output"), 0);
	EqString mountPath = KV_GetValueString(currentTarget->FindSection("mountPath"), 0);
	EqString encryption = KV_GetValueString(currentTarget->FindSection("encryption"), 0);

	ProcessVariableString(targetFilename);
	ProcessVariableString(mountPath);
	ProcessVariableString(encryption);

	if (!mountPath.Length())
	{
		MsgError("Package '%s' missing 'mountPath' value\n", targetName);
		return;
	}

	if(encryption.Length())
		MsgInfo("Output package is encrypted with key\n");

	if (targetFilename.Length())
		outputFileName = targetFilename;

	Array<EqString>	ignoreCompressionExt(PP_SL);
	Array<EqString>	keyValueFileExt(PP_SL);
	keyValueFileExt.append("mat");
	keyValueFileExt.append("res");
	keyValueFileExt.append("def");
	keyValueFileExt.append("txt");

	CDPKFileWriter dpkWriter(mountPath, targetCompression, encryption);
	CFileListBuilder fileListBuilder;

	for (int i = 0; i < currentTarget->KeyCount(); ++i)
	{
		const KVSection* kvSec = currentTarget->KeyAt(i);
		if (!CString::CompareCaseIns(kvSec->GetName(), "add"))
		{
			EqString wildcard = KV_GetValueString(kvSec);
			EqString targetNameOrDir = KV_GetValueString(kvSec, 1, wildcard);

			ProcessVariableString(wildcard);
			ProcessVariableString(targetNameOrDir);

			const int wildcardStart = wildcard.Find("*");
			if (wildcardStart == -1 && fnmPathExtractName(wildcard).Length() > 0)
			{
				// try adding file directly
				if(fileListBuilder.AddFile(wildcard, targetNameOrDir))
					Msg("Adding file '%s' as '%s' to package\n", wildcard.ToCString(), targetNameOrDir.ToCString());
			}
			else
			{
				const int fileCount = fileListBuilder.AddDirectory(wildcard, targetNameOrDir);
				Msg("Adding %d files in '%s' as '%s' to package\n", fileCount, wildcard.ToCString(), targetNameOrDir.ToCString());
			}
		}
		else if (!CString::CompareCaseIns(kvSec->GetName(), "ignoreCompressionExt"))
		{
			EqString extName = KV_GetValueString(kvSec);
			ProcessVariableString(extName);

			MsgInfo("Ignoring compression for '%s' files\n", extName.ToCString());
			ignoreCompressionExt.append(extName);
		}
	}
	
	// also make a folder
	fnmPathFixSeparators(outputFileName);
	const EqString packagePath = fnmPathExtractPath(outputFileName).TrimChar(CORRECT_PATH_SEPARATOR);
	if (packagePath.Length() > 0 && fnmPathExtractExt(packagePath).Length() == 0)
		g_fileSystem->MakeDir(packagePath, SP_ROOT);

	if (!fileListBuilder.GetFiles().numElem())
	{
		MsgError("No files added to package '%s'!\n", outputFileName.ToCString());
		return;
	}

	if (dpkWriter.Begin(outputFileName.ToCString()))
	{
		uint64 packedSizeTotal = 0;
		uint64 originalSizeTotal = 0;

		StartPacifier("Adding files, this may take a while: ");

		int numFilesProcessed = 0;
		const int maxFiles = fileListBuilder.GetFiles().numElem();
		for (const CFileListBuilder::FileInfo& fileInfo : fileListBuilder.GetFiles())
		{
			const EqString fileExt = fnmPathExtractExt(fileInfo.fileName);
			const bool skipCompression = CheckExtensionList(ignoreCompressionExt, fileExt);

			int targetFileFlags = (skipCompression ? 0 : DPKFILE_FLAG_COMPRESSED) | DPKFILE_FLAG_ENCRYPTED;

			bool loadRawFile = true;
			CMemoryStream fileMemoryStream(PP_SL);
			fileMemoryStream.Ref_Grab();

			if (CheckExtensionList(keyValueFileExt, fileExt))
			{
				// TODO: convert key-values file and store it (maybe uncompressed)
				KVSection sectionFile;
				if (KV_LoadFromFile(fileInfo.fileName, SP_ROOT, &sectionFile))
				{
					fileMemoryStream.Open(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, 16 * 1024);
					KV_WriteToStreamBinary(&fileMemoryStream, &sectionFile);

					MsgInfo("Converted key-values file to binary: %s\n", fileInfo.fileName.ToCString());
					loadRawFile = false;
				}
			}

			IVirtualStreamPtr stream(&fileMemoryStream);
			if (loadRawFile)
			{
				stream = g_fileSystem->Open(fileInfo.fileName, "rb", SP_ROOT);
				if (fnmPathExtractExt(fileInfo.fileName) == s_dpkPackageDefaultExt)
				{
					// validate EPK file
					dpkheader_t hdr;
					stream->Read(hdr);
					if (hdr.signature == DPK_SIGNATURE && hdr.version == DPK_VERSION)
					{
						MsgInfo("Embedded package file %s\n", fileInfo.fileName.ToCString());

						// disable encryption and conversion for embedded packages
						targetFileFlags &= ~(DPKFILE_FLAG_COMPRESSED | DPKFILE_FLAG_ENCRYPTED);
						ASSERT(DPK_IsBlockFile(targetFileFlags) == false);
					}
					stream->Seek(0, VS_SEEK_SET);
				}
			}

			const uint packedSize = dpkWriter.Add(stream, fileInfo.aliasName, targetFileFlags);

			originalSizeTotal += stream->GetSize();
			packedSizeTotal += packedSize;

			if ((dpkWriter.GetFileCount() % 500) == 0)
				dpkWriter.Flush();

			UpdatePacifier((float)numFilesProcessed / (float)maxFiles);
			++numFilesProcessed;
		}

		EndPacifier();

		const float compressionRatio = 1.0f - (float)packedSizeTotal / (float)originalSizeTotal;
		Msg("Compression is %.2f %%\n", compressionRatio * 100.0f);

		dpkWriter.End();
	}
	else
		MsgError("Cannot create package file '%s'!\n", outputFileName.ToCString());
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
		else if (!argStr.CompareCaseIns("-set") && g_cmdLine->GetArgumentCount())
		{
			const char* keyValue[2];
			const int numValues = g_cmdLine->GetArgumentsOf(i, keyValue, elementsOf(keyValue));
			if (numValues != 2) {
				Msg("-set: key and value are required\n");
				continue;
			}
			s_variables.SetKey(keyValue[0], keyValue[1]);
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
