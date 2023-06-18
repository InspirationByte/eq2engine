//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Filesystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IFilePackageReader.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

enum ESearchPath
{
    SP_DATA = (1 << 1),
    SP_ROOT = (1 << 2),
    SP_MOD	= (1 << 3),
};

using IFile = IVirtualStream; // TODO: wrap around RefCounted instead
struct DKMODULE; // module structure
struct DKFINDDATA;

//------------------------------------------------------------------------------
// Filesystem interface
//------------------------------------------------------------------------------

class IFileSystem : public IEqCoreModule
{
	friend class CFileSystemFind;
public:
	CORE_INTERFACE("E2_Filesystem_007")

    // Initialization of filesystem
    virtual bool			Init(bool bEditorMode) = 0;
	virtual void			Shutdown() = 0;

	//------------------------------------------------------------
	// Directory stuff
	//------------------------------------------------------------

	// something like working directory
	virtual void			SetBasePath(const char* path) = 0;
	virtual const char*		GetBasePath() const = 0;

	virtual EqString		GetAbsolutePath(ESearchPath search, const char* dirOrFileName) const = 0;

    // Returns current game path
    virtual const char*		GetCurrentGameDirectory() const = 0;

    // Returns current engine data path
    virtual const char*		GetCurrentDataDirectory() const = 0;

	// adds data directory for file search
	virtual void			AddSearchPath(const char* pathId, const char* pszDir) = 0;
	virtual void			RemoveSearchPath(const char* pathId) = 0;

	// renames file or directory
	virtual void			Rename(const char* oldNameOrPath, const char* newNameOrPath, ESearchPath search) const = 0;

    // Directory operations
    virtual void			MakeDir(const char* dirname, ESearchPath search ) const = 0;
    virtual void			RemoveDir(const char* dirname, ESearchPath search ) const = 0;

	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------

    virtual IFile*			Open( const char* filename, const char* mode = "r", int searchFlags = -1) = 0;
    virtual void			Close( IFile* pFile ) = 0;

	// other operations
	virtual bool			FileExist(const char* filename, int searchFlags = -1) const = 0;
	virtual void			FileRemove(const char* filename, ESearchPath search ) const = 0;
	virtual bool			FileCopy(const char* filename, const char* dest_file, bool overWrite, ESearchPath search) = 0;

	// The next ones are deprecated and will be removed

    virtual char*			GetFileBuffer(const char* filename, long *filesize = 0, int searchFlags = -1) = 0;
    virtual long			GetFileSize(const char* filename, int searchFlags = -1) = 0;
	virtual uint32			GetFileCRC32(const char* filename, int searchFlags = -1) = 0;

	//------------------------------------------------------------
	// Packages
	//------------------------------------------------------------

	// sets access key for using encrypted packages
	virtual bool			SetAccessKey(const char* accessKey) = 0;

	// adds package to file system as another layer, acts just like AddSearchPath
	virtual bool			AddPackage(const char* packageName, ESearchPath type, const char* mountPath = nullptr) = 0;
	virtual void			RemovePackage(const char* packageName) = 0;

	// opens package for further reading. Does not add package as FS layer. NOTE: it's not supported to open package inside EPK or ZIP file.
	virtual IFilePackageReader* OpenPackage(const char* packageName) = 0;
	virtual void			ClosePackage(IFilePackageReader* package) = 0;

	//------------------------------------------------------------
	// Dynamic library stuff
	//------------------------------------------------------------

	// loads module
	virtual DKMODULE*		OpenModule(const char* mod_name, EqString* outError = nullptr) = 0;

	// frees module
	virtual void			CloseModule( DKMODULE* pModule ) = 0;

	// returns procedure address of the loaded module
	virtual void*			GetProcedureAddress(DKMODULE* pModule, const char* pszProc) = 0;

protected:
	//------------------------------------------------------------
	// Locator
	//------------------------------------------------------------

	// opens directory for search props
	virtual const char* FindFirst(const char* wildcard, DKFINDDATA** findData, int searchPath) = 0;
	virtual const char* FindNext(DKFINDDATA* findData) const = 0;
	virtual void		FindClose(DKFINDDATA* findData) = 0;
	virtual bool		FindIsDirectory(DKFINDDATA* findData) const = 0;
};

INTERFACE_SINGLETON( IFileSystem, CFileSystem, g_fileSystem )

//-----------------------------------------------------------------------------------------
// Filesystem find helper class
//-----------------------------------------------------------------------------------------

class CFileSystemFind
{
public:
	CFileSystemFind();
	CFileSystemFind(const char* wildcard, int searchPath = -1);
	~CFileSystemFind();

	void			Init(const char* wildcard, int searchPath = -1);
	bool			IsDirectory() const;
	const char*		GetPath() const;

	bool			Next();

protected:
	int				m_searchPath;
	char*			m_wildcard;

	char*			m_curPath;
	DKFINDDATA*		m_fd;
};

//-----------------------------------------------------------------------------------------

inline CFileSystemFind::CFileSystemFind() : m_fd(nullptr), m_curPath(nullptr)
{
}

inline CFileSystemFind::CFileSystemFind(const char* wildcard, int searchPath) : CFileSystemFind()
{
	Init(wildcard, searchPath);
}

inline void CFileSystemFind::Init(const char* wildcard, int searchPath)
{
	m_wildcard = (char*)wildcard;
	m_searchPath = searchPath;
}

inline CFileSystemFind::~CFileSystemFind()
{
	g_fileSystem->FindClose(m_fd);
}

inline bool  CFileSystemFind::IsDirectory() const
{
	return g_fileSystem->FindIsDirectory(m_fd);
}

inline const char* CFileSystemFind::GetPath() const
{
	return m_curPath;
}

inline bool CFileSystemFind::Next()
{
	if (!m_fd)
		m_curPath = (char*)g_fileSystem->FindFirst(m_wildcard, &m_fd, m_searchPath);
	else
		m_curPath = (char*)g_fileSystem->FindNext(m_fd);

	return (m_fd && m_curPath);
}
