//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Virtual Stream implementation classes
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IVirtualStream.h"

//--------------------------
// CMemoryStream - File stream
//--------------------------

class CMemoryStream : public IVirtualStream
{
public:
						CMemoryStream();
						CMemoryStream(ubyte* data, int nOpenFlags, int nDataSize);

						~CMemoryStream();

	// opens stream, if this is a file, data is filename
	bool				Open(ubyte* data, int nOpenFlags, int nDataSize);
	// closes stream
	void				Close();

	// reads data from virtual stream
	size_t				Read(void *dest, size_t count, size_t size);

	// writes data to virtual stream
	size_t				Write(const void *src, size_t count, size_t size);

	// seeks pointer to position
	int					Seek(long nOffset, VirtStreamSeek_e seekType);

	// returns current pointer position
	long				Tell() const;

	// returns memory allocated for this stream
	long				GetSize();

	// flushes stream, doesn't affects on memory stream
	bool				Flush();

	char*				Gets( char *dest, int destSize );

	// returns CRC32 checksum
	uint32				GetCRC32();

	VirtStreamType_e	GetType() const { return VS_TYPE_MEMORY; }

	// reads file to this stream
	bool				ReadFromFileStream(IVirtualStream* pFile);

	// saves stream to file for stream (only for memory stream )
	void				WriteToFileStream(IVirtualStream* pFile);

	// returns current pointer to the stream (only memory stream)
	ubyte*				GetCurrentPointer();

	// returns base pointer to the stream (only memory stream)
	ubyte*				GetBasePointer();

protected:

	// reallocates memory
	void				ReAllocate(long nNewSize);

private:

	ubyte*				m_start{ nullptr };
	ubyte*				m_currentPtr{ nullptr };

	long				m_allocatedSize{ 0 };
	long				m_openFlags{ 0 };
	bool				m_ownBuffer{ false };
};