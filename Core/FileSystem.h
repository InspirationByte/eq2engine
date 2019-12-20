//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Filesystem
//////////////////////////////////////////////////////////////////////////////////

#ifndef CFILESYSTEM_H
#define CFILESYSTEM_H

#include "IFileSystem.h"
#include "DPKFileReader.h"
#include "utils/eqthread.h"

using namespace Threading;

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
    long				Tell();
    size_t				Read( void *dest, size_t count, size_t size);
    size_t				Write( const void *src, size_t count, size_t size);
    int					Error();
    int					Flush();

    char*				Gets( char *dest, int destSize );

	uint32				GetCRC32();

	long				GetSize();

	VirtStreamType_e	GetType() {return VS_TYPE_FILE;}

protected:
	FILE*				m_pFilePtr;
};

//------------------------------------------------------------------------------
// Virtual filesystem file of DarkTech Package file
//------------------------------------------------------------------------------

class CVirtualDPKFile : public IFile
{
	friend class CFileSystem;

public:
						CVirtualDPKFile(DPKFILE* pFile, CDPKFileReader* pPackage) : m_pFilePtr(pFile), m_pDKPReader(pPackage)
						{
						}

    int					Seek( long pos, VirtStreamSeek_e seekType );
    long				Tell();
    size_t				Read( void *dest, size_t count, size_t size);
    size_t				Write( const void *src, size_t count, size_t size);
    int					Error();
    int					Flush();

    char*				Gets( char *dest, int destSize );

	uint32				GetCRC32();

	long				GetSize();

	VirtStreamType_e	GetType() {return VS_TYPE_FILE_PACKAGE;}

protected:
	DPKFILE*			m_pFilePtr;
	CDPKFileReader*		m_pDKPReader;
};

//------------------------------------------------------------------------------
// Filesystem base
//------------------------------------------------------------------------------

class CFileSystem : public IFileSystem
{
	friend class CFile;
	friend class CVirtualDPKFile;

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
	const char*					GetBasePath() const				{return m_basePath.c_str();}

    // Returns current game path
    const char*					GetCurrentGameDirectory() const;

    // Returns current engine data path
    const char*					GetCurrentDataDirectory() const;

	// adds directory for file search
	void						AddSearchPath(const char* pathId, const char* pszDir);
	void						RemoveSearchPath(const char* pathId);

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

    // Package tools
    bool						AddPackage(const char* packageName,SearchPath_e type);

	// extracts single file from package
	void						ExtractFile(const char* filename, bool onlyNonExist = false);

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

	// This actually opens file
    IFile*						GetFileHandle(const char* file_name_to_check,const char* options, int searchFlags );

private:

	EqString					m_basePath;			// base prepended path

    EqString					m_dataDir;		// Used to load engine data

	struct SearchPath_t
	{
		EqString id;
		EqString path;
	};

	DkList<SearchPath_t>		m_directories;		// mod data, for fall back

    // Packages currently loaded
    DkList<CDPKFileReader*>		m_packages;
    DkList<IFile*>				m_openFiles;
	DkList<DKFINDDATA*>			m_findDatas;
	DkList<DKMODULE*>			m_modules;

    bool						m_editorMode;
	bool						m_isInit;

	CEqMutex					m_FSMutex;
};

#endif // CFILESYSTEM_H
