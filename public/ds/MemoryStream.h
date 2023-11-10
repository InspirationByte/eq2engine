//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	~CMemoryStream();

	CMemoryStream(PPSourceLine sl);
	CMemoryStream(ubyte* data, int nOpenFlags, int nDataSize, PPSourceLine sl);

	// opens stream, if this is a file, data is filename
	bool				Open(ubyte* data, int nOpenFlags, int nDataSize);

	// closes stream
	void				Close(bool deallocate = false);

	// reads data from virtual stream
	size_t				Read(void *dest, size_t count, size_t size);

	// writes data to virtual stream
	size_t				Write(const void *src, size_t count, size_t size);

	// seeks pointer to position
	int					Seek(int nOffset, EVirtStreamSeek seekType);

	// returns current pointer position
	int					Tell() const;

	// returns memory allocated for this stream
	int					GetSize();

	// flushes stream, doesn't affects on memory stream
	bool				Flush();

	// returns CRC32 checksum
	uint32				GetCRC32();

	VirtStreamType_e	GetType() const { return VS_TYPE_MEMORY; }

	const char*			GetName() const { return m_sl.GetFileName(); }

	// reads other stream into this one
	bool				AppendStream(IVirtualStream* pStream, int maxSize = 0);

	// writes constents of this stream into the other stream
	void				WriteToStream(IVirtualStream* pStream, int maxSize = 0);

	// resizes buffer to specified size (finalize buffer for reading)
	void				ShrinkBuffer(int size);

	// returns current pointer to the stream (only memory stream)
	ubyte*				GetCurrentPointer();

	// returns base pointer to the stream (only memory stream)
	ubyte*				GetBasePointer();

protected:

	// reallocates memory
	void				ReAllocate(int nNewSize);

private:
	PPSourceLine		m_sl;
	ubyte*				m_start{ nullptr };
	ubyte*				m_currentPtr{ nullptr };
	int					m_writeTop{ 0 };

	int					m_allocatedSize{ 0 };
	int					m_openFlags{ 0 };
	bool				m_ownBuffer{ false };
};