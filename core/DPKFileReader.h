///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#ifndef DPKFILEREADER_H
#define DPKFILEREADER_H

#include "BasePackageFileReader.h"

#include "ds/DkList.h"
#include "utils/IceKey.h"

#include "core/dpk_defs.h"

#include <stdio.h>

typedef int dpkhandle_t;

#define DPKX_MAX_HANDLES		32
#define DPK_HANDLE_INVALID		(-1)

//------------------------------------------------------------------------------------------

class CDPKFileReader;

class CDPKFileStream : public CBasePackageFileStream
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
	long Tell() const;

	// returns memory allocated for this stream
	long GetSize();

	// flushes stream from memory
	int	Flush();

	// returns stream type
	VirtStreamType_e GetType() const { return VS_TYPE_FILE_PACKAGE; }

	// returns CRC32 checksum of stream
	uint32 GetCRC32();

	CBasePackageFileReader* GetHostPackage() const;

protected:
	void				DecodeBlock(int block);

	ubyte				m_blockData[DPK_BLOCK_MAXSIZE];
	dpkfileinfo_t		m_info;
	IceKey				m_ice;
	dpkblock_t			m_blockInfo;

	uint32				m_curBlockOfs;
	int					m_curBlockIdx;

	FILE*				m_handle;
	int					m_curPos;

	CDPKFileReader*		m_host;
};

//------------------------------------------------------------------------------------------

class CDPKFileReader : public CBasePackageFileReader
{
public:
	CDPKFileReader(Threading::CEqMutex& fsMutex);
	~CDPKFileReader();

	bool					InitPackage( const char* filename, const char* mountPath /*= nullptr*/);

	IVirtualStream*			Open( const char* filename, const char* mode );
	void					Close(IVirtualStream* fp );
	bool					FileExists(const char* filename) const;

protected:

	int						FindFileIndex(const char* filename) const;

	dpkheader_t				m_header;
	dpkfileinfo_t*			m_dpkFiles;

	DkList<CDPKFileStream*>	m_openFiles;
};

#endif //DPK_FILE_READER_H
