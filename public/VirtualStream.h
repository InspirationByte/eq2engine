//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Virtual Stream implementation classes
//////////////////////////////////////////////////////////////////////////////////

#ifndef VIRTUALSTREAM_H
#define VIRTUALSTREAM_H

#include "IVirtualStream.h"
#include "IFileSystem.h"
#include "DebugInterface.h"

//--------------------------
// CMemoryStream - File stream
//--------------------------

enum MemStreamOpenFlags_e
{
	VS_OPEN_MEMORY_FROM_EXISTING = (1 << 4),	// use existing allocated buffer
};

class CMemoryStream : public IVirtualStream
{
public:
						CMemoryStream();
						~CMemoryStream();

	// reads data from virtual stream
	size_t				Read(void *dest, size_t count, size_t size);

	// writes data to virtual stream
	size_t				Write(const void *src, size_t count, size_t size);

	// seeks pointer to position
	int					Seek(long nOffset, VirtStreamSeek_e seekType);

	// returns current pointer position
	long				Tell();

	// returns memory allocated for this stream
	long				GetSize();

	// opens stream, if this is a file, data is filename
	bool				Open(ubyte* data, int nOpenFlags, int nDataSize);

	// closes stream
	void				Close();

	// flushes stream, doesn't affects on memory stream
	int					Flush();

	char*				Gets( char *dest, int destSize );

	// returns CRC32 checksum
	uint32				GetCRC32();

	VirtStreamType_e	GetType() { return VS_TYPE_MEMORY; }

	// reads file to this stream
	bool				ReadFromFileStream( IFile* pFile );

	// saves stream to file for stream (only for memory stream )
	void				WriteToFileStream( IFile* pFile );

	// returns current pointer to the stream (only memory stream)
	ubyte*				GetCurrentPointer();

	// returns base pointer to the stream (only memory stream)
	ubyte*				GetBasePointer();

protected:

	// reallocates memory
	void				ReAllocate(long nNewSize);

private:

	ubyte*				m_pStart;
	ubyte*				m_pCurrent;

	long				m_nAllocatedSize;
	long				m_nUsageFlags;
};

IVirtualStream* OpenMemoryStream(int nOpenFlags, int nBufferSize, ubyte* pBufferData = NULL);

#endif // VIRTUALSTREAM_H
