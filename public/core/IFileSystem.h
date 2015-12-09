//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Filesystem
//////////////////////////////////////////////////////////////////////////////////

#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include "core_base_header.h"
#include "IVirtualStream.h"

#include <sys/types.h>
#include <sys/stat.h>

#define FILESYSTEM_INTERFACE_VERSION		"CORE_Filesystem_004"

// Linux-only definition
#ifndef _WIN32
#   define MAX_PATH 260
#else
#undef FreeModule
#endif

typedef enum
{
    SP_DATA = (1 << 1),
    SP_ROOT = (1 << 2),
    SP_MOD	= (1 << 3),
}SearchPath_e;

struct DKMODULE; // module structure

//------------------------------------------------------------------------------
// File stream interface
//------------------------------------------------------------------------------

typedef IVirtualStream IFile; // pretty same

// for backwards compat
#define DKFILE IFile

//------------------------------------------------------------------------------
// Filesystem interface
//------------------------------------------------------------------------------

class IFileSystem
{
public:
    // Initialization of filesystem
    virtual bool		Init(bool bEditorMode) = 0;
	virtual void		Shutdown() = 0;

	//------------------------------------------------------------
	// Directory stuff
	//------------------------------------------------------------

    // Returns current game path
    virtual const char* GetCurrentGameDirectory() = 0;

    // Returns current engine data path
    virtual const char* GetCurrentDataDirectory() = 0;

	// adds directory for file search
	virtual void		AddSearchPath(const char* pszDir) = 0;

    // Directory operations
    virtual void		MakeDir(const char* dirname, SearchPath_e search ) = 0;
    virtual void		RemoveDir(const char* dirname, SearchPath_e search ) = 0;

	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------

    virtual IFile*		Open( const char* filename, const char* options, int searchFlags = -1 ) = 0;
    virtual void		Close( IFile* pFile ) = 0;

	// other operations
	virtual bool		FileExist(const char* filename, int searchFlags = -1) = 0;
	virtual void		RemoveFile(const char* filename, SearchPath_e search ) = 0;
	virtual bool		FileCopy(const char* filename, const char* dest_file, bool overWrite, SearchPath_e search) = 0;

	// The next ones are deprecated and will be removed

    virtual char*		GetFileBuffer(const char* filename,long *filesize = 0, int searchFlags = -1, bool useHunk = false) = 0;
    virtual long		GetFileSize(const char* filename, int searchFlags = -1) = 0;
	virtual uint32		GetFileCRC32(const char* filename, int searchFlags = -1) = 0;

    virtual int			Seek( DKFILE *fp, long pos, int seekType ) = 0;
    virtual long		Tell( DKFILE *fp ) = 0;
    virtual size_t		Read( void *dest, size_t count, size_t size, DKFILE *fp ) = 0;
    virtual size_t		Write( const void *src, size_t count, size_t size, DKFILE *fp ) = 0;
	virtual size_t		Printf( DKFILE *fp, const char *fmt, ... ) = 0;
    virtual int			Error( DKFILE *fp ) = 0;
    virtual int			Flush( DKFILE *fp ) = 0;
    virtual char*		Gets( char *dest, int destSize, DKFILE *fp ) = 0;

#ifdef _WIN32
    virtual int			Stat( const char *path, struct _stat *buf, int searchFlags = -1 ) = 0;
#else
    virtual int			Stat( const char *path, struct stat *buf, int searchFlags = -1 ) = 0;
#endif

#ifdef _WIN32
    virtual HANDLE		FindFirstFile(char *findname, WIN32_FIND_DATA *dat, int searchFlags = -1) = 0;
    virtual bool		FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat) = 0;
    virtual bool		FindClose(HANDLE handle) = 0;
#endif

    // Package tools
    virtual bool		AddPackage(const char* packageName,SearchPath_e type) = 0;

	// extracts a single file to it's directory (usable for dll's)
	virtual void		ExtractFile(const char* filename, bool onlyNonExist) = 0;

	//------------------------------------------------------------
	// Dynamic library stuff
	//------------------------------------------------------------

	// loads module
	virtual DKMODULE*	LoadModule(const char* mod_name) = 0;

	// frees module
	virtual void		FreeModule( DKMODULE* pModule ) = 0;

	// returns procedure address of the loaded module
	virtual void*		GetProcedureAddress(DKMODULE* pModule, const char* pszProc) = 0;

};

IEXPORTS IFileSystem*   GetFileSystem();

#endif // IFILESYSTEM_H