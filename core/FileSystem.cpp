//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef CloseModule
#	undef CloseModule
#endif

#include <direct.h>	// mkdir()
#include <io.h>		// _access()
#define access		_access
#define F_OK		0       // Test for existence.
#else
#include <unistd.h> // rmdir(), access()
#include <dlfcn.h>	// dlopen()
#include <dirent.h> // opendir, readdir
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "core/core_common.h"

#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/ILocalize.h"
#include "core/platform/OSFindData.h"

#include "utils/KeyValues.h"
#include "FileSystem.h"

#include "dpk/BasePackageFileReader.h"

static int FSStringId(const char *str)
{
	constexpr int MULTIPLIER = 37;

	ASSERT(str);
	int hash = 0;
	for (const char* p = str; *p; p++)
		hash = MULTIPLIER * hash + (int)CType::LowerChar(*p);

	return hash;
}

#ifndef _WIN32
using HMODULE = void*;
#endif

using namespace Threading;
static CEqMutex	s_FSMutex;

EXPORTED_INTERFACE(IFileSystem, CFileSystem);

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

CFile::CFile(const char* fileName, COSFile&& file)
	: m_name(fileName)
	, m_osFile(std::move(file))
{
}

VSSize CFile::Seek(int64 pos, EVirtStreamSeek seekType )
{
	return m_osFile.Seek(pos, static_cast<COSFile::ESeekPos>(seekType));
}

VSSize CFile::Tell() const
{
	return m_osFile.Tell();
}

VSSize CFile::Read( void *dest, VSSize count, VSSize size)
{
	VSSize numBytes = count * size;
	if (numBytes <= 0)
		return 0;
	return m_osFile.Read(dest, numBytes) / size;
}

VSSize CFile::Write( const void *src, VSSize count, VSSize size)
{
	VSSize numBytes = count * size;
	if (numBytes <= 0)
		return 0;
	return m_osFile.Write(src, numBytes) / size;
}

bool CFile::Flush()
{
	return m_osFile.Flush();
}

VSSize CFile::GetSize()
{
	const VSSize currentPos = Tell();
	Seek(0, VS_SEEK_END);

	const VSSize size = Tell();
	Seek(currentPos, VS_SEEK_SET);

	return size;
}

uint32 CFile::GetCRC32()
{
	const VSSize pos = Tell();
	const VSSize fileSize = GetSize();

	ubyte* fileData = (ubyte*)PPAlloc(fileSize + 16);
	Read(fileData, 1, fileSize);
	Seek(pos, VS_SEEK_SET);

	const uint32 crc = CRC32_BlockChecksum(fileData, fileSize);
	PPFree(fileData);

	return crc;
}

//------------------------------------------------------------------------------

class CFlatFileReader : public CBasePackageReader
{
public:
	EPackageType		GetType() const { return PACKAGE_READER_FLAT; }

	bool				InitPackage(const char* filename, const char* mountPath /*= nullptr*/);
	IFilePtr			Open(const char* filename, int modeFlags);
	bool				FileExists(const char* filename) const;

	// stubs
	bool				OpenEmbeddedPackage(CBasePackageReader* target, const char* filename) { return false; }
	IFilePtr			Open(int fileIndex, int modeFlags) { return nullptr; }
	int					FindFileIndex(const char* filename) const { return -1; }
};

bool CFlatFileReader::InitPackage(const char* filename, const char* mountPath /*= nullptr*/)
{
	m_packagePath = filename;
	return true;
}

IFilePtr CFlatFileReader::Open(const char* filename, int modeFlags)
{
	if (modeFlags != VS_OPEN_READ)
		return nullptr;

	EqString filePath;
	fnmPathCombine(filePath, m_packagePath, filename);

	return g_fileSystem->Open(filePath, "r");
}

bool CFlatFileReader::FileExists(const char* filename) const
{
	EqString filePath;
	fnmPathCombine(filePath, m_packagePath, filename);

	return g_fileSystem->FileExist(filePath);
}

//------------------------------------------------------------------------------
// Main filesystem code
//------------------------------------------------------------------------------

struct DKMODULE
{
	char	name[256];
	HMODULE module;
};

struct DKFINDDATA
{
	OSFindData	osFind;
	EqString	wildcard;
	int			searchPaths{ -1 };
	int			dirIndex{ 0 };
	bool		singleDir{ false };
};

struct SearchPathInfo
{
#ifndef _WIN32
	// name hash to file path mapping (case-insensitive support)
	Map<int, EqString> pathToFileMapping{ PP_SL };
#endif
	EqString	id;
	EqString	path;
	bool		mainWritePath;
};

//--------------------------------------------------

static ESearchPath GetSearchPathByName(EqStringRef str)
{
	if (!str.CompareCaseIns("SP_MOD"))
		return SP_MOD;
	else if (!str.CompareCaseIns("SP_DATA"))
		return SP_DATA;

	return SP_ROOT;
}

static bool mkdirRecursive(const char* path, bool includeDotPath)
{
	char folder[265];
	const char* end;
	const char* curend;

	end = path;

	do
	{
		// skip any separators in the beginning
		while (*end == CORRECT_PATH_SEPARATOR)
			end++;

		// get next string part
		curend = (char*)strchr(end, CORRECT_PATH_SEPARATOR);

		if (curend)
			end = curend;
		else
			end = path + strlen(path);

		strncpy(folder, path, end - path);
		folder[end - path] = 0;

		// stop on file extension if needed
		if (!includeDotPath && strchr(folder, '.'))
			break;

		int result;
#ifdef _WIN32
		result = _mkdir(folder);
#else
		result = mkdir(folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

		if (result > 0 && result != EEXIST)
			return false;

	} while (curend);

	return true;
}

//--------------------------------------------------

CFileSystem::CFileSystem()
{
	g_eqCore->RegisterInterface(this);
}

CFileSystem::~CFileSystem()
{
	g_eqCore->UnregisterInterface<CFileSystem>();
}

bool CFileSystem::Init(bool bEditorMode)
{
	Msg("\n-------- Filesystem Init --------\n\n");

	m_editorMode = bEditorMode;

	const KVSection* fsConfig = g_eqCore->GetConfig()->FindSection("FileSystem", KV_FLAG_SECTION);
	if (!fsConfig)
	{
		Msg("E2.CONFIG missing FileSystemDirectories section!\n");
		return false;
	}

	EqStringRef workDir;
	if (fsConfig->Get("WorkDir").GetValues(workDir))
	{
#ifdef _WIN32
		SetCurrentDirectoryA(workDir);
#else
		chdir(workDir);
#endif // _WIN32
	}

	EqStringRef basePath;
	if(fsConfig->Get("BasePath").GetValues(basePath))
	{
		SetBasePath(basePath);
		if (m_basePath.Length() > 0)
			MsgInfo("* Base directory: %s\n", m_basePath.GetData());
	}

	m_dataDir = "EngineBase";
	fsConfig->Get("EngineDataDir").GetValues(m_dataDir);
	MsgInfo("* Engine Data directory: %s\n", m_dataDir.GetData());

	if(!m_editorMode)
	{
		EqStringRef gamePath = "DefaultGameDir_MISSING";
		fsConfig->Get("DefaultGameDir").GetValues(gamePath);

		const int gamePathArg = g_cmdLine->FindArgument("-game");
		if(gamePathArg != -1)
			gamePath = g_cmdLine->GetArgumentsOf(gamePathArg);

		AddSearchPath("$GAME$", gamePath);

		MsgInfo("* Game Data directory: %s\n", GetCurrentGameDirectory());

		// FS dev addon for game tools
		const int iDevAddonPathArg = g_cmdLine->FindArgument("-devAddon");
		const char* devAddonPath = g_cmdLine->GetArgumentsOf(iDevAddonPathArg);
		if (devAddonPath)
		{
			AddSearchPath("$MOD$_$WRITE$", devAddonPath);
			MsgInfo("* Dev addon path: %s\n", devAddonPath);
		}
	}

	for(const KVSection* pkgSec : fsConfig->Keys("AddPackage"))
	{
		EqStringRef packageName;
		EqStringRef pathType = "SP_MOD";
		EqStringRef mountPath;
		if (pkgSec->GetValues(packageName, pathType, mountPath) < 1)
			continue;

		const ESearchPath type = GetSearchPathByName(pathType);
		AddPackage(packageName, type, mountPath);
	}

	m_isInit = true;
    return true;
}

void CFileSystem::Shutdown()
{
	m_isInit = false;

	for(int i = 0; i < m_modules.numElem(); i++)
	{
		CloseModule(m_modules[i]);
		i--;
	}

	for(int i = 0; i < m_fsPackages.numElem(); i++)
		delete m_fsPackages[i];

	m_fsPackages.clear(true);
	m_findDatas.clear(true);

	for(int i = 0; i < m_directories.numElem(); i++)
		delete m_directories[i];

	m_directories.clear(true);
}

void CFileSystem::SetBasePath(const char* path) 
{ 
	m_basePath = path;
	fnmPathFixSeparators(m_basePath);

	if(m_basePath[m_basePath.Length()-1] != CORRECT_PATH_SEPARATOR)
		m_basePath.Append(CORRECT_PATH_SEPARATOR);
}

EqString CFileSystem::FindFilePath(const char* filename, int searchFlags /*= -1*/) const
{
	EqString existingFilePath;

	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (access(filePath, F_OK) != -1)
		{
			existingFilePath = filePath;
			return true;
		}

		return false;
	};

	WalkOverSearchPaths(searchFlags, filename, walkFileFunc);

	return existingFilePath;
}

IFilePtr CFileSystem::Open(const char* filename, const char* mode, int searchFlags/* = -1*/ )
{
	ASSERT_MSG(filename, "Open - Must specify 'filename'");
	ASSERT_MSG(mode, "Open - Must specify 'mode'");

	if (!filename || !mode)
		return nullptr;

	int modeFlags = 0;
	while (*mode)
	{
		switch (CType::LowerChar(*mode))
		{
		case 'r':
			modeFlags |= COSFile::READ;
			break;
		case 'w':
			modeFlags |= COSFile::WRITE;
			break;
		case 'a':
		case '+':
			modeFlags |= COSFile::WRITE | COSFile::APPEND;
			break;
		}
		++mode;
	}

	const bool isWrite = modeFlags & (COSFile::APPEND | COSFile::WRITE);

	IFilePtr fileHandle;
	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (isWrite && !writePath)
			return false;

		COSFile osFile;
		if (osFile.Open(filePath, modeFlags))
		{
			fileHandle = IFilePtr(CRefPtr_new(CFile, filename, std::move(osFile)));
			return true;
		}

		if (isWrite)
			return false;

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageReader* fsPacakage = m_fsPackages[j];

			if (!(spFlags & fsPacakage->GetSearchPath()))
				continue;

			EqString pkgFileName;
			if (!fsPacakage->GetInternalFileName(pkgFileName, filePath.ToCString() + m_basePath.Length()))
				continue;

			// package readers do not support base path, get rid of it
			fileHandle = fsPacakage->Open(pkgFileName, modeFlags);
			if (fileHandle)
				return true;
		}

		return false;
	};

	WalkOverSearchPaths(searchFlags, filename, walkFileFunc);
	return fileHandle;
}

ubyte* CFileSystem::GetFileBuffer(const char* filename, VSSize* filesize/* = 0*/, int searchFlags/* = -1*/)
{
	IFilePtr pFile = Open(filename, "rb", searchFlags);

    if (!pFile)
        return nullptr;

    const VSSize length = pFile->GetSize();
    ubyte *buffer = (ubyte*)PPAlloc(length + 1);

    if (!buffer)
        return nullptr;

	memset(buffer, 0, length+1);

	pFile->Read(buffer, 1, length);
    buffer[length] = 0;

    if (filesize)
        *filesize = length;

    return buffer;
}

VSSize CFileSystem::GetFileSize(const char* filename, int searchFlags/* = -1*/)
{
    IFilePtr file = Open(filename,"rb",searchFlags);
    if (!file)
        return 0;

	return file->GetSize();
}

uint32 CFileSystem::GetFileCRC32(const char* filename, int searchFlags)
{
    IFilePtr file = Open(filename,"rb",searchFlags);
    if (!file)
        return 0;

    return file->GetCRC32();
}


bool CFileSystem::FileCopy(const char* filename, const char* dest_file, bool overWrite, ESearchPath search)
{
	char buf[4096];

	if( FileExist(filename, search) && (overWrite || FileExist(dest_file, search) == false))
	{
		IFilePtr fpWrite = Open(dest_file, "wb", search);
		IFilePtr fpRead = Open(filename, "rb", search);

		if (!fpRead || !fpWrite)
			return false;

		VSSize nread;
		while (nread = fpRead->Read(buf, sizeof(buf), 1), nread > 0)
		{
			char* outPtr = buf;
			VSSize nwritten;

			do {
				nwritten = fpWrite->Write(outPtr, nread, 1);

				if (nwritten >= 0)
				{
					nread -= nwritten;
					outPtr += nwritten;
				}
			} while (nread > 0);
		}

		ASSERT(nread == 0);
	}
	else
		return false;

	return true;
}

bool CFileSystem::FileExist(const char* filename, int searchFlags) const
{
	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (access(filePath, F_OK) != -1)
			return true;

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			const CBasePackageReader* fsPacakage = m_fsPackages[j];

			if (!(spFlags & fsPacakage->GetSearchPath()))
				continue;

			EqString pkgFileName;
			if (!fsPacakage->GetInternalFileName(pkgFileName, filePath.ToCString() + m_basePath.Length()))
				continue;

			// package readers do not support base path, get rid of it
			if (fsPacakage->FileExists(pkgFileName))
				return true;
		}

		return false;
	};

	return WalkOverSearchPaths(searchFlags, filename, walkFileFunc);
}

EqString CFileSystem::GetSearchPath(ESearchPath search, int directoryId) const
{
	EqString searchPath;
	switch (search)
	{
		case SP_DATA:
			fnmPathCombine(searchPath, m_basePath, m_dataDir);
			break;
		case SP_MOD:
			if(directoryId == -1) // default write path
				fnmPathCombine(searchPath, m_basePath, GetCurrentGameDirectory());
			else
				fnmPathCombine(searchPath, m_basePath, m_directories[directoryId]->path);
			break;
		case SP_ROOT:
			searchPath = m_basePath;
			break;
		default:
			searchPath = ".";
			break;
	}

	return searchPath;
}

static bool UTIL_IsAbsolutePath(const char* dirOrFileName)
{
	return (dirOrFileName[0] == CORRECT_PATH_SEPARATOR || CType::IsAlphabetic(dirOrFileName[0]) && dirOrFileName[1] == ':');
}

EqString CFileSystem::GetAbsolutePath(ESearchPath search, const char* dirOrFileName) const
{
	EqString fullPath;

	const bool isAbsolutePath = (search == SP_ROOT && UTIL_IsAbsolutePath(dirOrFileName));

	if (!isAbsolutePath)
		fnmPathCombine(fullPath, GetSearchPath(search).ToCString(), dirOrFileName);
	else
		fullPath = dirOrFileName;

	fnmPathFixSeparators(fullPath);

	return fullPath;
}

void CFileSystem::FileRemove(const char* filename, ESearchPath search ) const
{
	remove(GetAbsolutePath(search, filename));
}

bool CFileSystem::DirExist(const char* dirname, ESearchPath search) const
{
	struct stat info;
	if (stat(GetAbsolutePath(search, dirname), &info) != 0)
		return false;

	return info.st_mode & S_IFDIR;
}

//Directory operations
void CFileSystem::MakeDir(const char* dirname, ESearchPath search ) const
{
	mkdirRecursive(GetAbsolutePath(search, dirname), true);
}

void CFileSystem::RemoveDir(const char* dirname, ESearchPath search ) const
{
    rmdir(GetAbsolutePath(search, dirname));
}

void CFileSystem::Rename(const char* oldNameOrPath, const char* newNameOrPath, ESearchPath search) const
{
	rename(GetAbsolutePath(search, oldNameOrPath), GetAbsolutePath(search, newNameOrPath));
}

bool CFileSystem::WalkOverSearchPaths(int searchFlags, const char* fileName, const SPWalkFunc& func) const
{
	int flags = searchFlags;
	if (flags == -1)
		flags = SP_MOD | SP_DATA | SP_ROOT;

	const bool isAbsolutePath = UTIL_IsAbsolutePath(fileName);
	if (isAbsolutePath)
		flags = SP_ROOT;

	// First we checking mod directory
	if (flags & SP_MOD)
	{
		for (const SearchPathInfo* spInfo : m_directories)
		{
			EqString filePath;
			fnmPathCombine(filePath, m_basePath, spInfo->path, fileName);

#ifndef _WIN32
			const int nameHash = FSStringId(filePath);
			const auto it = spInfo->pathToFileMapping.find(nameHash);
			if (!it.atEnd())
			{
				// apply correct filepath
				filePath = *it;
			}
#endif

			if (func(filePath, SP_MOD, flags, spInfo->mainWritePath))
				return true;
		}
	}

	//Then we checking data directory
	if (flags & SP_DATA)
	{
		EqString filePath;
		fnmPathCombine(filePath, m_basePath, m_dataDir, fileName);
		fnmPathFixSeparators(filePath);

		if (func(filePath, SP_DATA, flags, false))
			return true;
	}

	// And checking root.
	// not adding basepath to this
	if (flags & SP_ROOT)
	{
		EqString filePath;

		if(isAbsolutePath)
		{
			filePath = fileName;
			fnmPathFixSeparators(filePath);
		}
		else
			fnmPathCombine(filePath, m_basePath, fileName);
		
		// TODO: write path detection if it's same as ones from m_directories or m_dataDir
		if (func(filePath, SP_ROOT, flags, true))
			return true;
	}

	return false;
}

bool CFileSystem::SetAccessKey(const char* accessKey)
{
	m_accessKey = accessKey;
	return m_accessKey.Length() > 0;
}

bool CFileSystem::AddPackage(const char* packageName, ESearchPath type, const char* mountPath /*= nullptr*/)
{
	const EqString packagePath = g_fileSystem->GetAbsolutePath(SP_ROOT, packageName);

	for(CBasePackageReader* package : m_fsPackages)
	{
		if(!packagePath.CompareCaseIns(package->GetPackageFilename()))
		{
			ASSERT_FAIL("Package %s was already added to file system", packageName);
			return false;
		}
	}

	CBasePackageReader* reader = CBasePackageReader::CreateReaderByExtension(packageName);
	if (!reader->InitPackage(packagePath, mountPath))
	{
		MsgError("Cannot open package '%s'\n", packagePath.ToCString());
		delete reader;
		return false;
	}

	if(mountPath)
		DevMsg(DEVMSG_FS, "Adding package file '%s' force mount at '%s'\n", packageName, mountPath);
	else
		DevMsg(DEVMSG_FS, "Adding package file '%s'\n", packageName);

    reader->SetSearchPath(type);
	reader->SetKey(m_accessKey);

    m_fsPackages.append(reader);
    return true;
}

void CFileSystem::RemovePackage(const char* packageName)
{
	const EqString packagePath = g_fileSystem->GetAbsolutePath(SP_ROOT, packageName);

	for (int i = 0; i < m_fsPackages.numElem(); i++)
	{
		CBasePackageReader* reader = m_fsPackages[i];

		if (!packagePath.CompareCaseIns(reader->GetPackageFilename()))
		{
			m_fsPackages.fastRemoveIndex(i);
			delete reader;
			return;
		}
	}
}

// opens package for further reading. Does not add package as FS layer
IFilePackageReader* CFileSystem::OpenPackage(const char* packageName, int searchFlags)
{
	CBasePackageReader* reader = CBasePackageReader::CreateReaderByExtension(packageName);

	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (reader->InitPackage(filePath, nullptr))
			return true;

		if (g_fileSystem->DirExist(filePath, searchPath))
		{
			delete reader;

			// open flat reader
			reader = PPNew CFlatFileReader();
			reader->InitPackage(filePath, nullptr);
			return true;
		}

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageReader* fsPacakage = m_fsPackages[j];

			if (!(spFlags & fsPacakage->GetSearchPath()))
				continue;

			EqString pkgFileName;
			if (!fsPacakage->GetInternalFileName(pkgFileName, filePath.ToCString() + m_basePath.Length()))
				continue;

			// trying to open package file inside package (embedded DPK)
			if (fsPacakage->OpenEmbeddedPackage(reader, pkgFileName))
				return true;
		}

		return false;
	};

	if (!WalkOverSearchPaths(searchFlags, packageName, walkFileFunc))
	{
		delete reader;
		MsgError("Cannot open package '%s'\n", packageName);

		return nullptr;
	}

	reader->SetSearchPath(SP_ROOT);
	reader->SetKey(m_accessKey);

	{
		CScopedMutex m(s_FSMutex);
		m_openPackages.append(reader);
	}

	return reader;
}

void CFileSystem::ClosePackage(IFilePackageReader* package)
{
	bool removed = false;
	{
		CScopedMutex m(s_FSMutex);
		removed = m_openPackages.fastRemove(package);
	}

	if (removed)
		delete package;
}

void CFileSystem::MapFiles(SearchPathInfo& pathInfo)
{
#ifndef _WIN32
	Array<EqString> openSet(PP_SL);
	openSet.reserve(5000);

	fnmPathCombine(openSet.append(), m_basePath, pathInfo.path);

	while (openSet.numElem())
	{
		EqString path = openSet.popFront();

		DIR* dir = opendir(path);
		if (!dir)
			break;

		do
		{
			dirent* entry = readdir(dir);
			if (!entry)
				break;

			if (*entry->d_name == 0)
				continue;

			if (*entry->d_name == '.')
				continue;

			EqString entryName(EqString::Format("%s/%s", path.TrimChar(CORRECT_PATH_SEPARATOR).ToCString(), entry->d_name));

			bool isEntryDir = false;
			struct stat st;
			if (stat(entryName, &st) == 0 && (st.st_mode & S_IFDIR))
			{
				openSet.append(entryName);
			}
			else
			{
				// add a mapping
				const int nameHash = FSStringId(entryName);
				pathInfo.pathToFileMapping.insert(nameHash, entryName);
			}
		} while (true);
	}

	if (pathInfo.pathToFileMapping.size())
		DevMsg(DEVMSG_FS, "Mapped %d files for '%s'\n", pathInfo.pathToFileMapping.size(), pathInfo.path.ToCString());
#endif // _WIN32
}

// sets fallback directory for mod
void CFileSystem::AddSearchPath(const char* pathId, const char* pszDir)
{
	const int idx = arrayFindIndexF(m_directories, [=](const SearchPathInfo* spInfo) {
		return spInfo->id == pathId;
	});
	if (idx != -1)
	{
		ErrorMsg("AddSearchPath Error: pathId %s already added", pathId);
		return;
	}

	DevMsg(DEVMSG_FS, "Adding search patch '%s' at '%s'\n", pathId, pszDir);

	const bool isReadPriorityPath = strstr(pathId, "$MOD$") || strstr(pathId, "$LOCALIZE$");
	const bool isWriteablePath = strstr(pathId, "$WRITE$");

	SearchPathInfo* pathInfo = PPNew SearchPathInfo;
	pathInfo->id = pathId;
	pathInfo->path = pszDir;
	pathInfo->mainWritePath = !isReadPriorityPath || isWriteablePath;

	if(isReadPriorityPath)
		m_directories.insert(pathInfo, 0);
	else
		m_directories.append(pathInfo);

	if(!pathInfo->mainWritePath)
		MapFiles(*pathInfo);
}

void CFileSystem::RemoveSearchPath(const char* pathId)
{
	for(int i = 0; i < m_directories.numElem(); i++)
	{
		if(m_directories[i]->id == EqStringRef(pathId))
		{
			DevMsg(DEVMSG_FS, "Removing search patch '%s'\n", pathId);
			delete m_directories[i];
			m_directories.removeIndex(i);
			break;
		}
	}
}

// Returns current game path
const char* CFileSystem::GetCurrentGameDirectory() const
{
	// return first directory with 'mainWritePath' attribute set
	for (const SearchPathInfo* spInfo : m_directories)
	{
		if (spInfo->mainWritePath)
			return spInfo->path;
	}

    return m_dataDir.GetData();
}

// Returns current engine data path
const char* CFileSystem::GetCurrentDataDirectory() const
{
    return m_dataDir.GetData();
}

bool CFileSystem::InitNextPath(DKFINDDATA* findData) const
{
	EqString fsBaseDir;
	EqString searchWildcard;

	// Try Game paths
	if (findData->searchPaths & SP_MOD)
	{
		while (findData->dirIndex < m_directories.numElem())
		{
			fsBaseDir = GetSearchPath(SP_MOD, findData->dirIndex++);
			fnmPathCombine(searchWildcard, fsBaseDir, findData->wildcard);
			if (findData->osFind.Init(searchWildcard))
				return true;

			if(findData->singleDir)
				break;
		}
		findData->searchPaths &= ~SP_MOD;
	}
	
	// Try Data
	if (findData->searchPaths & SP_DATA)
	{
		findData->searchPaths &= ~SP_DATA;
		fsBaseDir = GetSearchPath(SP_DATA, -1);
		fnmPathCombine(searchWildcard, fsBaseDir, findData->wildcard);
		if (findData->osFind.Init(searchWildcard))
			return true;
	}

	// Try Root
	if (findData->searchPaths & SP_ROOT)
	{
		findData->searchPaths &= ~SP_ROOT;
		fsBaseDir = GetSearchPath(SP_ROOT, -1);
		fnmPathCombine(searchWildcard, fsBaseDir, findData->wildcard);
		if (findData->osFind.Init(searchWildcard))
			return true;
	}

	return false;
}

// opens directory for search props
const char* CFileSystem::FindFirst(const char* wildcard, DKFINDDATA** findData, int searchPath, int dirIndex)
{
	ASSERT(findData != nullptr);

	if(findData == nullptr)
		return nullptr;

	ASSERT_MSG(searchPath, "searchPath flags must be specified");

	DKFINDDATA* newFind = PPNew DKFINDDATA;
	newFind->searchPaths = searchPath;
	newFind->wildcard = wildcard;
	fnmPathFixSeparators(newFind->wildcard);
	newFind->dirIndex = max(0, dirIndex);
	newFind->singleDir = (dirIndex >= 0);

	if (!InitNextPath(newFind))
	{
		delete newFind;
		return nullptr;
	}

	m_findDatas.append(newFind);
	*findData = newFind;

	return newFind->osFind.GetCurrentPath();
}

const char* CFileSystem::FindNext(DKFINDDATA* findData) const
{
	if(!findData)
		return nullptr;

	if (findData->osFind.GetNext())
		return findData->osFind.GetCurrentPath();

	// if GetNext failed, reinit in new search path
	if (!InitNextPath(findData))
		return nullptr;

	return findData->osFind.GetCurrentPath();
}

void CFileSystem::FindClose(DKFINDDATA* findData)
{
	if(!findData)
		return;
	if(m_findDatas.fastRemove(findData))
		delete findData;
}

bool CFileSystem::FindIsDirectory(DKFINDDATA* findData) const
{
	if(!findData)
		return false;
	return findData->osFind.IsDirectory();
}

int	CFileSystem::FindGetDirIndex(DKFINDDATA* findData) const
{
	if(!findData)
		return -1;
	return findData->dirIndex - 1;
}

// loads module
DKMODULE* CFileSystem::OpenModule(const char* mod_name, EqString* outError)
{
	EqString moduleFileName = mod_name;
	EqString modExt = fnmPathExtractExt(moduleFileName);

#ifdef _WIN32
	// make default module extension
	if(modExt.Length() == 0)
		moduleFileName = moduleFileName + ".dll";

	HMODULE mod = LoadLibraryA( moduleFileName );
	DWORD lastErr = GetLastError();

	char err[256] = {'\0'};
	if(lastErr != 0)
	{
		FormatMessageA(	FORMAT_MESSAGE_FROM_SYSTEM,
						nullptr,
						lastErr,
						MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
						err,
						255,
						nullptr);
	}

#else
	// make default module extension
	if(modExt.Length() == 0)
		moduleFileName = moduleFileName + ".so";

	HMODULE mod = dlopen( moduleFileName, RTLD_LAZY | RTLD_LOCAL );
	if( !mod )
	{
		moduleFileName = "lib" + moduleFileName;
		mod = dlopen( moduleFileName, RTLD_LAZY | RTLD_LOCAL );
	}

	const char* err = dlerror();
	int lastErr = 0;
#endif // _WIN32 && MEMDLL

	if( !mod )
	{
		if(outError)
			*outError = err;

		return nullptr;
	}

	MsgInfo("Loaded module '%s'\n", moduleFileName.ToCString());

	DKMODULE* pModule = PPNew DKMODULE;
	strcpy( pModule->name, moduleFileName );
	pModule->module = (HMODULE)mod;

	return pModule;
}

// frees module
void CFileSystem::CloseModule( DKMODULE* pModule )
{
	if(!pModule)
		return;

	// don't unload any modules if we prining a leaklog
#ifdef _WIN32
	FreeLibrary(pModule->module);
#else
	dlclose(pModule->module);
#endif // _WIN32 && MEMDLL

	delete pModule;
	m_modules.remove(pModule);
}

// returns procedure address of the loaded module
void* CFileSystem::GetProcedureAddress(DKMODULE* pModule, const char* pszProc)
{
#ifdef _WIN32
	return GetProcAddress( pModule->module, pszProc );
#else
	return dlsym( pModule->module, pszProc );
#endif // _WIN32 && MEMDLL
}
