///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "BasePackageFileReader.h"
#include "core/platform/OSFile.h"
#include "dpk/dpk_defs.h"
#include "utils/IceKey.h"

class CDPKFileReader;
class COSFile;

// data package file info
struct DPKFileHdr
{
	uint64	offset;
	uint32	size;				// The real file size
	uint32	crc;

	short	numBlocks;			// number of blocks
	short	flags;
};


class CDPKFileStream : public IPackFileStream
{
	friend class CDPKFileReader;
	friend class CFileSystem;
public:
	CDPKFileStream(const char* filename, const DPKFileHdr& info, COSFile&& osFile);
	~CDPKFileStream();

	// reads data from virtual stream
	VSSize				Read(void *dest, VSSize count, VSSize size);
	VSSize				Write(const void *src, VSSize count, VSSize size);
	VSSize				Seek(int64 nOffset, EFileStreamSeek seekType);

	VSSize				Tell() const;
	VSSize				GetSize();
	bool				Flush();

	// returns stream type
	EFileStreamType			GetType() const { return FS_TYPE_FILE_PACKAGE; }

	// returns CRC32 checksum of stream
	uint32				GetCRC32();

	const char*			GetName() const { return m_name; }

	CBasePackageReader* GetHostPackage() const;

protected:
	void				DecodeBlock(int block);

	struct BlockInfo;

	EqString			m_name;

	DPKFileHdr			m_info;
	IceKey				m_ice;
	COSFile				m_osFile;
	Array<BlockInfo>	m_blockInfo{ PP_SL };
	
	CDPKFileReader*		m_host{ nullptr };
	void*				m_blockData{ nullptr };
	void*				m_tmpDecompressData{ nullptr };

	int					m_curPos;
	int					m_curBlockIdx;
};

//------------------------------------------------------------------------------------------

class CDPKFileReader : public CBasePackageReader
{
public:
	static bool				CheckValidHeader(const dpkheader_t& header, const char* packageName);

	EPackageType			GetType() const { return PACKAGE_READER_DPK; }

	bool					InitPackage( const char* filename, const char* name /*= nullptr*/, const char* mountPath /*= nullptr*/);
	bool					OpenEmbeddedPackage(CBasePackageReader* target, const char* filename);

	IFileStreamPtr			Open(const char* filename, int modeFlags);
	IFileStreamPtr			Open(int fileIndex, int modeFlags);
	int						GetFileCount() const { return m_dpkFiles.numElem(); }

	bool					FileExist(const char* filename) const;
	int						FindFileIndex(const char* filename) const;

protected:
	bool					InitPackageInternal(COSFile& osFile, const VSSize startOffset, const dpkheader_t& header, const char* mountPath /*= nullptr*/);

	Array<DPKFileHdr>		m_dpkFiles{ PP_SL };
	Map<int, int>			m_fileIndices{ PP_SL };
	int						m_version{ 0 };
};