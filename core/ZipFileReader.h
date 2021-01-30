///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#ifndef ZIPFILEREADER_H
#define ZIPFILEREADER_H

#include "BasePackageFileReader.h"
#include "contrib/minizip/unzip.h"

#include "utils/DkList.h"
#include "utils/IVirtualStream.h"

class CZipFileReader;

class CZipFileStream : public CBasePackageFileStream
{
	friend class CZipFileReader;
	friend class CFileSystem;
public:
	CZipFileStream(unzFile zip);
	~CZipFileStream();

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

	CBasePackageFileReader* GetHostPackage() const;

protected:

	unzFile				m_zipHandle;

	CZipFileReader*		m_host;
};

//----------------------------------------------------------------------

struct zfileinfo_t
{
	EqString filename;
	int hash;
	unz_file_pos pos;
};

class CZipFileReader : public CBasePackageFileReader
{
public:
	CZipFileReader(Threading::CEqMutex& fsMutex);
	~CZipFileReader();

	bool					InitPackage(const char* filename, const char* mountPath = nullptr);

	IVirtualStream*			Open(const char* filename, const char* mode);
	void					Close(IVirtualStream* fp);
	bool					FileExists(const char* filename) const;

protected:
	unzFile					GetNewZipHandle() const;
	unzFile					GetZippedFile(const char* filename) const;

	DkList<CZipFileStream*>	m_openFiles;
	DkList<zfileinfo_t>		m_files;
};

#endif ZIPFILEREADER_H