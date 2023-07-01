///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BasePackageFileReader.h"
#include "minizip/unzip.h"

class CZipFileReader;

class CZipFileStream : public CBasePackageFileStream
{
	friend class CZipFileReader;
	friend class CFileSystem;
public:
	CZipFileStream(unzFile zip, CZipFileReader* host);
	~CZipFileStream();

	void				Ref_DeleteObject();

	// reads data from virtual stream
	size_t				Read(void *dest, size_t count, size_t size);

	// writes data to virtual stream
	size_t				Write(const void *src, size_t count, size_t size);

	// seeks pointer to position
	int					Seek(long nOffset, EVirtStreamSeek seekType);

	// fprintf analog
	void				Print(const char* fmt, ...);

	// returns current pointer position
	long				Tell() const;

	// returns memory allocated for this stream
	long				GetSize();

	// flushes stream from memory
	bool				Flush();

	// returns stream type
	VirtStreamType_e	GetType() const { return VS_TYPE_FILE_PACKAGE; }

	// returns CRC32 checksum of stream
	uint32				GetCRC32();

	CBasePackageReader* GetHostPackage() const;

protected:

	unzFile				m_zipHandle;
	unz_file_info		m_finfo;

	CZipFileReader*		m_host;
};

//----------------------------------------------------------------------

class CZipFileReader : public CBasePackageReader
{
public:
	CZipFileReader();
	~CZipFileReader();

	bool					InitPackage(const char* filename, const char* mountPath = nullptr);

	IFilePtr				Open(const char* filename, int modeFlags);
	bool					FileExists(const char* filename) const;

protected:
	unzFile					GetNewZipHandle() const;
	unzFile					GetZippedFile(const char* filename) const;

	struct zfileinfo_t
	{
		EqString filename;
		unz_file_pos pos;
	};

	Map<int, zfileinfo_t>	m_files{ PP_SL };
};