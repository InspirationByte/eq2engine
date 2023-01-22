///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "BasePackageFileReader.h"
#include "dpk/dpk_defs.h"
#include "utils/IceKey.h"

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
	CDPKFileStream(const char* fileName, const dpkfileinfo_t& info, COSFile&& osFile);
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
	bool Flush();

	// returns stream type
	VirtStreamType_e GetType() const { return VS_TYPE_FILE_PACKAGE; }

	// returns CRC32 checksum of stream
	uint32 GetCRC32();

	CBasePackageFileReader* GetHostPackage() const;

protected:
	void				DecodeBlock(int block);

	struct dpkblock_info_t
	{
		uint32 offset;
		uint32 size;
		uint32 compressedSize;
		short flags;
	};

	void*					m_blockData{ nullptr };
	void*					m_tmpDecompressData{ nullptr };
	//EqString				m_dbgFilename;

	dpkfileinfo_t			m_info;
	IceKey					m_ice;

	Array<dpkblock_info_t>	m_blockInfo{ PP_SL };
	int						m_curBlockIdx;

	COSFile					m_osFile;
	int						m_curPos;

	CDPKFileReader*			m_host;
};

//------------------------------------------------------------------------------------------

class CDPKFileReader : public CBasePackageFileReader
{
public:
	CDPKFileReader(Threading::CEqMutex& fsMutex);
	~CDPKFileReader();

	bool					InitPackage( const char* filename, const char* mountPath /*= nullptr*/);

	IVirtualStream*			Open( const char* filename, int modeFlags);
	void					Close(IVirtualStream* fp );
	bool					FileExists(const char* filename) const;

protected:

	int						FindFileIndex(const char* filename) const;

	dpkheader_t				m_header;
	dpkfileinfo_t*			m_dpkFiles;
	Map<int, int>			m_fileIndices{ PP_SL };

	Array<CDPKFileStream*>	m_openFiles{ PP_SL };
};