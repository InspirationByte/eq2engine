//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Memory Stream
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "MemoryStream.h"
#include "utils/CRC32.h"

#ifdef _MSC_VER
#pragma warning(disable: 4267)
#endif

#define VSTREAM_GRANULARITY 1024 * 4	// 4kb

// prints string to stream
void IFileStream::PrintF(const char* pFmt, ...)
{
	EqString str;
	va_list	argptr;

	va_start (argptr,pFmt);
	str = EqString::FormatV(pFmt, argptr);
	va_end (argptr);

	Write(str.GetData(), 1, str.Length());
}

//--------------------------
// CMemoryStream - File stream
//--------------------------

CMemoryStream::CMemoryStream(CMemoryStream&& other)
	 : m_sl(other.m_sl)
	, m_start(other.m_start)
	, m_currentPtr(other.m_currentPtr)
	, m_writeTop(other.m_writeTop)
	, m_allocatedSize(other.m_allocatedSize)
	, m_openFlags(other.m_openFlags)
	, m_ownBuffer(other.m_ownBuffer)
{
	other.m_start = nullptr;
	other.m_currentPtr = nullptr;
	other.m_writeTop = 0;
	other.m_allocatedSize = 0;
	other.m_openFlags = 0;
	other.m_ownBuffer = false;
}


CMemoryStream& CMemoryStream::operator=(CMemoryStream&& other)
{
	m_sl = other.m_sl;
	m_start = other.m_start;
	m_currentPtr = other.m_currentPtr;
	m_writeTop = other.m_writeTop;
	m_allocatedSize = other.m_allocatedSize;
	m_openFlags = other.m_openFlags;
	m_ownBuffer = other.m_ownBuffer;
	other.m_start = nullptr;
	other.m_currentPtr = nullptr;
	other.m_writeTop = 0;
	other.m_allocatedSize = 0;
	other.m_openFlags = 0;
	other.m_ownBuffer = false;
	return *this;
}

CMemoryStream::CMemoryStream(PPSourceLine sl)
	: m_sl(sl)
{
}

CMemoryStream::CMemoryStream(ubyte* data, int openFlags, VSSize dataSize, PPSourceLine sl)
	: m_sl(sl)
{
	Open(data, openFlags, dataSize);
}

CMemoryStream::CMemoryStream(const ubyte* data, VSSize dataSize, PPSourceLine sl)
	: m_sl(sl)
{
	Open(data, dataSize);
}

// destroys stream data
CMemoryStream::~CMemoryStream()
{
	Close(true);
}

// reads data from virtual stream
VSSize CMemoryStream::Read(void *dest, VSSize count, VSSize size)
{
	ASSERT(m_openFlags & FS_OPEN_READ);

	const VSSize numBytesToRead = size * count;
	if (numBytesToRead <= 0)
		return 0;

	const int curPos = Tell();
	ASSERT_MSG(curPos + numBytesToRead <= m_allocatedSize, "Reading more than CMemoryStream has (expected capacity %d, has %d)", curPos + numBytesToRead, m_allocatedSize);

	const VSSize readBytes = min(static_cast<VSSize>(curPos + numBytesToRead), static_cast<VSSize>(m_allocatedSize)) - curPos;

	memcpy(dest, m_currentPtr, readBytes);
	m_currentPtr += readBytes;

	return readBytes / size;
}

// writes data to virtual stream
VSSize CMemoryStream::Write(const void *src, VSSize count, VSSize size)
{
	ASSERT(m_openFlags & FS_OPEN_WRITE);

	const VSSize numBytesToWrite = size * count;
	if (numBytesToWrite <= 0)
		return 0;

	const VSSize currentPos = Tell();

	if(currentPos + numBytesToWrite > m_allocatedSize)
	{
		const int64 memDiff = (currentPos + numBytesToWrite) - m_allocatedSize;
		ASSERT(memDiff >= 0);

		VSSize newSize = m_allocatedSize + memDiff + VSTREAM_GRANULARITY - 1;
		newSize -= newSize % VSTREAM_GRANULARITY;

		ReAllocate( newSize );
	}

	// copy memory
	memcpy(m_currentPtr, src, numBytesToWrite);
	m_currentPtr += numBytesToWrite;
	
	const int64 newSize = m_currentPtr - m_start;
	ASSERT(newSize >= 0);

	m_writeTop = max(m_writeTop, newSize);
	return count;
}

// seeks pointer to position
VSSize CMemoryStream::Seek(int64 nOffset, EFileStreamSeek seekType)
{
	ASSERT(m_openFlags != 0);

	switch(seekType)
	{
		case FS_SEEK_SET:
			m_currentPtr = m_start + nOffset;
			break;
		case FS_SEEK_CUR:
			m_currentPtr = m_currentPtr + nOffset;
			break;
		case FS_SEEK_END:
			m_currentPtr = m_start + m_writeTop + nOffset;
			break;
	}

	m_writeTop = max(m_writeTop, m_currentPtr - m_start);

	return Tell();
}

// returns current pointer position
VSSize CMemoryStream::Tell() const
{
	return m_currentPtr - m_start;
}

VSSize CMemoryStream::GetSize()
{
	return m_writeTop;
}

// opens stream, if this is a file, data is filename
bool CMemoryStream::Open(ubyte* data, int openFlags, VSSize dataSize)
{
	ASSERT(dataSize >= 0);
	ASSERT_MSG(m_openFlags == 0, "Already open");

	if (m_ownBuffer && data != nullptr)
		Close(true);

	m_ownBuffer = (data == nullptr);
	m_openFlags = openFlags;
	m_writeTop = ((openFlags & FS_OPEN_READ) && data) ? dataSize : 0;

	if (m_ownBuffer)
	{
		ReAllocate(dataSize);
	}
	else
	{
		m_start = m_currentPtr = data;
		m_allocatedSize = dataSize;
	}

	return true;
}

bool CMemoryStream::Open(const ubyte* data, VSSize dataSize)
{
	if (!data || !dataSize)
		return false;

	ASSERT(dataSize >= 0);
	ASSERT_MSG(m_openFlags == 0, "Already open");

	if (m_ownBuffer)
		Close(true);

	m_ownBuffer = false;
	m_openFlags = FS_OPEN_READ;
	m_writeTop = dataSize;

	m_start = m_currentPtr = const_cast<ubyte*>(data);
	m_allocatedSize = dataSize;

	return true;
}

// closes stream
void CMemoryStream::Close(bool deallocate)
{
	if(deallocate)
	{
		if (m_ownBuffer)
			PPFree(m_start);
		m_start = nullptr;
		m_allocatedSize = 0;
		m_ownBuffer = false;
	}
	m_currentPtr = m_start;
	m_writeTop = 0;
	m_openFlags = 0;
}

// flushes stream, doesn't affects on memory stream
bool CMemoryStream::Flush()
{
	return true;
}

// reallocates memory
void CMemoryStream::ReAllocate(int nNewSize)
{
	if(nNewSize == m_allocatedSize)
		return;

	// don't reallocate existing buffer
	if (!m_ownBuffer)
	{
		ASSERT_FAIL("CMemoryStream reallocate called when not owning buffer");
		return;
	}
	
	const int curPos = Tell();
	m_start = (ubyte*)PPDReAlloc(m_start, nNewSize, m_sl);
	ASSERT_MSG(m_start, "CMemoryStream reallocate failed!");

	m_allocatedSize = nNewSize;
	m_currentPtr = m_start + curPos;
}

// resizes buffer to specified size
void CMemoryStream::ShrinkBuffer(VSSize size)
{
	if(size < m_allocatedSize)
	{
		ReAllocate(size);

		if (size < m_writeTop)
			m_writeTop = size;
	}
}

// writes constents of this stream into the other stream
void CMemoryStream::WriteToStream(IFileStream* pStream, VSSize maxSize)
{
	pStream->Write(m_start, 1, min(maxSize > 0 ? maxSize : INT_MAX, m_writeTop));
}

// reads other stream into this one
bool CMemoryStream::AppendStream(IFileStream* pStream, VSSize maxSize)
{
	ASSERT(m_openFlags & FS_OPEN_WRITE);

	const VSSize resetPos = pStream->Tell();
	const VSSize readSize = min(maxSize > 0 ? maxSize : INT_MAX, pStream->GetSize() - resetPos);
	
	m_writeTop = max(Tell() + readSize, m_writeTop);
	ReAllocate(m_writeTop + 16);

	// read to me
	pStream->Read(m_currentPtr, readSize, 1);
	pStream->Seek(resetPos, FS_SEEK_SET);
	m_currentPtr += readSize;

	// let user seek this stream after
	return true;
}

// returns current pointer to the stream (only memory stream)
ubyte* CMemoryStream::GetCurrentPointer()
{
	return m_currentPtr;
}

// returns base pointer to the stream (only memory stream)
ubyte* CMemoryStream::GetBasePointer()
{
	return m_start;
}

uint32 CMemoryStream::GetCRC32()
{
	uint32 nCRC = CRC32_BlockChecksum( m_start, m_writeTop );

	return nCRC;
}
