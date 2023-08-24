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
void IVirtualStream::Print(const char* pFmt, ...)
{
	EqString str;
	va_list	argptr;

	va_start (argptr,pFmt);
	str = EqString::FormatVa(pFmt, argptr);
	va_end (argptr);

	Write(str.GetData(), 1, str.Length());
}

//--------------------------
// CMemoryStream - File stream
//--------------------------

CMemoryStream::CMemoryStream(PPSourceLine sl) : m_sl(sl)
{
}

CMemoryStream::CMemoryStream(ubyte* data, int nOpenFlags, int nDataSize, PPSourceLine sl)
	: m_sl(sl)
{
	Open(data, nOpenFlags, nDataSize);
}

// destroys stream data
CMemoryStream::~CMemoryStream()
{
	Close();
}

// reads data from virtual stream
size_t CMemoryStream::Read(void *dest, size_t count, size_t size)
{
	ASSERT(m_openFlags & VS_OPEN_READ);
	const long curPos = Tell();
	ASSERT_MSG(curPos + size * count <= m_allocatedSize, "Reading more than CMemoryStream has (expected capacity %d, has %d)", curPos + size * count, m_allocatedSize);

	const size_t readBytes = min(static_cast<size_t>(curPos + size * count), static_cast<size_t>(m_allocatedSize)) - curPos;

	memcpy(dest, m_currentPtr, readBytes);
	m_currentPtr += readBytes;

	return readBytes;
}

// writes data to virtual stream
size_t CMemoryStream::Write(const void *src, size_t count, size_t size)
{
	ASSERT(m_openFlags & VS_OPEN_WRITE);

	const long nAddBytes = size*count;
	const long nCurrPos = Tell();

	if(nCurrPos+nAddBytes > m_allocatedSize)
	{
		const long memDiff = (nCurrPos + nAddBytes) - m_allocatedSize;
		long newSize = m_allocatedSize + memDiff + VSTREAM_GRANULARITY - 1;
		newSize -= newSize % VSTREAM_GRANULARITY;

		ReAllocate( newSize );
	}

	// copy memory
	memcpy(m_currentPtr, src, nAddBytes);

	m_currentPtr += nAddBytes;
	m_writeTop = max(m_writeTop, m_currentPtr - m_start);

	return count;
}

// seeks pointer to position
int CMemoryStream::Seek(long nOffset, EVirtStreamSeek seekType)
{
	ASSERT(m_openFlags != 0);

	switch(seekType)
	{
		case VS_SEEK_SET:
			m_currentPtr = m_start + nOffset;
			break;
		case VS_SEEK_CUR:
			m_currentPtr = m_currentPtr + nOffset;
			break;
		case VS_SEEK_END:
			m_currentPtr = m_start + m_writeTop + nOffset;
			break;
	}

	m_writeTop = max(m_writeTop, m_currentPtr - m_start);

	return Tell();
}

// returns current pointer position
long CMemoryStream::Tell() const
{
	return m_currentPtr - m_start;
}

long CMemoryStream::GetSize()
{
	return m_writeTop;
}

// opens stream, if this is a file, data is filename
bool CMemoryStream::Open(ubyte* data, int nOpenFlags, int nDataSize)
{
	ASSERT(nDataSize >= 0);
	ASSERT_MSG(m_openFlags == 0, "Already open");

	m_openFlags = nOpenFlags;
	m_ownBuffer = (data == nullptr);
	m_writeTop = ((nOpenFlags & VS_OPEN_READ) && data) ? nDataSize : 0;

	if (m_ownBuffer)
	{
		ReAllocate(nDataSize);
	}
	else
	{
		m_start = m_currentPtr = data;
		m_allocatedSize = nDataSize;
	}

	return true;
}

// closes stream
void CMemoryStream::Close()
{
	if (m_ownBuffer)
		PPFree(m_start);
	m_allocatedSize = 0;
	m_writeTop = 0;
	m_start = nullptr;
	m_currentPtr = m_start;
	m_openFlags = 0;
	m_ownBuffer = false;
}

// flushes stream, doesn't affects on memory stream
bool CMemoryStream::Flush()
{
	return true;
}

// reallocates memory
void CMemoryStream::ReAllocate(long nNewSize)
{
	if(nNewSize == m_allocatedSize)
		return;

	// don't reallocate existing buffer
	if (!m_ownBuffer)
	{
		ASSERT_FAIL("CMemoryStream reallocate called when not owning buffer");
		return;
	}
	
	const long curPos = Tell();
	m_start = (ubyte*)PPDReAlloc(m_start, nNewSize, m_sl);
	ASSERT_MSG(m_start, "CMemoryStream reallocate failed!");

	m_allocatedSize = nNewSize;
	m_currentPtr = m_start + curPos;
}

// resizes buffer to specified size
void CMemoryStream::ShrinkBuffer(long size)
{
	if(size < m_allocatedSize)
	{
		ReAllocate(size);

		if (size < m_writeTop)
			m_writeTop = size;
	}
}

// writes constents of this stream into the other stream
void CMemoryStream::WriteToStream(IVirtualStream* pStream, int maxSize)
{
	pStream->Write(m_start, 1, min(maxSize > 0 ? maxSize : INT_MAX, m_writeTop));
}

// reads other stream into this one
bool CMemoryStream::AppendStream(IVirtualStream* pStream, int maxSize)
{
	ASSERT(m_openFlags & VS_OPEN_WRITE);

	const int resetPos = pStream->Tell();
	const int readSize = min(maxSize > 0 ? maxSize : INT_MAX, pStream->GetSize() - resetPos);
	
	m_writeTop = max(Tell() + readSize, m_writeTop);
	ReAllocate(m_writeTop + 16);

	// read to me
	pStream->Read(m_currentPtr, readSize, 1);
	pStream->Seek(resetPos, VS_SEEK_SET);
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
