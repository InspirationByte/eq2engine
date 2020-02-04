///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#ifndef DPKFILEREADER_H
#define DPKFILEREADER_H

#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#endif

#include "utils/eqstring.h"
#include "utils/DkList.h"
#include "utils/eqthread.h"

#include "dpk_defs.h"

#include "IVirtualStream.h"

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

class CDPKFileStream : public IVirtualStream
{
	friend class CDPKFileReader;
	friend class CFileSystem;
public:
	CDPKFileStream(const dpkfileinfo_t& info, FILE* fp);
	~CDPKFileStream();

	// reads data from virtual stream
	size_t Read(void *dest, size_t count, size_t size);

	// writes data to virtual stream
	size_t Write(const void *src, size_t count, size_t size);

	// seeks pointer to position
	int	Seek(long nOffset, VirtStreamSeek_e seekType);

	// fprintf analog
	void Print(const char* fmt, ...);

	// returns current pointer position
	long Tell();

	// returns memory allocated for this stream
	long GetSize();

	// flushes stream from memory
	int	Flush();

	// returns stream type
	VirtStreamType_e GetType();

	// returns CRC32 checksum of stream
	uint32 GetCRC32();

protected:
	void				DecodeBlock(int block);

	ubyte				m_blockData[DPK_BLOCK_MAXSIZE];
	dpkfileinfo_t		m_info;
	dpkblock_t			m_blockInfo;

	FILE*				m_handle;
	int					m_curPos;

	CDPKFileReader*		m_host;
};

//------------------------------------------------------------------------------------------

class CDPKFileReader
{
public:
							CDPKFileReader(Threading::CEqMutex& fsMutex);
							~CDPKFileReader();

	bool					SetPackageFilename( const char* filename );
	const char*				GetPackageFilename() const;

	int						GetSearchPath() const;
	void					SetSearchPath(int search);

	void					SetKey( const char* key );
	char*					GetKey() const;

	void					DumpPackage(PACKAGE_DUMP_MODE mode);

	int						FindFileIndex( const char* filename ) const;

	// file data api
	CDPKFileStream*			Open( const char* filename, const char* mode );
	void					Close(CDPKFileStream* fp );

protected:
	dpkheader_t				m_header;
	dpkfileinfo_t*			m_dpkFiles;

	DkList<CDPKFileStream*>	m_openFiles;

	EqString				m_packageName;
	int						m_searchPath;
	EqString				m_tempPath;

	uint32					m_key[4];

	Threading::CEqMutex&	m_FSMutex;
};

#endif //DPK_FILE_READER_H
