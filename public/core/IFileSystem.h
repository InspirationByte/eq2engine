//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Filesystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IPackFileReader.h"

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

enum ESearchPath : int
{
	SP_ROOT = (1 << 0),	// Direct and absolute path (relative to baseDir)
	SP_DATA = (1 << 1),	// Engine data path
	SP_MOD	= (1 << 2),	// GameData and addons path

	SP_DIRECT = (1 << 3),			// same as SP_ROOT but baseDir is ignored
	SP_IGNORE_PACKAGE	= (1 << 4),	// ignore files from packages
};

struct FSFindData;

//------------------------------------------------------------------------------
// Filesystem interface
//------------------------------------------------------------------------------

class IFileSystem : public IEqCoreModule
{
	friend class CFileSystemFind;
public:
	CORE_INTERFACE("E2_Filesystem_012")

    // Initialization of filesystem
    virtual bool			Init(bool bEditorMode) = 0;
	virtual void			Shutdown() = 0;

	//------------------------------------------------------------
	// Directory stuff
	//------------------------------------------------------------

	// something like working directory
	virtual void			SetBasePath(const char* path) = 0;
	virtual const char*		GetBasePath() const = 0;
	
	virtual EqStringRef		GetAbsolutePath(ESearchPath search, const char* dirOrFileName) const = 0;
	virtual EqStringRef		GetAbsoluteFullPath(ESearchPath search, const char* dirOrFileName) const = 0;
	
	// Returns current game path
	virtual const char*		GetCurrentGameDirectory() const = 0;
	
	// Returns current engine data path
	virtual const char*		GetCurrentDataDirectory() const = 0;
	
	// adds data directory for file search
	virtual void			AddSearchPath(const char* pathId, const char* pszDir) = 0;
	virtual void			RemoveSearchPath(const char* pathId) = 0;
	
	// renames file or directory
	virtual bool			Rename(const char* oldNameOrPath, const char* newNameOrPath, ESearchPath search) const = 0;
	
	// Directory operations
	virtual bool			DirExist(const char* dirname, ESearchPath search) const = 0;
	virtual bool			MakeDir(const char* dirname, ESearchPath search ) const = 0;
	virtual bool			RemoveDir(const char* dirname, ESearchPath search ) const = 0;
	
	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------
	
	virtual IFileStreamPtr	Open( const char* filename, int openFlags, int searchFlags = -1) = 0;
	
	// other operations
	virtual EqString		FindFilePath(const char* filename, int searchFlags = -1) const = 0;
	virtual bool			FileExist(const char* filename, int searchFlags = -1) const = 0;
	virtual bool			FileRemove(const char* filename, ESearchPath search ) const = 0;
	virtual bool			FileCopy(const char* filename, const char* dest_file, bool overWrite, ESearchPath search) = 0;
	
	// The next ones are deprecated and will be removed
	
	virtual ubyte*			GetFileBuffer(const char* filename, VSSize* filesize = 0, int searchFlags = -1) = 0;
	virtual VSSize			GetFileSize(const char* filename, int searchFlags = -1) = 0;
	virtual uint32			GetFileCRC32(const char* filename, int searchFlags = -1) = 0;

	//------------------------------------------------------------
	// Packages
	//------------------------------------------------------------

	// sets access key for using encrypted packages
	virtual bool			SetAccessKey(const char* accessKey) = 0;

	// adds package to file system as another layer, acts just like AddSearchPath
	// NOTE: packageName must be root-level package file
	virtual bool			AddPackage(const char* packageName, ESearchPath type, const char* mountPath = nullptr) = 0;
	virtual void			RemovePackage(const char* packageName) = 0;

	// opens package for further reading. Does not add package as FS layer.
	virtual IPackFileReaderPtr OpenPackage(const char* packageName, int searchFlags = -1) = 0;

	//------------------------------------------------------------
	// Source control
	//------------------------------------------------------------

	// files written, removed or renamed by the calling thread go to the pending changelist
	// with this description until the scope is popped. Use SourceControlScope.
	virtual void			PushSourceControlChangelist(const char* description) = 0;
	virtual void			PopSourceControlChangelist() = 0;

	// true when written files are checked out and added, connects to the server on the first call
	virtual bool			IsSourceControlEnabled() const = 0;

	// checks out a depot file, or all depot files of a directory recursively, into the current
	// changelist. Writing many files of a checked out directory does not query the server per file.
	// Also used by writers which do not open files with Open.
	virtual void			CheckoutSourceControlFiles(const char* fileOrDirName, ESearchPath search) = 0;

	// adds a file created without Open, or checks it out when it is in the depot
	virtual void			AddSourceControlFile(const char* fileName, ESearchPath search) = 0;

protected:
	//------------------------------------------------------------
	// Locator
	//------------------------------------------------------------

	// opens directory for search props
	virtual const char* FindFirst(const char* wildcard, FSFindData** findData, int searchPaths = -1, int dirIndex = -1) = 0;
	virtual const char* FindNext(FSFindData* findData) const = 0;
	virtual void		FindClose(FSFindData* findData) = 0;
	virtual bool		FindIsDirectory(FSFindData* findData) const = 0;
	virtual int			FindGetDirIndex(FSFindData* findData) const = 0;
};

INTERFACE_SINGLETON(IFileSystem, g_fileSystem)

//-----------------------------------------------------------------------------------------
// Filesystem find helper class
//-----------------------------------------------------------------------------------------

class CFileSystemFind
{
public:
	CFileSystemFind() = default;
	CFileSystemFind(const char* wildcard, int searchPaths) 					{ Init(wildcard, searchPaths, -1); }
	CFileSystemFind(const char* wildcard, int searchPaths, int dirIndex)	{ Init(wildcard, searchPaths, dirIndex); }
	~CFileSystemFind();

	void		Init(const char* wildcard, int searchPaths, int dirIndex);
	int			GetDirIndex() const;
	bool		IsDirectory() const;
	const char*	GetPath() const;

	bool		Next();

protected:
	int			m_searchPaths{ SP_ROOT };
	int			m_startDirIndex{ -1 };
	EqString	m_wildcard{ nullptr };
	char*		m_curPath{ nullptr };
	FSFindData*	m_fd{ nullptr };
};

//-----------------------------------------------------------------------------------------

inline void CFileSystemFind::Init(const char* wildcard, int searchPaths, int dirIndex)
{
	m_wildcard = wildcard;
	m_searchPaths = searchPaths;
	m_startDirIndex = dirIndex;
}

inline CFileSystemFind::~CFileSystemFind()
{
	g_fileSystem->FindClose(m_fd);
}

inline bool  CFileSystemFind::IsDirectory() const
{
	return g_fileSystem->FindIsDirectory(m_fd);
}

inline int CFileSystemFind::GetDirIndex() const
{
	return g_fileSystem->FindGetDirIndex(m_fd);
}

inline const char* CFileSystemFind::GetPath() const
{
	return m_curPath;
}

inline bool CFileSystemFind::Next()
{
	if (!m_fd)
		m_curPath = (char*)g_fileSystem->FindFirst(m_wildcard, &m_fd, m_searchPaths, m_startDirIndex);
	else
		m_curPath = (char*)g_fileSystem->FindNext(m_fd);

	return (m_fd && m_curPath);
}

//-----------------------------------------------------------------------------------------
// Source control changelist scope
//-----------------------------------------------------------------------------------------

// Files written by the calling thread while the scope exists go to the pending changelist
// with the given description, it is created on the first file and reused when it exists.
// New files are added by one command when the scope ends.
// Does nothing when source control is not enabled.
//
//	SourceControlScope scScope("+export %s", levelName.ToCString());
class SourceControlScope
{
public:
	SourceControlScope(const char* descFormat, ...)
	{
		va_list argList;
		va_start(argList, descFormat);
		const EqString description(EqString::FormatV(descFormat, argList));
		va_end(argList);

		g_fileSystem->PushSourceControlChangelist(description);
	}

	~SourceControlScope()
	{
		g_fileSystem->PopSourceControlChangelist();
	}

	SourceControlScope(const SourceControlScope&) = delete;
	SourceControlScope& operator=(const SourceControlScope&) = delete;
};
