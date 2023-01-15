//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IFileSystem.h"

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

class CFile : public IFile
{
	friend class CFileSystem;

public:
						CFile(FILE* pFile) : m_pFilePtr(pFile)
						{
						}

    int					Seek( long pos, VirtStreamSeek_e seekType );
    long				Tell() const;
    size_t				Read( void *dest, size_t count, size_t size);
    size_t				Write( const void *src, size_t count, size_t size);
    int					Error();
    int					Flush();

    char*				Gets( char *dest, int destSize );

	uint32				GetCRC32();

	long				GetSize();

	VirtStreamType_e	GetType() const {return VS_TYPE_FILE;}

protected:
	FILE*				m_pFilePtr;
};

//------------------------------------------------------------------------------
// Filesystem base
//------------------------------------------------------------------------------

class CBasePackageFileReader;

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

	void						Rename(const char* oldNameOrPath, const char* newNameOrPath, SearchPath_e search) const;

    //Directory operations
    void						MakeDir(const char* dirname, SearchPath_e search ) const;
    void						RemoveDir(const char* dirname, SearchPath_e search ) const;

	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------

    IFile*						Open(const char* filename,const char* options, int searchFlags = -1 );
    void						Close( IFile *fp );

	bool						FileCopy(const char* filename, const char* dest_file, bool overWrite, SearchPath_e search);
	bool						FileExist(const char* filename, int searchFlags = -1) const;
	void						FileRemove(const char* filename, SearchPath_e search ) const;

	// The next ones are deprecated and will be removed

    char*						GetFileBuffer(const char* filename,long *filesize = 0, int searchFlags = -1);
    long						GetFileSize(const char* filename, int searchFlags = -1);
	uint32						GetFileCRC32(const char* filename, int searchFlags = -1);

	//------------------------------------------------------------
	// Packages
	//------------------------------------------------------------

	bool						SetAccessKey(const char* accessKey);

	bool						AddPackage(const char* packageName, SearchPath_e type, const char* mountPath = nullptr);
	void						RemovePackage(const char* packageName);

	//------------------------------------------------------------
	// Locator
	//------------------------------------------------------------

	// opens directory for search props
	const char*					FindFirst(const char* wildcard, DKFINDDATA** findData, int searchPath);
	const char*					FindNext(DKFINDDATA* findData) const;
	void						FindClose(DKFINDDATA* findData);
	bool						FindIsDirectory(DKFINDDATA* findData) const;

	//------------------------------------------------------------
	// Dynamic library stuff
	//------------------------------------------------------------

	// loads module
	DKMODULE*					LoadModule(const char* mod_name);

	// frees module
	void						FreeModule( DKMODULE* pModule );

	// returns procedure address of the loaded module
	void*						GetProcedureAddress(DKMODULE* pModule, const char* pszProc);

	//-------------------------
	bool						IsInitialized() const		{return m_isInit;}
	const char*					GetInterfaceName() const	{return FILESYSTEM_INTERFACE_VERSION;}

protected:

	EqString					GetAbsolutePath(SearchPath_e search, const char* dirOrFileName) const;
	EqString					GetSearchPath(SearchPath_e search, int directoryId = -1) const;


	using SPWalkFunc = EqFunction<bool(const EqString& filePath, SearchPath_e searchPath, int spFlags, bool writePath)>;
	bool						WalkOverSearchPaths(int searchFlags, const char* fileName, const SPWalkFunc& func) const;

	EqString					m_basePath;			// base prepended path
    EqString					m_dataDir;			// Used to load engine data
	EqString					m_accessKey;

	struct SearchPath_t
	{
		EqString id;
		EqString path;
		bool mainWritePath;
	};

	Array<SearchPath_t>				m_directories{ PP_SL };		// mod data, for fall back

    // Packages currently loaded
    Array<CBasePackageFileReader*>	m_packages{ PP_SL };
    Array<IFile*>					m_openFiles{ PP_SL };
	Array<DKFINDDATA*>				m_findDatas{ PP_SL };
	Array<DKMODULE*>				m_modules{ PP_SL };

    bool							m_editorMode;
	bool							m_isInit;

	Threading::CEqMutex				m_FSMutex;
};

