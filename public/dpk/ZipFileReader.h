///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "BasePackageFileReader.h"

class CZipFileReader;

class CZipFileStream : public CBasePackageFileStream
{
	friend class CZipFileReader;
	friend class CFileSystem;
public:
	CZipFileStream(const char* fileName, uintptr_t zf, CZipFileReader* host);
	~CZipFileStream();

	VSSize				Read(void *dest, VSSize count, VSSize size);
	VSSize				Write(const void *src, VSSize count, VSSize size);
	VSSize				Seek(int64 nOffset, EVirtStreamSeek seekType);
	void				Print(const char* fmt, ...);
	VSSize				Tell() const;
	VSSize				GetSize() { return static_cast<VSSize>(m_uncompressedSize); }
	bool				Flush() { return false; }

	EStreamType	GetType() const { return VS_TYPE_FILE_PACKAGE; }
	uint32				GetCRC32() { return m_crc; }

	const char*			GetName() const { return m_name; }

	CBasePackageReader* GetHostPackage() const;

protected:

	EqString			m_name;
	uintptr_t			m_zipHandle;
	int					m_crc;
	int					m_uncompressedSize;

	CZipFileReader*		m_host;
};

//----------------------------------------------------------------------

class CZipFileReader : public CBasePackageReader
{
public:
	CZipFileReader() = default;

	bool				InitPackage(const char* filename, const char* mountPath = nullptr);

	IFilePtr			Open(const char* filename, int modeFlags);
	IFilePtr			Open(int fileIndex, int modeFlags);
	bool				FileExists(const char* filename) const;
	int					FindFileIndex(const char* filename) const;

	EPackageReaderType	GetType() const { return PACKAGE_READER_ZIP; }

protected:
	uintptr_t			GetZippedFile(int nameHash) const;

	struct ZFileInfo
	{
		EqString		fileName;
		unsigned long	filePos[4]{ 0 };
	};

	Map<int, ZFileInfo>	m_files{ PP_SL };
};