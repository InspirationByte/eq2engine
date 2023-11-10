//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IFileSystem.h"
#include "core/platform/OSFile.h"

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

class CFile : public IFile
{
	friend class CFileSystem;

public:
	CFile(const char* fileName, COSFile&& file);

    int					Seek(int pos, EVirtStreamSeek seekType );
    int					Tell() const;
    size_t				Read( void *dest, size_t count, size_t size);
    size_t				Write( const void *src, size_t count, size_t size);

    bool				Flush();

	uint32				GetCRC32();

	int					GetSize();

	VirtStreamType_e	GetType() const { return VS_TYPE_FILE; }

	const char*			GetName() const { return m_name; }
protected:
	EqString			m_name;
	COSFile				m_osFile;
};

//------------------------------------------------------------------------------
// Filesystem base
//------------------------------------------------------------------------------

class CBasePackageReader;

class CFileSystem : public IFileSystem
{
	friend class CFile;
	friend class CDPKFileReader;
	friend class CZipFileReader;
public:
								 CFileSystem();
								~CFileSystem();

    //Initialization of filesystem
    bool						Init( bool bEditorMode );
	void						Shutdown();

	//------------------------------------------------------------
	// Directory stuff
	//------------------------------------------------------------

	// something like working directory
	void						SetBasePath(const char* path);
	const char*					GetBasePath() const				{return m_basePath.ToCString();}

    // Returns current game path
    const char*					GetCurrentGameDirectory() const;

    // Returns current engine data path
    const char*					GetCurrentDataDirectory() const;

	// adds directory for file search
	void						AddSearchPath(const char* pathId, const char* pszDir);
	void						RemoveSearchPath(const char* pathId);

	void						Rename(const char* oldNameOrPath, const char* newNameOrPath, ESearchPath search) const;

    //Directory operations
    void						MakeDir(const char* dirname, ESearchPath search ) const;
    void						RemoveDir(const char* dirname, ESearchPath search ) const;

	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------

    IFilePtr					Open(const char* filename, const char* mode, int searchFlags = -1 );

	EqString					FindFilePath(const char* filename, int searchFlags = -1) const;
	bool						FileCopy(const char* filename, const char* dest_file, bool overWrite, ESearchPath search);
	bool						FileExist(const char* filename, int searchFlags = -1) const;
	void						FileRemove(const char* filename, ESearchPath search ) const;

	// The next ones are deprecated and will be removed

    ubyte*						GetFileBuffer(const char* filename, int* filesize = 0, int searchFlags = -1);
	int							GetFileSize(const char* filename, int searchFlags = -1);
	uint32						GetFileCRC32(const char* filename, int searchFlags = -1);

	//------------------------------------------------------------
	// Packages
	//------------------------------------------------------------

	bool						SetAccessKey(const char* accessKey);

	// adds package to file system as another layer, acts just like AddSearchPath
	// NOTE: packageName must be root-level package file
	bool						AddPackage(const char* packageName, ESearchPath type, const char* mountPath = nullptr);
	void						RemovePackage(const char* packageName);

	// opens package for further reading. Does not add package as FS layer
	IFilePackageReader*			OpenPackage(const char* packageName, int searchFlags = -1);
	void						ClosePackage(IFilePackageReader* package);

	//------------------------------------------------------------
	// Locator
	//------------------------------------------------------------

	// opens directory for search props
	const char*					FindFirst(const char* wildcard, DKFINDDATA** findData, int searchPaths);
	const char*					FindNext(DKFINDDATA* findData) const;
	void						FindClose(DKFINDDATA* findData);
	bool						FindIsDirectory(DKFINDDATA* findData) const;

	//------------------------------------------------------------
	// Dynamic library stuff
	//------------------------------------------------------------

	// loads module
	DKMODULE*					OpenModule(const char* mod_name, EqString* outError = nullptr);

	// frees module
	void						CloseModule( DKMODULE* pModule );

	// returns procedure address of the loaded module
	void*						GetProcedureAddress(DKMODULE* pModule, const char* pszProc);

	//-------------------------
	bool						IsInitialized() const		{return m_isInit;}

protected:

	bool						InitNextPath(DKFINDDATA* findData) const;

	EqString					GetAbsolutePath(ESearchPath search, const char* dirOrFileName) const;
	EqString					GetSearchPath(ESearchPath search, int directoryId = -1) const;


	using SPWalkFunc = EqFunction<bool(const EqString& filePath, ESearchPath searchPath, int spFlags, bool writePath)>;
	bool						WalkOverSearchPaths(int searchFlags, const char* fileName, const SPWalkFunc& func) const;

	EqString					m_basePath;			// base prepended path
    EqString					m_dataDir;			// Used to load engine data
	EqString					m_accessKey;

	struct SearchPathInfo
	{
#ifdef PLAT_LINUX
		// name hash to file path mapping (case-insensitive support)
		Map<int, EqString> pathToFileMapping{ PP_SL };
#endif
		EqString	id;
		EqString	path;
		bool		mainWritePath;
	};

	Array<SearchPathInfo*>		m_directories{ PP_SL };		// mod data, for fall back
    Array<CBasePackageReader*>	m_fsPackages{ PP_SL };		// package serving as FS layers
	Array<IFilePackageReader*>	m_openPackages{ PP_SL };

	Array<DKFINDDATA*>			m_findDatas{ PP_SL };
	Array<DKMODULE*>			m_modules{ PP_SL };

    bool						m_editorMode{ false };
	bool						m_isInit{ false };
};

