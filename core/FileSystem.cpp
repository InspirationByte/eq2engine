//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>

#include "core/core_common.h"

#ifdef _WIN32
#include <Windows.h>
#ifdef CloseModule	// Windows.h
#undef CloseModule
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

#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/ILocalize.h"
#include "core/platform/OSFindData.h"

#include "utils/KeyValues.h"
#include "FileSystem.h"

#include "dpk/DPKFileReader.h"
#include "dpk/ZipFileReader.h"

#ifdef PLAT_LINUX
static int FSStringId(const char *str)
{
	constexpr int MULTIPLIER = 37;

	ASSERT(str);
	int hash = 0;
	for (ubyte* p = (ubyte*)str; *p; p++)
		hash = MULTIPLIER * hash + tolower(*p);

	return hash;
}
#endif

using namespace Threading;
static CEqMutex	s_FSMutex;

EXPORTED_INTERFACE(IFileSystem, CFileSystem);

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

CFile::CFile(COSFile&& file)
	: m_osFile(std::move(file))
{
}

int	CFile::Seek( long pos, EVirtStreamSeek seekType )
{
	return m_osFile.Seek(pos, static_cast<COSFile::ESeekPos>(seekType) );
}

long CFile::Tell() const
{
	return m_osFile.Tell();
}

size_t CFile::Read( void *dest, size_t count, size_t size)
{
	size_t numBytes = count * size;
	if (!numBytes)
		return 0;
	return m_osFile.Read(dest, numBytes) / size;
}

size_t CFile::Write( const void *src, size_t count, size_t size)
{
	size_t numBytes = count * size;
	if (!numBytes)
		return 0;

	return m_osFile.Write(src, numBytes) / size;
}

bool CFile::Flush()
{
	return m_osFile.Flush();
}

long CFile::GetSize()
{
	long pos = Tell();

	Seek(0, VS_SEEK_END);

	long length = Tell();

	Seek(pos, VS_SEEK_SET);

	return length;
}

uint32 CFile::GetCRC32()
{
	long pos = Tell();
	long fSize = GetSize();

	ubyte* pFileData = (ubyte*)PPAlloc(fSize+16);

	Read(pFileData, 1, fSize);

	Seek(pos, VS_SEEK_SET);

	uint32 nCRC = CRC32_BlockChecksum( pFileData, fSize );

	PPFree(pFileData);

	return nCRC;
}

//------------------------------------------------------------------------------
// Main filesystem code
//------------------------------------------------------------------------------

#ifndef _WIN32
typedef void* HMODULE;
#endif

struct DKMODULE
{
	char	name[256];
	HMODULE module;
};

struct DKFINDDATA
{
	OSFindData	osFind;
	EqString	wildcard;
	ESearchPath searchPath{ SP_ROOT };
	int			searchPathId{ -1 };
};

//--------------------------------------------------

static ESearchPath GetSearchPathByName(const char* str)
{
	if (!stricmp(str, "SP_MOD"))
		return SP_MOD;
	else if (!stricmp(str, "SP_DATA"))
		return SP_DATA;

	return SP_ROOT;
}

static bool mkdirRecursive(const char* path, bool includeDotPath)
{
	char folder[265];
	const char* end, * curend;

	end = path;

	do
	{
		int result;

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

	KVSection* pFilesystem = g_eqCore->GetConfig()->FindSection("FileSystem", KV_FLAG_SECTION);

	if (!pFilesystem)
	{
		Msg("EQ.CONFIG missing FileSystemDirectories section!\n");
		return false;
	}

	const char* workDir = KV_GetValueString(pFilesystem->FindSection("WorkDir"), 0, nullptr);
	if (workDir)
	{
#ifdef _WIN32
		SetCurrentDirectoryA(workDir);
#else
		chdir(workDir);
#endif // _WIN32
	}

	m_basePath = KV_GetValueString(pFilesystem->FindSection("BasePath"), 0, m_basePath);

	if(m_basePath.Length() > 0)
		MsgInfo("* FS Init with basePath=%s\n", m_basePath.GetData());

	m_dataDir = KV_GetValueString(pFilesystem->FindSection("EngineDataDir"), 0, "EngineBase" );

	MsgInfo("* Engine Data directory: %s\n", m_dataDir.GetData());

	if(!m_editorMode)
	{
		const int iGamePathArg = g_cmdLine->FindArgument("-game");
		const char* gamePath = g_cmdLine->GetArgumentsOf(iGamePathArg);

		// set or change game path
		if (gamePath)
			AddSearchPath("$GAME$", gamePath);
		else
			AddSearchPath("$GAME$", (const char*)KV_GetValueString(pFilesystem->FindSection("DefaultGameDir"), 0, "DefaultGameDir_MISSING"));

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

	for(int i = 0; i < pFilesystem->keys.numElem(); i++)
	{
		if(!stricmp(pFilesystem->keys[i]->name, "AddPackage" ))
		{
			ESearchPath packageSearchPathFlag = GetSearchPathByName(KV_GetValueString(pFilesystem->keys[i], 1, "SP_MOD"));

			AddPackage(KV_GetValueString(pFilesystem->keys[i]), packageSearchPathFlag, KV_GetValueString(pFilesystem->keys[i], 2, nullptr));
		}
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
}

EqString CFileSystem::FindFilePath(const char* filename, int searchFlags /*= -1*/) const
{
	EqString basePath = m_basePath;
	if (basePath.Length() > 0) // FIXME: is that correct?
		basePath.Append(CORRECT_PATH_SEPARATOR);

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
		switch (tolower(*mode))
		{
		case 'r':
			modeFlags |= COSFile::READ;
			break;
		case 'w':
			modeFlags |= COSFile::WRITE;
			break;
		case 'a':
		case '+':
			modeFlags |= COSFile::APPEND;
			break;
		}
		++mode;
	}

	const bool isWrite = modeFlags & (COSFile::APPEND | COSFile::WRITE);

	EqString basePath = m_basePath;
	if (basePath.Length() > 0) // FIXME: is that correct?
		basePath.Append(CORRECT_PATH_SEPARATOR);

	IFilePtr fileHandle;
	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (isWrite && !writePath)
			return false;

		COSFile osFile;
		if (osFile.Open(filePath, modeFlags))
		{
			fileHandle = IFilePtr(CRefPtr_new(CFile, std::move(osFile)));
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
			if (!fsPacakage->GetInternalFileName(pkgFileName, filePath.ToCString() + basePath.Length()))
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

ubyte* CFileSystem::GetFileBuffer(const char* filename,long *filesize/* = 0*/, int searchFlags/* = -1*/)
{
	IFilePtr pFile = Open(filename, "rb", searchFlags);

    if (!pFile)
        return nullptr;

    const long length = pFile->GetSize();
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

long CFileSystem::GetFileSize(const char* filename, int searchFlags/* = -1*/)
{
    IFilePtr file = Open(filename,"rb",searchFlags);

    if (!file)
        return 0;

	long rSize = file->GetSize();
    return rSize;
}

uint32 CFileSystem::GetFileCRC32(const char* filename, int searchFlags)
{
    IFilePtr file = Open(filename,"rb",searchFlags);

    if (!file)
        return 0;

	uint32 checkSum = file->GetCRC32();
    return checkSum;
}


bool CFileSystem::FileCopy(const char* filename, const char* dest_file, bool overWrite, ESearchPath search)
{
	char buf[4096];

	if( FileExist(filename, search) && (overWrite || FileExist(dest_file, search) == false))
	{
		IFilePtr fp_write = Open(dest_file, "wb", search);
		IFilePtr fp_read = Open(filename, "rb", search);

		if (!fp_read || !fp_write)
			return false;

		int nread;
		while (nread = fp_read->Read(buf, sizeof(buf), 1), nread > 0)
		{
			char* out_ptr = buf;
			size_t nwritten;

			do {
				nwritten = fp_write->Write(out_ptr, nread, 1);

				if (nwritten >= 0)
				{
					nread -= nwritten;
					out_ptr += nwritten;
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
	EqString basePath = m_basePath;
	if (basePath.Length() > 0) // FIXME: is that correct?
		basePath.Append(CORRECT_PATH_SEPARATOR);

	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (access(filePath, F_OK) != -1)
			return true;

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageReader* fsPacakage = m_fsPackages[j];

			if (!(spFlags & fsPacakage->GetSearchPath()))
				continue;

			EqString pkgFileName;
			if (!fsPacakage->GetInternalFileName(pkgFileName, filePath.ToCString() + basePath.Length()))
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

	EqString basePath = m_basePath;
	if (basePath.Length() > 0)
		basePath.Append(CORRECT_PATH_SEPARATOR);

	switch (search)
	{
		case SP_DATA:
			CombinePath(searchPath, basePath.ToCString(), m_dataDir.ToCString());
			break;
		case SP_MOD:
			if(directoryId == -1) // default write path
				CombinePath(searchPath, basePath.ToCString(), GetCurrentGameDirectory());
			else
				CombinePath(searchPath, basePath.ToCString(), m_directories[directoryId]->path.ToCString());
			break;
		case SP_ROOT:
			searchPath = basePath.ToCString();
			break;
		default:
			searchPath = ".";
			break;
	}

	return searchPath;
}

static bool UTIL_IsAbsolutePath(const char* dirOrFileName)
{
	return (dirOrFileName[0] == CORRECT_PATH_SEPARATOR || isalpha(dirOrFileName[0]) && dirOrFileName[1] == ':');
}

EqString CFileSystem::GetAbsolutePath(ESearchPath search, const char* dirOrFileName) const
{
	EqString fullPath;

	const bool isAbsolutePath = (search == SP_ROOT && UTIL_IsAbsolutePath(dirOrFileName));

	if (!isAbsolutePath)
		CombinePath(fullPath, GetSearchPath(search).ToCString(), dirOrFileName);
	else
		fullPath = dirOrFileName;

	fullPath.Path_FixSlashes();

	return fullPath;
}

void CFileSystem::FileRemove(const char* filename, ESearchPath search ) const
{
	remove(GetAbsolutePath(search, filename));
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

	EqString basePath = m_basePath;
	if (!isAbsolutePath && basePath.Length() > 0) // FIXME: is that correct?
		basePath.Append(CORRECT_PATH_SEPARATOR);

	if (isAbsolutePath)
		flags = SP_ROOT;

	// First we checking mod directory
	if (flags & SP_MOD)
	{
		for (int i = 0; i < m_directories.numElem(); i++)
		{
			const SearchPathInfo& spInfo = *m_directories[i];

			EqString filePath;
			CombinePath(filePath, basePath.ToCString(), spInfo.path.ToCString(), fileName);
			filePath.Path_FixSlashes();

#ifdef PLAT_LINUX
			const int nameHash = FSStringId(filePath.ToCString());
			const auto it = spInfo.pathToFileMapping.find(nameHash);
			if (!it.atEnd())
			{
				// apply correct filepath
				filePath = *it;
			}
#endif

			if (func(filePath, SP_MOD, flags, spInfo.mainWritePath))
				return true;
		}
	}

	//Then we checking data directory
	if (flags & SP_DATA)
	{
		EqString filePath;
		CombinePath(filePath, basePath.ToCString(), m_dataDir.ToCString(), fileName);
		filePath.Path_FixSlashes();

		if (func(filePath, SP_DATA, flags, false))
			return true;
	}

	// And checking root.
	// not adding basepath to this
	if (flags & SP_ROOT)
	{
		EqString filePath;

		if(isAbsolutePath)
			filePath = fileName;
		else
			CombinePath(filePath, basePath.ToCString(), fileName);
		filePath.Path_FixSlashes();

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

static CBasePackageReader* GetPackageReader(const char* packageName)
{
	const EqString fileExt(_Es(packageName).Path_Extract_Ext());
	CBasePackageReader* reader = nullptr;

	// allow zip files to be loaded
	if (!fileExt.CompareCaseIns("zip"))
		reader = PPNew CZipFileReader();
	else
		reader = PPNew CDPKFileReader();

	return reader;
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

	CBasePackageReader* reader = GetPackageReader(packageName);
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
	EqString basePath = m_basePath;
	if (basePath.Length() > 0) // FIXME: is that correct?
		basePath.Append(CORRECT_PATH_SEPARATOR);

	CBasePackageReader* reader = GetPackageReader(packageName);

	auto walkFileFunc = [&](EqString filePath, ESearchPath searchPath, int spFlags, bool writePath) -> bool
	{
		if (reader->InitPackage(filePath, nullptr))
			return true;

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageReader* fsPacakage = m_fsPackages[j];

			if (!(spFlags & fsPacakage->GetSearchPath()))
				continue;

			EqString pkgFileName;
			if (!fsPacakage->GetInternalFileName(pkgFileName, filePath.ToCString() + basePath.Length()))
				continue;

			// trying to open package file inside package (embedded DPK)
			if (fsPacakage->OpenEmbeddedPackage(reader, pkgFileName))
				return true;
		}

		return false;
	};

	if (!WalkOverSearchPaths(searchFlags, packageName, walkFileFunc))
	{
		MsgError("Cannot open package '%s'\n", packageName);
		delete reader;
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

// sets fallback directory for mod
void CFileSystem::AddSearchPath(const char* pathId, const char* pszDir)
{
	for(int i = 0; i < m_directories.numElem(); i++)
	{
		if(m_directories[i]->id == pathId)
		{
			ErrorMsg("AddSearchPath Error: pathId %s already added", pathId);
			return;
		}
	}

	DevMsg(DEVMSG_FS, "Adding search patch '%s' at '%s'\n", pathId, pszDir);

	const bool isReadPriorityPath = strstr(pathId, "$MOD$") || strstr(pathId, "$LOCALIZE$");
	const bool isWriteablePath = strstr(pathId, "$WRITE$");

	SearchPathInfo* pathInfo = PPNew SearchPathInfo;
	pathInfo->id = pathId;
	pathInfo->path = pszDir;
	pathInfo->mainWritePath = !isReadPriorityPath || isWriteablePath;

	int spIdx = 0;
	if(isReadPriorityPath)
		m_directories.insert(pathInfo, 0);
	else
		spIdx = m_directories.append(pathInfo);

#ifdef PLAT_LINUX
	if(!pathInfo->mainWritePath)
	{
		// scan files and map
		SearchPathInfo& spInfo = *m_directories[spIdx];

		Array<EqString> openSet(PP_SL);
		openSet.reserve(5000);

		openSet.append(spInfo.path);

		while(openSet.numElem())
		{
			EqString path = openSet.popFront();

			DIR* dir = opendir(path);
			if(!dir)
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
					spInfo.pathToFileMapping.insert(nameHash, entryName);
				}
			} while (true);
		}

		if(spInfo.pathToFileMapping.size())
			DevMsg(DEVMSG_FS, "Mapped %d files for '%s'\n", spInfo.pathToFileMapping.size(), pszDir);
	}
#endif
}

void CFileSystem::RemoveSearchPath(const char* pathId)
{
	for(int i = 0; i < m_directories.numElem(); i++)
	{
		if(m_directories[i]->id == pathId)
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
	for (int i = 0; i < m_directories.numElem(); i++)
	{
		if (m_directories[i]->mainWritePath)
			return m_directories[i]->path;
	}

    return m_dataDir.GetData();
}

// Returns current engine data path
const char* CFileSystem::GetCurrentDataDirectory() const
{
    return m_dataDir.GetData();
}

// opens directory for search props
const char* CFileSystem::FindFirst(const char* wildcard, DKFINDDATA** findData, int searchPath)
{
	ASSERT(findData != nullptr);

	if(findData == nullptr)
		return nullptr;

	DKFINDDATA* newFind = PPNew DKFINDDATA;

	newFind->searchPath = (ESearchPath)searchPath;
	newFind->wildcard = wildcard;
	newFind->wildcard.Path_FixSlashes();

	if (newFind->searchPath != SP_MOD)
	{
		EqString fsBaseDir = GetSearchPath((ESearchPath)searchPath, -1);

		EqString searchWildcard;
		CombinePath(searchWildcard, fsBaseDir.ToCString(), newFind->wildcard.ToCString());

		if (newFind->osFind.Init(searchWildcard))
		{
			m_findDatas.append(newFind);
			*findData = newFind;
			return newFind->osFind.GetCurrentPath();
		}

		delete newFind;
		return nullptr;
	}

	// try initialize in different addon directories

	newFind->searchPathId = (searchPath == SP_MOD) ? 0 : -1;

	EqString fsBaseDir;
	EqString searchWildcard;
	while (newFind->searchPathId < m_directories.numElem())
	{
		fsBaseDir = GetSearchPath((ESearchPath)searchPath, newFind->searchPathId);
		CombinePath(searchWildcard, fsBaseDir.ToCString(), newFind->wildcard.ToCString());

		if (newFind->osFind.Init(searchWildcard))
		{
			m_findDatas.append(newFind);
			*findData = newFind;
			return newFind->osFind.GetCurrentPath();
		}

		newFind->searchPathId++;
	}

	delete newFind;
	return nullptr;
}

const char* CFileSystem::FindNext(DKFINDDATA* findData) const
{
	if(!findData)
		return nullptr;

	if (findData->osFind.GetNext())
		return findData->osFind.GetCurrentPath();

	if(findData->searchPath != SP_MOD)
		return nullptr;

	// try reinitialize
	findData->searchPathId++;
	while(findData->searchPathId < m_directories.numElem())
	{
		EqString fsBaseDir;
		EqString searchWildcard;
			
		fsBaseDir = GetSearchPath(findData->searchPath, findData->searchPathId);
		CombinePath(searchWildcard, fsBaseDir.ToCString(), findData->wildcard.ToCString());

		if (findData->osFind.Init(searchWildcard.ToCString()))
		{
			return findData->osFind.GetCurrentPath();
		}

		findData->searchPathId++;
	}

	return nullptr;
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

// loads module
DKMODULE* CFileSystem::OpenModule(const char* mod_name, EqString* outError)
{
	EqString moduleFileName = mod_name;
	EqString modExt = moduleFileName.Path_Extract_Ext();

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
