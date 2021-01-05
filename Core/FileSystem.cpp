//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#define CORE_INTERFACE_EXPORT
#include "InterfaceManager.h"

#include "FileSystem.h"

#include "utils/SmartPtr.h"
#include "utils/KeyValues.h"
#include "utils/strtools.h"
#include "IDkCore.h"
#include "ILocalize.h"

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
#include <glob.h>     // glob(), globfree()
#endif

#include "ConCommand.h"
#include "DebugInterface.h"
#include "utils/CRC32.h"

const char archiveKey[] = { 'W','@','k','m','U','5','1','c', 0 };

EXPORTED_INTERFACE(IFileSystem, CFileSystem);

DECLARE_CONCOMMAND_FN(addpackage)
{
	if (CMD_ARGC)
		g_fileSystem->AddPackage(CMD_ARGV(0).c_str(), SP_ROOT);
	else
		MsgWarning("Usage: fs_addpackage <package name>\n");
}

ConCommand* c_addpackage = NULL;

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

int	CFile::Seek( long pos, VirtStreamSeek_e seekType )
{
	return fseek( m_pFilePtr, pos, seekType );
}

long CFile::Tell()
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

	ubyte* pFileData = (ubyte*)malloc(fSize+16);

	Read(pFileData, 1, fSize);

	Seek(pos, VS_SEEK_SET);

	uint32 nCRC = CRC32_BlockChecksum( pFileData, fSize );

	free(pFileData);

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

CFileSystem::CFileSystem() : m_isInit(false), m_editorMode(false)
{
	// required by mobile port
	GetCore()->RegisterInterface( FILESYSTEM_INTERFACE_VERSION, this);
}

CFileSystem::~CFileSystem()
{

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
	ASSERTMSG(!m_isInit, "ERROR - Filesystem module already initialized!");

	if (m_isInit)
		return false;
	
    Msg("\n-------- Filesystem Init --------\n\n");

    m_editorMode = bEditorMode;

	kvkeybase_t* pFilesystem = GetCore()->GetConfig()->FindKeyBase("FileSystem", KV_FLAG_SECTION);

	if(!pFilesystem)
	{
		Msg("EQ.CONFIG missing FileSystemDirectories section!\n");
		return false;
	}

	if(m_basePath.Length() > 0)
		MsgInfo("* FS Init with basePath=%s\n", m_basePath.GetData());

	m_dataDir = KV_GetValueString(pFilesystem->FindKeyBase("EngineDataDir"), 0, "EngineBase" );

	MsgInfo("* Engine Data directory: %s\n", m_dataDir.GetData());

	// Change mod path?
    int iModPathArg = g_cmdLine->FindArgument("-game");

	if(!m_editorMode)
	{
		if (iModPathArg == -1)
			AddSearchPath("$GAME$", (const char*)KV_GetValueString(pFilesystem->FindKeyBase("DefaultGameDir"), 0, "DefaultGameDir_MISSING" ));
		else
			AddSearchPath("$GAME$",  g_cmdLine->GetArgumentString(iModPathArg+1) );

		 MsgInfo("* Game Data directory: %s\n", GetCurrentGameDirectory());
	}

	for(int i = 0;i < pFilesystem->keys.numElem(); i++)
	{
		if(!stricmp(pFilesystem->keys[i]->name, "AddPackage" ))
		{
			SearchPath_e packageSearchPathFlag = GetSearchPathByName(KV_GetValueString(pFilesystem->keys[i], 1, "SP_MOD"));

			if(!AddPackage( KV_GetValueString(pFilesystem->keys[i]), packageSearchPathFlag))
				return false;
		}
	}

	g_localizer->Init();

	c_addpackage = new ConCommand("fs_addpackage", CONCOMMAND_FN(addpackage), "Add packages");

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

	delete c_addpackage;
	c_addpackage = NULL;
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

	m_FSMutex.Lock();
	m_openFiles.fastRemove( fp );
	m_FSMutex.Unlock();
}

char* CFileSystem::GetFileBuffer(const char* filename,long *filesize/* = 0*/, int searchFlags/* = -1*/)
{
	if(!FileExist(filename, searchFlags))
		return NULL;

	IFile* pFile = Open(filename,"rb",searchFlags);

    if (!pFile)
        return NULL;

    long length = pFile->GetSize();

    char *buffer = (char*)PPAlloc(length+1);

    if (!buffer)
    {
        fprintf(stderr,"Memory error!");
        Close(pFile);
        return NULL;
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

#if 0
int CopyFileSlow(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}
#endif

bool CFileSystem::FileCopy(const char* filename, const char* dest_file, bool overWrite, SearchPath_e search)
{
	if( FileExist(filename, search) )
	{
#ifdef WIN32
		CopyFileA(filename, dest_file, (BOOL)overWrite);
#else
		ASSERTMSG(false, "CFileSystem::FileCopy for LINUX is undone!");
#endif
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

    char pFilePath[ MAX_PATH ];
    strcpy( pFilePath, filename );
    FixSlashes(pFilePath);

	char tmp_path[2048];

	EqString basePath = m_basePath;
	if(basePath.Length() > 0)
		basePath.Append( CORRECT_PATH_SEPARATOR );

    //First we checking mod directory
    if (flags & SP_MOD)
    {
		for(int i = 0; i < m_directories.numElem(); i++)
		{
			sprintf(tmp_path, "%s%s/%s", basePath.c_str(), m_directories[i].path.c_str(), pFilePath);
			if (access(tmp_path, F_OK ) != -1)
				return true;

			// base path is not used when dealing with package files
			sprintf(tmp_path, "%s/%s", m_directories[i].path.c_str(), pFilePath);

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
		sprintf(tmp_path, "%s%s/%s", basePath.c_str(), m_dataDir.c_str(), pFilePath);
		if (access(tmp_path, F_OK ) != -1)
			return true;

		// base path is not used when dealing with package files
		sprintf(tmp_path, "%s/%s", m_dataDir.c_str(), pFilePath);

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
		sprintf(tmp_path, "%s%s", basePath.c_str(), pFilePath);
		if (access(tmp_path, F_OK ) != -1)
			return true;

		// base path is not used when dealing with package files
		sprintf(tmp_path, "%s", pFilePath);

		// If failed to load directly, load it from package, in backward order
		for (int j = m_packages.numElem() - 1; j >= 0; j--)
		{
			CBasePackageFileReader* pPackageReader = m_packages[j];

			if ((flags & pPackageReader->GetSearchPath()) && pPackageReader->FileExists(tmp_path))
				return true;
		}
    }

	return false;
}

void CFileSystem::FileRemove(const char* filename, SearchPath_e search ) const
{
	EqString searchPath;

    switch (search)
    {
		case SP_DATA:
			searchPath = m_dataDir;
			break;
		case SP_MOD:
			searchPath = GetCurrentGameDirectory(); // Temporary set to data
			break;
		case SP_ROOT:
			searchPath = ".";
			break;
		default:
			searchPath = ".";
			break;
    }

    searchPath.Append(_Es("/") + filename);

	// finally!
	remove(searchPath.GetData());
}

//Directory operations
void CFileSystem::MakeDir(const char* dirname, SearchPath_e search ) const
{
    EqString searchPath;

    switch (search)
    {
    case SP_DATA:
        searchPath = m_dataDir;
        break;
    case SP_MOD:
        searchPath = GetCurrentGameDirectory(); // Temporary set to data
        break;
    case SP_ROOT:
        searchPath = ".";
        break;
    default:
        searchPath = "";
        break;
    }

	if(searchPath.Length() > 0)
		searchPath.Append(_Es(CORRECT_PATH_SEPARATOR) + dirname);
	else
		searchPath = dirname;

	if(	!(searchPath.c_str()[searchPath.Length()-1] == CORRECT_PATH_SEPARATOR  ||
		searchPath.c_str()[searchPath.Length()-1] == INCORRECT_PATH_SEPARATOR))
		searchPath.Append( CORRECT_PATH_SEPARATOR );

	searchPath.Path_FixSlashes();

#ifdef _WIN32
	_mkdir(searchPath.c_str());
#else
    mkdir(searchPath.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	int rv = remove(fpath);

	if (rv)
		perror(fpath);

	return rv;
}

void CFileSystem::RemoveDir(const char* dirname, SearchPath_e search ) const
{
    EqString searchPath;

    switch (search)
    {
		case SP_DATA:
			searchPath = m_dataDir;
			break;
		case SP_MOD:
			searchPath = GetCurrentGameDirectory(); // Temporary set to data
			break;
		case SP_ROOT:
			searchPath = ".";
			break;
		default:
			searchPath = ".";
			break;
    }

    searchPath.Append(_Es("/") + dirname);

    rmdir( searchPath.GetData() );
}

void CFileSystem::Rename(const char* oldNameOrPath, const char* newNameOrPath, SearchPath_e search) const
{
	EqString oldName;

	switch (search)
	{
		case SP_DATA:
			oldName = m_dataDir;
			break;
		case SP_MOD:
			oldName = GetCurrentGameDirectory(); // Temporary set to data
			break;
		case SP_ROOT:
			oldName = ".";
			break;
		default:
			oldName = ".";
			break;
	}

	EqString newName(oldName + _Es("/") + newNameOrPath);
	oldName.Append(_Es("/") + oldNameOrPath);

	rename(oldName.c_str(), newName.c_str());
}

//Filesystem's check and open file
IFile* CFileSystem::GetFileHandle(const char* filename,const char* options, int searchFlags )
{
    int flags = searchFlags;
    if (flags == -1)
        flags |= SP_MOD | SP_DATA | SP_ROOT;

	bool isWrite = (strchr(options, 'w') || strchr(options, 'a') || strchr(options, '+'));

    char pFilePath[ MAX_PATH ];
    strcpy( pFilePath, filename );
    FixSlashes(pFilePath);

	char tmp_path[2048];

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

			sprintf(tmp_path, "%s%s/%s", basePath.c_str(), m_directories[i].path.c_str(), pFilePath);

			FILE *tmpFile = fopen(tmp_path,options);
			if (tmpFile)
			{
				CFile* pFileHandle = new CFile(tmpFile);

				m_FSMutex.Lock();
				m_openFiles.append(pFileHandle);
				m_FSMutex.Unlock();

				return pFileHandle;
			}

			if (!isWrite)
			{
				// base path is not used when dealing with package files
				sprintf(tmp_path, "%s/%s", m_directories[i].path.c_str(), pFilePath);

				// If failed to load directly, load it from package, in backward order
				for (int j = m_packages.numElem() - 1; j >= 0; j--)
				{
					CBasePackageFileReader* pPackageReader = m_packages[j];

					if (!(flags & pPackageReader->GetSearchPath()))
						continue;

					IFile* pPackedFile = pPackageReader->Open(tmp_path, options);

					if (pPackedFile)
					{
						m_FSMutex.Lock();
						m_openFiles.append(pPackedFile);
						m_FSMutex.Unlock();

						return pPackedFile;
					}
				}
			}
		}
    }

    //Then we checking data directory
    if (flags & SP_DATA)
    {
		sprintf(tmp_path, "%s%s/%s", basePath.c_str(), m_dataDir.c_str(), pFilePath);

        FILE *tmpFile = fopen(tmp_path,options);
        if (tmpFile)
        {
			CFile* pFileHandle = new CFile(tmpFile);
			m_FSMutex.Lock();
			m_openFiles.append(pFileHandle);
			m_FSMutex.Unlock();

			return pFileHandle;
        }

		if (!isWrite)
		{
			// base path is not used when dealing with package files
			sprintf(tmp_path, "%s/%s", m_dataDir.c_str(), pFilePath);

			// If failed to load directly, load it from package, in backward order
			for (int j = m_packages.numElem() - 1; j >= 0; j--)
			{
				CBasePackageFileReader* pPackageReader = m_packages[j];

				if (!(flags & pPackageReader->GetSearchPath()))
					continue;

				IFile* pPackedFile = pPackageReader->Open(tmp_path, options);

				if (pPackedFile)
				{
					m_FSMutex.Lock();
					m_openFiles.append(pPackedFile);
					m_FSMutex.Unlock();

					return pPackedFile;
				}
			}
		}
    }

    // And checking root.
    // not adding basepath to this
    if (flags & SP_ROOT)
    {
		sprintf(tmp_path, "%s%s", basePath.c_str(), pFilePath);

        FILE *tmpFile = fopen(tmp_path,options);
		if (tmpFile)
		{
			CFile* pFileHandle = new CFile(tmpFile);
			m_openFiles.append(pFileHandle);

			return pFileHandle;
		}

		if (!isWrite)
		{
			// base path is not used when dealing with package files
			sprintf(tmp_path, "%s", pFilePath);

			// If failed to load directly, load it from package, in backward order
			for (int j = m_packages.numElem() - 1; j >= 0; j--)
			{
				CBasePackageFileReader* pPackageReader = m_packages[j];

				if (!(flags & pPackageReader->GetSearchPath()))
					continue;

				IFile* pPackedFile = pPackageReader->Open(tmp_path, options);

				if (pPackedFile)
				{
					m_FSMutex.Lock();
					m_openFiles.append(pPackedFile);
					m_FSMutex.Unlock();

					return pPackedFile;
				}
			}
		}
    }

    //Return NULL filename if file not found
    return NULL;
}

bool CFileSystem::AddPackage(const char* packageName, SearchPath_e type)
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
		pPackageReader = new CZipFileReader(m_FSMutex);
	else
		pPackageReader = new CDPKFileReader(m_FSMutex);

    if (pPackageReader->SetPackageFilename( packageName ))
    {
        DevMsg(DEVMSG_FS, "Adding package file '%s'\n",packageName);

        pPackageReader->SetSearchPath(type);
		pPackageReader->SetKey(archiveKey);

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
		m_directories.append(pathInfo);
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
			return m_directories[i].path.c_str();
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

	DKFINDDATA* newFind = new DKFINDDATA;
	*findData = newFind;

	newFind->searchPathId = -1;
	newFind->wildcard = wildcard;
	newFind->wildcard.Path_FixSlashes();

#ifndef _WIN32
	newFind->wildcard.ReplaceSubstr("*.*", "*");
#endif

	m_findDatas.append(newFind);

	EqString fsBaseDir = m_basePath.c_str();

	if(searchPath == SP_DATA)
	{
		fsBaseDir = CombinePath(2, m_basePath.c_str(), m_dataDir.c_str());
	}
	else if(searchPath == SP_MOD)
	{
		newFind->searchPathId = 0;
		fsBaseDir = CombinePath(2, m_basePath.c_str(), m_directories[0].path.c_str());
	}

	EqString searchWildcard( CombinePath(2, fsBaseDir.c_str(), newFind->wildcard.c_str()) );

#ifdef _WIN32
	newFind->fileHandle = ::FindFirstFileA(searchWildcard.c_str(), &newFind->wfd);

	if(newFind->fileHandle != INVALID_HANDLE_VALUE)
		return newFind->wfd.cFileName;

#else // POSIX
	newFind->index = -1;

	if (glob(searchWildcard.c_str(), 0, NULL, &newFind->gl) == 0)
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
			fsBaseDir = CombinePath(2, m_basePath.c_str(), m_directories[newFind->searchPathId].path.c_str());

			searchWildcard = CombinePath(2, fsBaseDir.c_str(), newFind->wildcard.c_str());

#ifdef _WIN32
			newFind->fileHandle = ::FindFirstFileA(searchWildcard.c_str(), &newFind->wfd);

			if(newFind->fileHandle != INVALID_HANDLE_VALUE)
				return newFind->wfd.cFileName;
#else // POSIX
			if (glob(searchWildcard.c_str(), 0, NULL, &newFind->gl) == 0)
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
			EqString fsBaseDir = CombinePath(2, m_basePath.c_str(), m_directories[findData->searchPathId].path.c_str());

			EqString searchWildcard( CombinePath(2, fsBaseDir.c_str(), findData->wildcard.c_str()) );

#ifdef _WIN32
			// close existing find
			::FindClose(findData->fileHandle);

			// tre init new
			findData->fileHandle = ::FindFirstFileA(searchWildcard.c_str(), &findData->wfd);

			if(findData->fileHandle != INVALID_HANDLE_VALUE)
				return findData->wfd.cFileName;
#else // POSIX
			globfree(&findData->gl);
			findData->index = -1;

			if (glob(searchWildcard.c_str(), 0, NULL, &findData->gl) == 0)
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
		return (st.st_mode & S_IFDIR) > 0;
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

	HMODULE mod = LoadLibraryA( moduleFileName.c_str() );

	DWORD lastErr = GetLastError();

	char err[256] = {'\0'};

	if(lastErr != 0)
	{
		FormatMessageA(	FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						lastErr,
						MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
						err,
						255,
						NULL);
	}

#else
	// make default module extension
	if(modExt.Length() == 0)
		moduleFileName = moduleFileName + ".so";

	HMODULE mod = dlopen( moduleFileName.c_str(), RTLD_LAZY | RTLD_LOCAL );

	const char* err = dlerror();
	int lastErr = 0;
#endif // _WIN32 && MEMDLL

	MsgInfo("Loading module '%s'\n", moduleFileName.c_str());

	if( !mod )
	{
        MsgInfo("Library '%s' loading error '%s', 0x%p\n", moduleFileName.c_str(), err, lastErr);

		ErrorMsg("CFileSystem::LoadModule Error: Failed to load %s\n - %s!", moduleFileName.c_str(), err);
		return NULL;
	}

	DKMODULE* pModule = new DKMODULE;
	strcpy( pModule->name, moduleFileName.c_str() );
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
