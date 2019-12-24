//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Filesystem
//////////////////////////////////////////////////////////////////////////////////

#include "FileSystem.h"

#include "utils/SmartPtr.h"
#include "utils/KeyValues.h"
#include "utils/strtools.h"
#include "IDkCore.h"
#include "ILocalize.h"

#ifdef _WIN32 // Not in linux

#include <direct.h>	// mkdir()
#include <io.h> // _access()
#define access		_access
#define F_OK		0       // Test for existence.
#else
#include <unistd.h> // rmdir(), access()
#include <stdarg.h> // va_*
#include <dlfcn.h>
#endif

#include "DebugInterface.h"
#include "utils/CRC32.h"

EXPORTED_INTERFACE(IFileSystem, CFileSystem);

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
// Virtual filesystem file of DarkTech Package file
//------------------------------------------------------------------------------

int	CVirtualDPKFile::Seek( long pos, VirtStreamSeek_e seekType )
{
	return m_pDKPReader->Seek( m_pFilePtr, pos, seekType );
}

long CVirtualDPKFile::Tell()
{
	return m_pDKPReader->Tell( m_pFilePtr );
}

size_t CVirtualDPKFile::Read( void *dest, size_t count, size_t size)
{
	return m_pDKPReader->Read( dest, count, size, m_pFilePtr );
}

size_t CVirtualDPKFile::Write( const void *src, size_t count, size_t size)
{
	ASSERTMSG(false, "CVirtualDPKFile::Write: DPK files is not allowed to write! Use fcompress.");

	return 0;
}

int	CVirtualDPKFile::Error()
{
	// NOT IMPLEMENTED

	return 0;
}

int	CVirtualDPKFile::Flush()
{
	// NOT IMPLEMENTED

	return 0;
}

char* CVirtualDPKFile::Gets( char *dest, int destSize )
{
	return m_pDKPReader->Gets(dest, destSize, m_pFilePtr );
}

uint32 CVirtualDPKFile::GetCRC32()
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

long CVirtualDPKFile::GetSize()
{
	long pos = Tell();

	Seek(0, VS_SEEK_END);

	long length = Tell();

	Seek(pos, VS_SEEK_SET);

	return length;
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

bool CFileSystem::Init(bool bEditorMode)
{
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

	for(int i = 0;i < pFilesystem->keys.numElem();i++)
	{
		if(!stricmp(pFilesystem->keys[i]->name, "AddPackage" ))
		{
			if(!AddPackage( KV_GetValueString(pFilesystem->keys[i]), SP_MOD ))
				return false;
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

	VirtStreamType_e vsType = fp->GetType();

	if(vsType == VS_TYPE_FILE)
	{
		CFile* pFile = (CFile*)fp;
		fclose(pFile->m_pFilePtr);
	}
	else if(vsType == VS_TYPE_FILE_PACKAGE)
	{
		CVirtualDPKFile* pFile = (CVirtualDPKFile*)fp;
		ASSERT(pFile->m_pDKPReader);

		pFile->m_pDKPReader->Close(pFile->m_pFilePtr);
	}

	delete fp;

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
		}
    }

    //Then we checking data directory
    if (flags & SP_DATA)
    {
		sprintf(tmp_path, "%s%s/%s", basePath.c_str(), m_dataDir.c_str(), pFilePath);
		if (access(tmp_path, F_OK ) != -1)
			return true;
    }

    // And checking root.
    // not adding basepath to this
    if (flags & SP_ROOT)
    {
		sprintf(tmp_path, "%s%s", basePath.c_str(), pFilePath);
		if (access(tmp_path, F_OK ) != -1)
			return true;
    }

	// check packages
    for (int i = m_packages.numElem()-1; i >= 0;i--)
    {
        CDPKFileReader* pPackageReader = m_packages[i];

        if (flags & SP_MOD)
        {
			for(int j = 0; j < m_directories.numElem(); j++)
			{
				sprintf(tmp_path, "%s/%s", m_directories[j].path.c_str(),pFilePath);
				if (pPackageReader->FindFileIndex( tmp_path ) != -1)
					return true;
			}
        }
        if (flags & SP_DATA)
        {
			sprintf(tmp_path, "%s/%s",m_dataDir.GetData(),pFilePath);
			if (pPackageReader->FindFileIndex( tmp_path ) != -1)
				return true;
        }
        if (flags & SP_ROOT)
        {
			if (pPackageReader->FindFileIndex( pFilePath ) != -1)
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
			if (isWrite && m_directories[i].mainWritePath)
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
    }

    // If failed to load directly, load it from package, in backward order
    // NOTE: basepath is not added to DPK paths
    for (int i = m_packages.numElem()-1; i >= 0;i--)
    {
        CDPKFileReader* pPackageReader = m_packages[i];
		//DkStr temp;
        if (flags & SP_MOD)
        {
			for(int j = 0; j < m_directories.numElem(); j++)
			{
				sprintf(tmp_path, "%s/%s", m_directories[j].path.c_str(),pFilePath);

				DPKFILE* pPackedFile = pPackageReader->Open(tmp_path, options);

				if (pPackedFile)
				{
					pPackedFile->packageId = i;

					CVirtualDPKFile* pFileHandle = new CVirtualDPKFile(pPackedFile, pPackageReader);
					m_FSMutex.Lock();
					m_openFiles.append(pFileHandle);
					m_FSMutex.Unlock();

					return pFileHandle;
				}
			}
        }
        if (flags & SP_DATA)
        {
			sprintf(tmp_path, "%s/%s",m_dataDir.GetData(),pFilePath);

            DPKFILE* pPackedFile = pPackageReader->Open(tmp_path,options);

            if (pPackedFile)
            {
                pPackedFile->packageId = i;

				CVirtualDPKFile* pFileHandle = new CVirtualDPKFile(pPackedFile, pPackageReader);
				m_FSMutex.Lock();
				m_openFiles.append(pFileHandle);
				m_FSMutex.Unlock();

				return pFileHandle;
            }
        }
        if (flags & SP_ROOT)
        {
            DPKFILE* pPackedFile = pPackageReader->Open(pFilePath,options);

            if (pPackedFile)
            {
                pPackedFile->packageId = i;

				CVirtualDPKFile* pFileHandle = new CVirtualDPKFile(pPackedFile, pPackageReader);
				m_FSMutex.Lock();
				m_openFiles.append(pFileHandle);
				m_FSMutex.Unlock();

				return pFileHandle;
            }
        }
    }

    //Return NULL filename if file not found
    return NULL;
}

bool CFileSystem::AddPackage(const char* packageName,SearchPath_e type)
{
	for(int i = 0; i < m_packages.numElem();i++)
	{
		if(!stricmp(m_packages[i]->GetPackageFilename(), packageName))
			return false;
	}

    CDPKFileReader* pPackageReader = new CDPKFileReader(m_FSMutex);

    if (pPackageReader->SetPackageFilename( packageName ))
    {
        DevMsg(DEVMSG_FS, "Adding package file '%s'\n",packageName);

        pPackageReader->SetSearchPath(type);
		//pPackageReader->SetKey("SdkwIuO4");

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

	bool isMod = strstr(pathId, "$MOD$") != nullptr;

	SearchPath_t pathInfo;
	pathInfo.id = pathId;
	pathInfo.path = pszDir;
	pathInfo.mainWritePath = !isMod;

	if(isMod)
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

// extracts single file from package
void CFileSystem::ExtractFile(const char* filename, bool onlyNonExist)
{
	if(onlyNonExist)
	{
		FILE* file = fopen(filename, "rb");
		if(file)
		{
			fclose(file);
			return;
		}
	}

	// do it from last package
    for (int i = m_packages.numElem()-1; i >= 0;i--)
	{
		int file_id = m_packages[i]->FindFileIndex( filename );
		if( file_id != -1 )
		{
			DPKFILE* pFile = m_packages[i]->Open(filename, "rb");

			m_packages[i]->Seek(pFile, 0, SEEK_END);
			long file_size = m_packages[i]->Tell(pFile);
			m_packages[i]->Seek(pFile, 0, SEEK_SET);

			ubyte* file_buffer = new ubyte[file_size];

			m_packages[i]->Read(file_buffer, 1, file_size, pFile);
			m_packages[i]->Close(pFile);

			FILE* pSavedFile = fopen(filename, "wb");

			if(pSavedFile)
			{
				fwrite(file_buffer, file_size, 1, pSavedFile);
				fclose(pSavedFile);
			}
			else
				MsgError("Can't dump file!\n");

			delete [] file_buffer;

			//exit because we dont want more same files to be extracted
			return;
		}
	}
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

	m_findDatas.append(newFind);

	EqString fsBaseDir;

	if(searchPath == SP_DATA)
	{
		fsBaseDir = m_dataDir;
	}
	else if(searchPath == SP_MOD)
	{
		newFind->searchPathId = 0;
		fsBaseDir = m_directories[0].path;
	}

	EqString searchWildcard( CombinePath(2, fsBaseDir.c_str(), newFind->wildcard.c_str()) );

#ifdef _WIN32
	newFind->fileHandle = ::FindFirstFileA(searchWildcard.c_str(), &newFind->wfd);

	if(newFind->fileHandle != NULL)
		return newFind->wfd.cFileName;
#else

#endif // _WIN32

	if(newFind->searchPathId != -1)
	{
		newFind->searchPathId++;

		// try reinitialize
		while(newFind->searchPathId < m_directories.numElem())
		{
			fsBaseDir = m_directories[newFind->searchPathId].path;

			searchWildcard = CombinePath(2, fsBaseDir.c_str(), newFind->wildcard.c_str());

#ifdef _WIN32
			newFind->fileHandle = ::FindFirstFileA(searchWildcard.c_str(), &newFind->wfd);

			if(newFind->fileHandle != NULL)
				return newFind->wfd.cFileName;
#else

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
//#error "POSIX FindNextFile"
    return nullptr;
#endif // _WIN32
	{
		if(findData->searchPathId == -1)
			return nullptr;

		findData->searchPathId++;

		// try reinitialize
		while(findData->searchPathId < m_directories.numElem())
		{
			EqString fsBaseDir(m_directories[findData->searchPathId].path);

			EqString searchWildcard( CombinePath(2, fsBaseDir.c_str(), findData->wildcard.c_str()) );

#ifdef _WIN32
			// close existing find
			::FindClose(findData->fileHandle);

			// tre init new
			findData->fileHandle = ::FindFirstFileA(searchWildcard.c_str(), &findData->wfd);

			if(findData->fileHandle != NULL)
				return findData->wfd.cFileName;
#else

#endif // _WIN32

			findData->searchPathId++;
		}

		return nullptr;
	}

#ifdef _WIN32
	return findData->wfd.cFileName;
#else

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
