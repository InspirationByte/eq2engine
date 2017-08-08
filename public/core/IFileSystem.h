//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Filesystem
//////////////////////////////////////////////////////////////////////////////////

#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include "InterfaceManager.h"
#include "IVirtualStream.h"

#include <sys/types.h>
#include <sys/stat.h>

#define FILESYSTEM_INTERFACE_VERSION		"CORE_Filesystem_005"

// Linux-only definition
#ifndef _WIN32
#   define MAX_PATH 260
#else
#undef FreeModule
#endif

//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

typedef enum
{
    SP_DATA = (1 << 1),
    SP_ROOT = (1 << 2),
    SP_MOD	= (1 << 3),
}SearchPath_e;

typedef IVirtualStream IFile; // pretty same
struct DKMODULE; // module structure
struct DKFINDDATA;

//------------------------------------------------------------------------------
// Filesystem interface
//------------------------------------------------------------------------------

class IFileSystem : public ICoreModuleInterface
{
public:
    // Initialization of filesystem
    virtual bool			Init(bool bEditorMode) = 0;
	virtual void			Shutdown() = 0;

	//------------------------------------------------------------
	// Directory stuff
	//------------------------------------------------------------

	// something like working directory
	virtual void			SetBasePath(const char* path) = 0;
	virtual const char*		GetBasePath() const = 0;

    // Returns current game path
    virtual const char*		GetCurrentGameDirectory() = 0;

    // Returns current engine data path
    virtual const char*		GetCurrentDataDirectory() = 0;

	// adds data directory for file search
	virtual void			AddSearchPath(const char* pathId, const char* pszDir) = 0;
	virtual void			RemoveSearchPath(const char* pathId) = 0;

    // Directory operations
    virtual void			MakeDir(const char* dirname, SearchPath_e search ) = 0;
    virtual void			RemoveDir(const char* dirname, SearchPath_e search ) = 0;

	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------

    virtual IFile*			Open( const char* filename, const char* options, int searchFlags = -1 ) = 0;
    virtual void			Close( IFile* pFile ) = 0;

	// other operations
	virtual bool			FileExist(const char* filename, int searchFlags = -1) = 0;
	virtual void			FileRemove(const char* filename, SearchPath_e search ) = 0;
	virtual bool			FileCopy(const char* filename, const char* dest_file, bool overWrite, SearchPath_e search) = 0;

	// The next ones are deprecated and will be removed

    virtual char*			GetFileBuffer(const char* filename,long *filesize = 0, int searchFlags = -1, bool useHunk = false) = 0;
    virtual long			GetFileSize(const char* filename, int searchFlags = -1) = 0;
	virtual uint32			GetFileCRC32(const char* filename, int searchFlags = -1) = 0;

    // Package tools
    virtual bool			AddPackage(const char* packageName,SearchPath_e type) = 0;

	// extracts a single file to it's directory (usable for dll's)
	virtual void			ExtractFile(const char* filename, bool onlyNonExist) = 0;

	//------------------------------------------------------------
	// Locator
	//------------------------------------------------------------

	// opens directory for search props
	virtual const char*		FindFirst(const char* wildcard, DKFINDDATA** findData, int searchPath) = 0;
	virtual const char*		FindNext(DKFINDDATA* findData) = 0;
	virtual void			FindClose(DKFINDDATA* findData) = 0;

	//------------------------------------------------------------
	// Dynamic library stuff
	//------------------------------------------------------------

	// loads module
	virtual DKMODULE*		LoadModule(const char* mod_name) = 0;

	// frees module
	virtual void			FreeModule( DKMODULE* pModule ) = 0;

	// returns procedure address of the loaded module
	virtual void*			GetProcedureAddress(DKMODULE* pModule, const char* pszProc) = 0;
};

INTERFACE_SINGLETON( IFileSystem, CFileSystem, FILESYSTEM_INTERFACE_VERSION, g_fileSystem )

#endif // IFILESYSTEM_H
