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
#include <io.h> // _access()

#define access		_access
#define F_OK		0       // Test for existence.

#else

#include <unistd.h> // rmdir(), access()
#include <stdarg.h> // va_*
#include <dlfcn.h>

#ifdef PLAT_ANDROID
#include "android_libc/glob.h"		// glob(), globfree()
#else
#include <glob.h>					// glob(), globfree()
#endif

#endif

using namespace Threading;

EXPORTED_INTERFACE(IFileSystem, CFileSystem);

static bool UTIL_IsAbsolutePath(const char* dirOrFileName)
{
	return (dirOrFileName[0] == CORRECT_PATH_SEPARATOR || isalpha(dirOrFileName[0]) && dirOrFileName[1] == ':');
}

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

int	CFile::Seek( long pos, VirtStreamSeek_e seekType )
{
	return fseek( m_pFilePtr, pos, seekType );
}

long CFile::Tell() const
{
	return ftell( m_pFilePtr );
}

size_t CFile::Read( void *dest, size_t count, size_t size)
{
	return fread( dest, size, count, m_pFilePtr );
}

size_t CFile::Write( const void *src, size_t count, size_t size)
{
	return fwrite( src, size, count, m_pFilePtr);
}

int	CFile::Error()
{
	return ferror( m_pFilePtr );
}

int	CFile::Flush()
{
	return fflush( m_pFilePtr );
}

char* CFile::Gets( char *dest, int destSize )
{
	return fgets(dest, destSize, m_pFilePtr );
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
	int searchPathId;
	EqString wildcard;

#ifdef _WIN32
	WIN32_FIND_DATAA	wfd;
	HANDLE				fileHandle;
#else
	glob_t				gl;
	int					index;
	int					pathlen;
#endif // _WIN32
};

extern bool g_bPrintLeaksOnShutdown;

CFileSystem::CFileSystem() :
	m_isInit(false), m_editorMode(false)
{
	g_eqCore->RegisterInterface(FILESYSTEM_INTERFACE_VERSION, this);
}

CFileSystem::~CFileSystem()
{
	g_eqCore->UnregisterInterface(FILESYSTEM_INTERFACE_VERSION);
}

SearchPath_e GetSearchPathByName(const char* str)
{
	if (!stricmp(str, "SP_MOD"))
		return SP_MOD;
	else if (!stricmp(str, "SP_DATA"))
		return SP_DATA;

	return SP_ROOT;
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

	m_basePath = KV_GetValueString(pFilesystem->FindSection("BasePath"), 0, m_basePath.ToCString());

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

	for(int i = 0; i < m_packages.numElem(); i++)
		delete m_packages[i];

	m_packages.clear();
	m_directories.clear();

	g_localizer->Shutdown();
}

void CFileSystem::SetBasePath(const char* path) 
{ 
	m_basePath = path; 
}

IFile* CFileSystem::Open(const char* filename,const char* options, int searchFlags/* = -1*/ )
{
    return GetFileHandle(filename,options,searchFlags);
}

void CFileSystem::Close( IFile* fp )
{
	if(!fp)
		return;

	{
		CScopedMutex m(m_FSMutex);
		if (!m_openFiles.fastRemove(fp))
			return;
	}

	VirtStreamType_e vsType = fp->GetType();

	if(vsType == VS_TYPE_FILE)
	{
		CFile* pFile = (CFile*)fp;
		fclose(pFile->m_pFilePtr);
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

	IFile* pFile = Open(filename,"rb",searchFlags);

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
    int flags = searchFlags;
    if (flags == -1)
        flags |= SP_MOD | SP_DATA | SP_ROOT;

	const bool isAbsolutePath = UTIL_IsAbsolutePath(filename);

	if (isAbsolutePath)
	{
		flags = SP_ROOT;
	}

	EqString tmp_path;

	EqString basePath = m_basePath;
	if(basePath.Length() > 0)
		basePath.Append( CORRECT_PATH_SEPARATOR );

    //First we checking mod directory
    if (flags & SP_MOD)
    {
		for(int i = 0; i < m_directories.numElem(); i++)
		{
			CombinePath(tmp_path, 3, basePath.ToCString(), m_directories[i].path.ToCString(), filename);
			tmp_path.Path_FixSlashes();

			if (access(tmp_path, F_OK ) != -1)
				return true;

			// base path is not used when dealing with package files
			CombinePath(tmp_path, 2, m_directories[i].path.ToCString(), filename);
			tmp_path.Path_FixSlashes();

			// If failed to load directly, load it from package, in backward order
			for (int j = m_packages.numElem() - 1; j >= 0; j--)
			{
				CBasePackageFileReader* pPackageReader = m_packages[j];

				if ((flags & pPackageReader->GetSearchPath()) && pPackageReader->FileExists(tmp_path))
					return true;
			}
		}
    }

    //Then we checking data directory
    if (flags & SP_DATA)
    {
		CombinePath(tmp_path, 3, basePath.ToCString(), m_dataDir.ToCString(), filename);
		tmp_path.Path_FixSlashes();

		if (access(tmp_path, F_OK ) != -1)
			return true;

		// base path is not used when dealing with package files
		CombinePath(tmp_path, 2, m_dataDir.ToCString(), filename);
		tmp_path.Path_FixSlashes();

		// If failed to load directly, load it from package, in backward order
		for (int j = m_packages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageFileReader* pPackageReader = m_packages[j];

			if ((flags & pPackageReader->GetSearchPath()) && pPackageReader->FileExists(tmp_path))
				return true;
		}
    }

    // And checking root.
    // not adding basepath to this
    if (flags & SP_ROOT)
    {
		// check
		if(filename[0] != CORRECT_PATH_SEPARATOR && !(isalpha(filename[0]) && filename[1] == ':'))
			CombinePath(tmp_path, 2, basePath.ToCString(), filename);
		else
			tmp_path = filename;

		tmp_path.Path_FixSlashes();

		if (access(tmp_path, F_OK ) != -1)
			return true;

		if (!isAbsolutePath)
		{
			// base path is not used when dealing with package files
			tmp_path = filename;
			tmp_path.Path_FixSlashes();

			// If failed to load directly, load it from package, in backward order
			for (int j = m_packages.numElem() - 1; j >= 0; j--)
			{
				CBasePackageFileReader* pPackageReader = m_packages[j];

				if ((flags & pPackageReader->GetSearchPath()) && pPackageReader->FileExists(tmp_path))
					return true;
			}
		}
    }

	return false;
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

	bool isAbsolutePath = (search == SP_ROOT && UTIL_IsAbsolutePath(dirOrFileName));

	if (!isAbsolutePath)
		CombinePath(fullPath, 2, GetSearchPath(search).ToCString(), dirOrFileName);
	else
		fullPath = dirOrFileName;

	return fullPath;
}

void CFileSystem::FileRemove(const char* filename, SearchPath_e search ) const
{
	EqString fullPath = GetAbsolutePath(search, filename);
	fullPath.Path_FixSlashes();

	remove(fullPath.ToCString());
}

bool mkdirRecursive(const char* path, bool includeDotPath)
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

//Directory operations
void CFileSystem::MakeDir(const char* dirname, SearchPath_e search ) const
{
	EqString fullPath = GetAbsolutePath(search, dirname);
	fullPath.Path_FixSlashes();

	mkdirRecursive(fullPath.ToCString(), true);
}

void CFileSystem::RemoveDir(const char* dirname, SearchPath_e search ) const
{
	EqString fullPath = GetAbsolutePath(search, dirname);
	fullPath.Path_FixSlashes();

	fullPath.Path_FixSlashes();

    rmdir(fullPath.GetData());
}

void CFileSystem::Rename(const char* oldNameOrPath, const char* newNameOrPath, SearchPath_e search) const
{
	EqString oldFullPath = GetAbsolutePath(search, oldNameOrPath);
	oldFullPath.Path_FixSlashes();

	EqString newFullPath = GetAbsolutePath(search, newNameOrPath);
	newFullPath.Path_FixSlashes();

	rename(oldFullPath.ToCString(), newFullPath.ToCString());
}

//Filesystem's check and open file
IFile* CFileSystem::GetFileHandle(const char* filename, const char* options, int searchFlags )
{
    int flags = searchFlags;
    if (flags == -1)
        flags |= SP_MOD | SP_DATA | SP_ROOT;

	const bool isAbsolutePath = UTIL_IsAbsolutePath(filename);

	if (isAbsolutePath)
	{
		flags = SP_ROOT;
	}

	const bool isWrite = (strchr(options, 'w') || strchr(options, 'a') || strchr(options, '+'));

	EqString tmp_path;

	EqString basePath = m_basePath;
	if(basePath.Length() > 0)
		basePath.Append( CORRECT_PATH_SEPARATOR );

    //First we checking mod directory
    if (flags & SP_MOD)
    {
		for(int i = 0; i < m_directories.numElem(); i++)
		{
			// don't create files in other write paths
			if (isWrite && !m_directories[i].mainWritePath)
				continue;

			CombinePath(tmp_path, 3, basePath.ToCString(), m_directories[i].path.ToCString(), filename);
			tmp_path.Path_FixSlashes();

			FILE *tmpFile = fopen(tmp_path,options);
			if (tmpFile)
			{
				CFile* pFileHandle = PPNew CFile(tmpFile);

				{
					CScopedMutex m(m_FSMutex);
					m_openFiles.append(pFileHandle);
				}

				return pFileHandle;
			}

			if (!isWrite)
			{
				// base path is not used when dealing with package files
				CombinePath(tmp_path, 2, m_directories[i].path.ToCString(), filename);
				tmp_path.Path_FixSlashes();

				// If failed to load directly, load it from package, in backward order
				for (int j = m_packages.numElem() - 1; j >= 0; j--)
				{
					CBasePackageFileReader* pPackageReader = m_packages[j];

					if (!(flags & pPackageReader->GetSearchPath()))
						continue;

					IFile* pPackedFile = pPackageReader->Open(tmp_path, options);

					if (pPackedFile)
					{
						{
							CScopedMutex m(m_FSMutex);
							m_openFiles.append(pPackedFile);
						}

						return pPackedFile;
					}
				}
			}
		}
    }

    //Then we checking data directory
    if (flags & SP_DATA)
    {
		CombinePath(tmp_path, 3, basePath.ToCString(), m_dataDir.ToCString(), filename);
		tmp_path.Path_FixSlashes();

        FILE *tmpFile = fopen(tmp_path,options);
        if (tmpFile)
        {
			CScopedMutex m(m_FSMutex);
			CFile* pFileHandle = PPNew CFile(tmpFile);
			m_openFiles.append(pFileHandle);
			return pFileHandle;
        }

		if (!isWrite)
		{
			// base path is not used when dealing with package files
			CombinePath(tmp_path, 2, m_dataDir.ToCString(), filename);
			tmp_path.Path_FixSlashes();

			// If failed to load directly, load it from package, in backward order
			for (int j = m_packages.numElem() - 1; j >= 0; j--)
			{
				CBasePackageFileReader* pPackageReader = m_packages[j];

				if (!(flags & pPackageReader->GetSearchPath()))
					continue;

				IFile* pPackedFile = pPackageReader->Open(tmp_path, options);

				if (pPackedFile)
				{
					{
						CScopedMutex m(m_FSMutex);
						m_openFiles.append(pPackedFile);
					}

					return pPackedFile;
				}
			}
		}
    }

    // And checking root.
    // not adding basepath to this
    if (flags & SP_ROOT)
    {
		if (filename[0] != CORRECT_PATH_SEPARATOR && !(isalpha(filename[0]) && filename[1] == ':'))
			CombinePath(tmp_path, 2, basePath.ToCString(), filename);
		else
			tmp_path = filename;

		tmp_path.Path_FixSlashes();

        FILE *tmpFile = fopen(tmp_path,options);
		if (tmpFile)
		{
			CScopedMutex m(m_FSMutex);
			CFile* pFileHandle = PPNew CFile(tmpFile);
			m_openFiles.append(pFileHandle);

			return pFileHandle;
		}

		if (!isWrite && !isAbsolutePath)
		{
			// base path is not used when dealing with package files
			tmp_path = filename;
			tmp_path.Path_FixSlashes();

			// If failed to load directly, load it from package, in backward order
			for (int j = m_packages.numElem() - 1; j >= 0; j--)
			{
				CBasePackageFileReader* pPackageReader = m_packages[j];

				if (!(flags & pPackageReader->GetSearchPath()))
					continue;

				IFile* pPackedFile = pPackageReader->Open(tmp_path, options);

				if (pPackedFile)
				{
					{
						CScopedMutex m(m_FSMutex);
						m_openFiles.append(pPackedFile);
					}

					return pPackedFile;
				}
			}
		}
    }

    return nullptr;
}

bool CFileSystem::SetAccessKey(const char* accessKey)
{
	m_accessKey = accessKey;
	return m_accessKey.Length() > 0;
}

bool CFileSystem::AddPackage(const char* packageName, SearchPath_e type, const char* mountPath /*= nullptr*/)
{
	for(int i = 0; i < m_packages.numElem();i++)
	{
		if(!stricmp(m_packages[i]->GetPackageFilename(), packageName))
			return false;
	}

	EqString fileExt(_Es(packageName).Path_Extract_Ext());

	CBasePackageFileReader* pPackageReader = nullptr;

	// allow zip files to be loaded
	if (!fileExt.CompareCaseIns("zip") || !fileExt.CompareCaseIns("obb"))
		pPackageReader = PPNew CZipFileReader(m_FSMutex);
	else
		pPackageReader = PPNew CDPKFileReader(m_FSMutex);

    if (pPackageReader->InitPackage( packageName, mountPath))
    {
		if(mountPath)
			DevMsg(DEVMSG_FS, "Adding package file '%s' force mount at '%s'\n", packageName, mountPath);
		else
			DevMsg(DEVMSG_FS, "Adding package file '%s'\n", packageName);

        pPackageReader->SetSearchPath(type);
		pPackageReader->SetKey(m_accessKey.ToCString());

        m_packages.append(pPackageReader);
        return true;
    }
    else
    {
        delete pPackageReader;

        return false;
    }

    return false;
}

void CFileSystem::RemovePackage(const char* packageName)
{
	for (int i = 0; i < m_packages.numElem(); i++)
	{
		CBasePackageFileReader* pPackageReader = m_packages[i];

		if (!stricmp(pPackageReader->GetPackageFilename(), packageName))
		{
			m_packages.fastRemoveIndex(i);
			delete pPackageReader;
			i--;
		}
	}
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

	bool isReadPriorityPath = strstr(pathId, "$MOD$") || strstr(pathId, "$LOCALIZE$");
	bool isWriteablePath = strstr(pathId, "$WRITE$");

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
			return m_directories[i].path.ToCString();
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

#ifndef _WIN32
	newFind->wildcard.ReplaceSubstr("*.*", "*");
#endif

	EqString fsBaseDir = m_basePath.ToCString();

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

#ifdef _WIN32
	newFind->fileHandle = ::FindFirstFileA(searchWildcard.ToCString(), &newFind->wfd);

	if(newFind->fileHandle != INVALID_HANDLE_VALUE)
		return newFind->wfd.cFileName;

#else // POSIX
	newFind->index = -1;

	if (glob(searchWildcard.ToCString(), 0, nullptr, &newFind->gl) == 0 && newFind->gl.gl_pathc > 0)
	{
		newFind->pathlen = searchWildcard.Path_Extract_Path().Length();
		newFind->index = 0;
		return newFind->gl.gl_pathv[newFind->index] + newFind->pathlen;
	}
#endif // _WIN32

	if(newFind->searchPathId != -1)
	{
		newFind->searchPathId++;

		// try reinitialize
		while(newFind->searchPathId < m_directories.numElem())
		{
			CombinePath(fsBaseDir, 2, m_basePath.ToCString(), m_directories[newFind->searchPathId].path.ToCString());
			CombinePath(searchWildcard, 2, fsBaseDir.ToCString(), newFind->wildcard.ToCString());

#ifdef _WIN32
			newFind->fileHandle = ::FindFirstFileA(searchWildcard.ToCString(), &newFind->wfd);

			if(newFind->fileHandle != INVALID_HANDLE_VALUE)
				return newFind->wfd.cFileName;
#else // POSIX
			if (glob(searchWildcard.ToCString(), 0, nullptr, &newFind->gl) == 0)
			{
				newFind->pathlen = searchWildcard.Path_Extract_Path().Length();
				newFind->index = 0;
				return newFind->gl.gl_pathv[newFind->index] + newFind->pathlen;
			}
#endif // _WIN32

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

#ifdef _WIN32
	if(!::FindNextFileA(findData->fileHandle, &findData->wfd))
#else
	if (findData->index < 0)
		return nullptr;

	if (findData->index >= findData->gl.gl_pathc)
#endif // _WIN32
	{
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

#ifdef _WIN32
			// close existing find
			::FindClose(findData->fileHandle);

			// tre init new
			findData->fileHandle = ::FindFirstFileA(searchWildcard.ToCString(), &findData->wfd);

			if(findData->fileHandle != INVALID_HANDLE_VALUE)
				return findData->wfd.cFileName;
#else // POSIX
			globfree(&findData->gl);
			findData->index = -1;

			if (glob(searchWildcard.ToCString(), 0, nullptr, &findData->gl) == 0)
			{
				findData->pathlen = searchWildcard.Path_Extract_Path().Length();
				findData->index = 0;
				return findData->gl.gl_pathv[findData->index] + findData->pathlen;
			}
#endif // _WIN32

			findData->searchPathId++;
		}

		return nullptr;
	}

#ifdef _WIN32
	return findData->wfd.cFileName;
#else
	findData->index++;
	return findData->gl.gl_pathv[findData->index] + findData->pathlen;
#endif // _WIN32
}

void CFileSystem::FindClose(DKFINDDATA* findData)
{
	if(!findData)
		return;

	if(m_findDatas.remove(findData))
	{
#ifdef _WIN32
		::FindClose(findData->fileHandle);
#else
		if(findData->index >= 0)
			globfree(&findData->gl);
#endif // _WIN32
		delete findData;
	}
}

bool CFileSystem::FindIsDirectory(DKFINDDATA* findData) const
{
	if(!findData)
		return false;

#ifdef _WIN32
	return (findData->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat st;

	if (stat(findData->gl.gl_pathv[findData->index], &st) == 0)
	{
		return (st.st_mode & S_IFDIR);
	}

	return false;
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

	HMODULE mod = LoadLibraryA( moduleFileName.ToCString() );

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

	HMODULE mod = dlopen( moduleFileName.ToCString(), RTLD_LAZY | RTLD_LOCAL );

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
	strcpy( pModule->name, moduleFileName.ToCString() );
	pModule->module = (HMODULE)mod;

	return pModule;
}

// frees module
void CFileSystem::FreeModule( DKMODULE* pModule )
{
	// don't unload any modules if we prining a leaklog
	//if( g_bPrintLeaksOnShutdown )
	//	return;

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
