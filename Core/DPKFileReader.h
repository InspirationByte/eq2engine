///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#ifndef DPKFILEREADER_H
#define DPKFILEREADER_H

#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#endif

#include "utils/eqstring.h"
#include "utils/DkList.h"
#include "dpk_defs.h"

typedef int dpkhandle_t;

#define DPKX_MAX_HANDLES		32
#define DPK_HANDLE_INVALID		(-1)

struct DPKFILE
{
	FILE*			file;
	dpkfileinfo_t*	info;
	dpkhandle_t		handle;

	int				packageId;
};

enum PACKAGE_DUMP_MODE
{
	PACKAGE_INFO = 0,
	PACKAGE_FILES,
};

//------------------------------------------------------------------------------------------

class CDPKFileReader
{
public:
							CDPKFileReader();
							~CDPKFileReader();

	bool					SetPackageFilename( const char* filename );
	const char*				GetPackageFilename();

	int						GetSearchPath();
	void					SetSearchPath(int search);

	void					SetKey( const char* key );
	char*					GetKey();

	void					DumpPackage(PACKAGE_DUMP_MODE mode);

	int						FindFileIndex( const char* filename );

	// file data api
	DPKFILE*				Open( const char* filename, const char* mode );
	void					Close( DPKFILE* fp );
	long					Seek( DPKFILE* fp, long pos, int seekType );
	long					Tell( DPKFILE* fp );
	size_t					Read( void *dest, size_t count, size_t size, DPKFILE *fp );
	char*					Gets( char *dest, int destSize, DPKFILE *fp);
	int						Eof( DPKFILE* fp );

protected:
	dpkhandle_t				DecompressFile( int fileIndex );


	dpkheader_t				m_header;
	dpkfileinfo_t*			m_dpkFiles;

	dpkhandle_t				m_handles[DPKX_MAX_HANDLES];
	int						m_dumpCount;

	DkList<DPKFILE*>		m_openFiles;

	EqString				m_packageName;
	int						m_searchPath;
	EqString				m_tempPath;

	uint32					m_key[4];
};

#endif //DPK_FILE_READER_H
