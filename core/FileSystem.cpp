//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/ILocalize.h"
#include "utils/KeyValues.h"
#include "FileSystem.h"

#include "DPKFileReader.h"
#include "ZipFileReader.h"

#ifdef _WIN32 // Not in linux

#include <direct.h>	// mkdir()
#include <io.h>		// _access()

#define access		_access
#define F_OK		0       // Test for existence.

#else

#include <unistd.h> // rmdir(), access()
#include <stdarg.h> // va_*
#include <dlfcn.h>
#include <dirent.h> // opendir, readdir

#endif

using namespace Threading;
static CEqMutex	s_FSMutex;

EXPORTED_INTERFACE(IFileSystem, CFileSystem);

static bool UTIL_IsAbsolutePath(const char* dirOrFileName)
{
	return (dirOrFileName[0] == CORRECT_PATH_SEPARATOR || isalpha(dirOrFileName[0]) && dirOrFileName[1] == ':');
}

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
	int					searchPathId;
	EqString			wildcard;

#if defined(PLAT_WIN)
	WIN32_FIND_DATAA	wfd;
	HANDLE				fileHandle{ INVALID_HANDLE_VALUE };
#elif defined(PLAT_POSIX)
	EqString			dirPath;
	DIR*				dir{ nullptr };
	struct dirent*		entry{ nullptr };
	bool				isEntryDir{ false };
#endif

	~DKFINDDATA()
	{
		Release();
	}

	void Release()
	{
#if defined(PLAT_WIN)
		if(fileHandle != INVALID_HANDLE_VALUE)
			FindClose(fileHandle);
		fileHandle = INVALID_HANDLE_VALUE;
#elif defined(PLAT_POSIX)
		closedir(dir);
		dir = nullptr;
		entry = nullptr;
#endif
	}

	bool Init(const EqString& searchWildcard)
	{
		Release();

#if defined(PLAT_WIN)
		fileHandle = FindFirstFileA(searchWildcard, &wfd);
		if (fileHandle != INVALID_HANDLE_VALUE)
			return true;
#elif defined(PLAT_POSIX)
		dirPath = searchWildcard.Path_Extract_Path();
		dir = opendir(dirPath);
		if (dir)
			return GetNext();
#endif
		return false;
	}

	bool GetNext()
	{
#if defined(PLAT_WIN)

		return FindNextFileA(fileHandle, &wfd);

#elif defined(PLAT_POSIX)
		do
		{
			entry = readdir(dir);

			if (!entry)
				break;

			if (*entry->d_name == 0)
				continue;

			int wildcardFileStart = wildcard.Find("*");
			if (wildcardFileStart != -1)
			{
				const char* wildcardFile = wildcard.ToCString() + wildcardFileStart + 1;
				if (*wildcardFile == 0)
					break;

				const char* found = xstristr(entry->d_name, wildcardFile);
				if (found && strlen(found) == strlen(wildcardFile))
					break;
			}
			else
				break;
		} while (true);

		isEntryDir = false;

		if (entry)
		{
			struct stat st;
			if (stat(EqString::Format("%s", dirPath.TrimChar(CORRECT_PATH_SEPARATOR).ToCString(), entry->d_name), &st) == 0)
				isEntryDir = (st.st_mode & S_IFDIR);
		}

		return entry;
#endif
	}

	const char* GetCurrentPath() const 
	{
#if defined(PLAT_WIN)
		return wfd.cFileName;
#elif defined(PLAT_POSIX)
		return entry->d_name;
#endif
	}
};

//--------------------------------------------------

static SearchPath_e GetSearchPathByName(const char* str)
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
	g_eqCore->RegisterInterface(FILESYSTEM_INTERFACE_VERSION, this);
}

CFileSystem::~CFileSystem()
{
	g_eqCore->UnregisterInterface(FILESYSTEM_INTERFACE_VERSION);
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
			SearchPath_e packageSearchPathFlag = GetSearchPathByName(KV_GetValueString(pFilesystem->keys[i], 1, "SP_MOD"));

			AddPackage(KV_GetValueString(pFilesystem->keys[i]), packageSearchPathFlag, KV_GetValueString(pFilesystem->keys[i], 2, nullptr));
		}
	}

	g_localizer->Init();

	m_isInit = true;

    return true;
}

void CFileSystem::Shutdown()
{
	m_isInit = false;

	for(int i = 0; i < m_modules.numElem(); i++)
	{
		FreeModule(m_modules[i]);
		i--;
	}

	/*
	for(int i = 0; i < m_pOpenFiles.numElem(); i++)
	{
		Close( m_pOpenFiles[i] );
		i--;
	}
	*/

	for(int i = 0; i < m_fsPackages.numElem(); i++)
		delete m_fsPackages[i];

	m_fsPackages.clear();
	m_directories.clear();

	g_localizer->Shutdown();
}

void CFileSystem::SetBasePath(const char* path) 
{ 
	m_basePath = path; 
}

IFile* CFileSystem::Open(const char* filename, const char* mode, int searchFlags/* = -1*/ )
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

	IFile* fileHandle = nullptr;
	auto walkFileFunc = [&](EqString filePath, SearchPath_e searchPath, int spFlags, bool writePath) -> bool
	{
		if (isWrite && !writePath)
			return false;

		COSFile osFile;
		if (osFile.Open(filePath, modeFlags))
		{
			fileHandle = PPNew CFile(std::move(osFile));
			return true;
		}

		if (isWrite)
			return false;

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageReader* pPackageReader = m_fsPackages[j];

			if (!(spFlags & pPackageReader->GetSearchPath()))
				continue;

			// package readers do not support base path, get rid of it
			fileHandle = pPackageReader->Open(filePath.ToCString() + basePath.Length(), modeFlags);
			if (fileHandle)
				return true;
		}

		return false;
	};

	WalkOverSearchPaths(searchFlags, filename, walkFileFunc);

	if (fileHandle)
	{
		CScopedMutex m(s_FSMutex);
		m_openFiles.append(fileHandle);
	}

	return fileHandle;
}

void CFileSystem::Close( IFile* fp )
{
	if(!fp)
		return;

	{
		CScopedMutex m(s_FSMutex);
		if (!m_openFiles.fastRemove(fp))
			return;
	}

	VirtStreamType_e vsType = fp->GetType();

	if(vsType == VS_TYPE_FILE)
	{
		delete fp;
	}
	else if(vsType == VS_TYPE_FILE_PACKAGE) // DPK or ZIP
	{
		CBasePackageFileStream* pFile = (CBasePackageFileStream*)fp;
		pFile->GetHostPackage()->Close(pFile);
	}
}

char* CFileSystem::GetFileBuffer(const char* filename,long *filesize/* = 0*/, int searchFlags/* = -1*/)
{
	if(!FileExist(filename, searchFlags))
		return nullptr;

	IFile* pFile = Open(filename, "rb", searchFlags);

    if (!pFile)
        return nullptr;

    long length = pFile->GetSize();

    char *buffer = (char*)PPAlloc(length+1);

    if (!buffer)
    {
        fprintf(stderr,"Memory error!");
        Close(pFile);
        return nullptr;
    }

	memset(buffer, 0, length+1);

	pFile->Read(buffer, 1, length);
    buffer[length] = 0;

    Close(pFile);

    if (filesize)
        *filesize = length;

    return buffer;
}

long CFileSystem::GetFileSize(const char* filename, int searchFlags/* = -1*/)
{
    IFile* pFile = Open(filename,"rb",searchFlags);

    if (!pFile)
        return 0;

	long rSize = pFile->GetSize();

    Close(pFile);

    return rSize;
}

uint32 CFileSystem::GetFileCRC32(const char* filename, int searchFlags)
{
    IFile* pFile = Open(filename,"rb",searchFlags);

    if (!pFile)
        return 0;

	uint32 checkSum = pFile->GetCRC32();

    Close(pFile);

    return checkSum;
}


bool CFileSystem::FileCopy(const char* filename, const char* dest_file, bool overWrite, SearchPath_e search)
{
	char buf[4096];

	if( FileExist(filename, search) && (overWrite || FileExist(dest_file, search) == false))
	{
		IFile* fp_write = Open(dest_file, "wb", search);
		IFile* fp_read = Open(filename, "rb", search);

		if (!fp_read || !fp_write)
		{
			Close(fp_read);
			Close(fp_write);
			return false;
		}

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

		Close(fp_read);
		Close(fp_write);
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

	auto walkFileFunc = [&](EqString filePath, SearchPath_e searchPath, int spFlags, bool writePath) -> bool
	{
		if (access(filePath, F_OK) != -1)
			return true;

		// If failed to load directly, load it from package, in backward order
		for (int j = m_fsPackages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageReader* pPackageReader = m_fsPackages[j];

			if (!(spFlags & pPackageReader->GetSearchPath()))
				continue;

			// package readers do not support base path, get rid of it
			if (pPackageReader->FileExists(filePath.ToCString() + basePath.Length()))
				return true;
		}

		return false;
	};

	return WalkOverSearchPaths(searchFlags, filename, walkFileFunc);
}

EqString CFileSystem::GetSearchPath(SearchPath_e search, int directoryId) const
{
	EqString searchPath;

	EqString basePath = m_basePath;
	if (basePath.Length() > 0)
		basePath.Append(CORRECT_PATH_SEPARATOR);

	switch (search)
	{
		case SP_DATA:
			CombinePath(searchPath, 2, basePath.ToCString(), m_dataDir.ToCString());
			break;
		case SP_MOD:
			if(directoryId == -1) // default write path
				CombinePath(searchPath, 2, basePath.ToCString(), GetCurrentGameDirectory());
			else
				CombinePath(searchPath, 2, basePath.ToCString(), m_directories[directoryId].path.ToCString());
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

EqString CFileSystem::GetAbsolutePath(SearchPath_e search, const char* dirOrFileName) const
{
	EqString fullPath;

	const bool isAbsolutePath = (search == SP_ROOT && UTIL_IsAbsolutePath(dirOrFileName));

	if (!isAbsolutePath)
		CombinePath(fullPath, 2, GetSearchPath(search).ToCString(), dirOrFileName);
	else
		fullPath = dirOrFileName;

	fullPath.Path_FixSlashes();

	return fullPath;
}

void CFileSystem::FileRemove(const char* filename, SearchPath_e search ) const
{
	remove(GetAbsolutePath(search, filename));
}

//Directory operations
void CFileSystem::MakeDir(const char* dirname, SearchPath_e search ) const
{
	mkdirRecursive(GetAbsolutePath(search, dirname), true);
}

void CFileSystem::RemoveDir(const char* dirname, SearchPath_e search ) const
{
    rmdir(GetAbsolutePath(search, dirname));
}

void CFileSystem::Rename(const char* oldNameOrPath, const char* newNameOrPath, SearchPath_e search) const
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
			EqString filePath;
			CombinePath(filePath, 3, basePath.ToCString(), m_directories[i].path.ToCString(), fileName);
			filePath.Path_FixSlashes();

			if (func(filePath, SP_MOD, flags, m_directories[i].mainWritePath))
				return true;
		}
	}

	//Then we checking data directory
	if (flags & SP_DATA)
	{
		EqString filePath;
		CombinePath(filePath, 3, basePath.ToCString(), m_dataDir.ToCString(), fileName);
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
			CombinePath(filePath, 2, basePath.ToCString(), fileName);
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

bool CFileSystem::AddPackage(const char* packageName, SearchPath_e type, const char* mountPath /*= nullptr*/)
{
	for(int i = 0; i < m_fsPackages.numElem();i++)
	{
		if(!stricmp(m_fsPackages[i]->GetPackageFilename(), packageName))
		{
			ASSERT_FAIL("Package %s was already added to file system", packageName);
			return false;
		}
	}

	CBasePackageReader* reader = GetPackageReader(packageName);

	if (!reader->InitPackage(packageName, mountPath))
	{
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
	for (int i = 0; i < m_fsPackages.numElem(); i++)
	{
		CBasePackageReader* reader = m_fsPackages[i];

		if (!stricmp(reader->GetPackageFilename(), packageName))
		{
			m_fsPackages.fastRemoveIndex(i);
			delete reader;
			return;
		}
	}
}

// opens package for further reading. Does not add package as FS layer
IFilePackageReader* CFileSystem::OpenPackage(const char* packageName)
{
	CBasePackageReader* reader = GetPackageReader(packageName);

	if (!reader->InitPackage(packageName, nullptr))
	{
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
		if(m_directories[i].id == pathId)
		{
			ErrorMsg("AddSearchPath Error: pathId %s already added", pathId);
			return;
		}
	}

	DevMsg(DEVMSG_FS, "Adding search patch '%s' at '%s'\n", pathId, pszDir);

	const bool isReadPriorityPath = strstr(pathId, "$MOD$") || strstr(pathId, "$LOCALIZE$");
	const bool isWriteablePath = strstr(pathId, "$WRITE$");

	SearchPath_t pathInfo;
	pathInfo.id = pathId;
	pathInfo.path = pszDir;
	pathInfo.mainWritePath = !isReadPriorityPath || isWriteablePath;

	if(isReadPriorityPath)
		m_directories.insert(pathInfo, 0);
	else
		m_directories.append(std::move(pathInfo));
}

void CFileSystem::RemoveSearchPath(const char* pathId)
{
	for(int i = 0; i < m_directories.numElem(); i++)
	{
		if(m_directories[i].id == pathId)
		{
			DevMsg(DEVMSG_FS, "Removing search patch '%s'\n", pathId);
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
		if (m_directories[i].mainWritePath)
			return m_directories[i].path;
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
	*findData = newFind;

	newFind->searchPathId = -1;
	newFind->wildcard = wildcard;
	newFind->wildcard.Path_FixSlashes();

	EqString fsBaseDir = m_basePath;

	if(searchPath == SP_DATA)
	{
		CombinePath(fsBaseDir, 2, m_basePath.ToCString(), m_dataDir.ToCString());
	}
	else if(searchPath == SP_MOD)
	{
		newFind->searchPathId = 0;
		CombinePath(fsBaseDir, 2, m_basePath.ToCString(), m_directories[0].path.ToCString());
	}

	EqString searchWildcard;
	CombinePath(searchWildcard, 2, fsBaseDir.ToCString(), newFind->wildcard.ToCString());

	m_findDatas.append(newFind);

	if (newFind->Init(searchWildcard))
		return newFind->GetCurrentPath();

	if(newFind->searchPathId != -1)
	{
		newFind->searchPathId++;

		// try reinitialize
		while(newFind->searchPathId < m_directories.numElem())
		{
			CombinePath(fsBaseDir, 2, m_basePath.ToCString(), m_directories[newFind->searchPathId].path.ToCString());
			CombinePath(searchWildcard, 2, fsBaseDir.ToCString(), newFind->wildcard.ToCString());

			if (newFind->Init(searchWildcard))
				return newFind->GetCurrentPath();

			newFind->searchPathId++;
		}
	}

	// delete if no luck
	delete newFind;
	m_findDatas.remove(newFind);

	return nullptr;
}

const char* CFileSystem::FindNext(DKFINDDATA* findData) const
{
	if(!findData)
		return nullptr;

	if (findData->GetNext())
		return findData->GetCurrentPath();

	if(findData->searchPathId == -1)
		return nullptr;

	findData->searchPathId++;

	// try reinitialize
	while(findData->searchPathId < m_directories.numElem())
	{
		EqString fsBaseDir;
		EqString searchWildcard;
			
		CombinePath(fsBaseDir, 2, m_basePath.ToCString(), m_directories[findData->searchPathId].path.ToCString());		
		CombinePath(searchWildcard, 2, fsBaseDir.ToCString(), findData->wildcard.ToCString());

		if (findData->Init(searchWildcard.ToCString()))
			return findData->GetCurrentPath();

		findData->searchPathId++;
	}

	return nullptr;
}

void CFileSystem::FindClose(DKFINDDATA* findData)
{
	if(!findData)
		return;

	if(m_findDatas.remove(findData))
		delete findData;
}

bool CFileSystem::FindIsDirectory(DKFINDDATA* findData) const
{
	if(!findData)
		return false;

#ifdef _WIN32
	return (findData->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	return findData->isEntryDir;
#endif // _WIN32
}

// loads module
DKMODULE* CFileSystem::LoadModule(const char* mod_name)
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

	const char* err = dlerror();
	int lastErr = 0;
#endif // _WIN32 && MEMDLL

	MsgInfo("Loading module '%s'\n", moduleFileName.ToCString());

	if( !mod )
	{
        MsgInfo("Library '%s' loading error '%s', 0x%p\n", moduleFileName.ToCString(), err, lastErr);

		ErrorMsg("CFileSystem::LoadModule Error: Failed to load %s\n - %s!", moduleFileName.ToCString(), err);
		return nullptr;
	}

	DKMODULE* pModule = PPNew DKMODULE;
	strcpy( pModule->name, moduleFileName );
	pModule->module = (HMODULE)mod;

	return pModule;
}

// frees module
void CFileSystem::FreeModule( DKMODULE* pModule )
{
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
