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

class CDPKFileStream : public CBasePackageFileStream
{
	friend class CDPKFileReader;
	friend class CFileSystem;
public:
	CDPKFileStream(const char* filename, const dpkfileinfo_t& info, COSFile&& osFile);
	~CDPKFileStream();

	// reads data from virtual stream
	VSSize				Read(void *dest, VSSize count, VSSize size);
	VSSize				Write(const void *src, VSSize count, VSSize size);
	VSSize				Seek(int64 nOffset, EVirtStreamSeek seekType);
	void				Print(const char* fmt, ...);

	VSSize				Tell() const;
	VSSize				GetSize();
	bool				Flush();

	// returns stream type
	EStreamType			GetType() const { return VS_TYPE_FILE_PACKAGE; }

	// returns CRC32 checksum of stream
	uint32				GetCRC32();

	const char*			GetName() const { return m_name; }

	CBasePackageReader* GetHostPackage() const;

protected:
	void				DecodeBlock(int block);

	struct BlockInfo;

	EqString			m_name;

	dpkfileinfo_t		m_info;
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
	CDPKFileReader();
	~CDPKFileReader();

	bool					InitPackage( const char* filename, const char* mountPath /*= nullptr*/);
	bool					OpenEmbeddedPackage(CBasePackageReader* target, const char* filename);

	EPackageReaderType		GetType() const { return PACKAGE_READER_DPK; }

	IFilePtr				Open(const char* filename, int modeFlags);
	IFilePtr				Open(int fileIndex, int modeFlags);
	bool					FileExists(const char* filename) const;
	int						FindFileIndex(const char* filename) const;

protected:
	bool					InitPackage(COSFile& osFile, const char* mountPath /*= nullptr*/);

	Array<dpkfileinfo_t>	m_dpkFiles{ PP_SL };
	Map<int, int>			m_fileIndices{ PP_SL };
	int						m_version{ 0 };
};