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
	CMemoryStream(ubyte* data, int nOpenFlags, VSSize nDataSize, PPSourceLine sl);

	// opens stream, if this is a file, data is filename
	bool			Open(ubyte* data, int nOpenFlags, VSSize nDataSize);

	// closes stream
	void			Close(bool deallocate = false);

	VSSize			Read(void *dest, VSSize count, VSSize size);
	VSSize			Write(const void *src, VSSize count, VSSize size);
	VSSize			Seek(int64 nOffset, EVirtStreamSeek seekType);

	VSSize			Tell() const;
	VSSize			GetSize();

	bool			Flush();
	uint32			GetCRC32();

	EStreamType		GetType() const { return VS_TYPE_MEMORY; }
	const char*		GetName() const { return m_sl.GetFileName(); }

	// reads other stream into this one
	bool			AppendStream(IVirtualStream* pStream, VSSize maxSize = 0);

	// writes constents of this stream into the other stream
	void			WriteToStream(IVirtualStream* pStream, VSSize maxSize = 0);

	// resizes buffer to specified size (finalize buffer for reading)
	void			ShrinkBuffer(VSSize size);

	// returns current pointer to the stream (only memory stream)
	ubyte*			GetCurrentPointer();

	// returns base pointer to the stream (only memory stream)
	ubyte*			GetBasePointer();

protected:

	// reallocates memory
	void			ReAllocate(int nNewSize);

private:
	PPSourceLine	m_sl;
	ubyte*			m_start{ nullptr };
	ubyte*			m_currentPtr{ nullptr };
	VSSize			m_writeTop{ 0 };

	VSSize			m_allocatedSize{ 0 };
	int				m_openFlags{ 0 };
	bool			m_ownBuffer{ false };
};