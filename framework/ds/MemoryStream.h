//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Virtual Stream implementation classes
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IFileStream.h"

//--------------------------
// CMemoryStream - File stream
//--------------------------

class CMemoryStream : public IFileStream
{
public:
	~CMemoryStream();

	CMemoryStream(CMemoryStream&& other);
	CMemoryStream(const CMemoryStream& other) = delete;
	CMemoryStream(PPSourceLine sl = PP_SL);

	CMemoryStream&	operator=(CMemoryStream&& other);
	CMemoryStream&	operator=(const CMemoryStream& other) = delete;

	// opens stream, if this is a file, data is filename
	bool			Open(int openFlags, ubyte* data = nullptr, VSSize dataSize = 0);
	bool			Open(const ubyte* data, VSSize dataSize);						// read-only

	// closes stream
	void			Close(bool deallocate = false);

	VSSize			Read(void *dest, VSSize count, VSSize size);
	VSSize			Write(const void *src, VSSize count, VSSize size);
	VSSize			Seek(int64 offset, EFileStreamSeek seekType);

	VSSize			Tell() const;
	VSSize			GetSize();

	bool			Flush();
	uint32			GetCRC32();

	EFileStreamType	GetType() const { return FS_TYPE_MEMORY; }
	const char*		GetName() const { return m_sl.GetFileName(); }

	// reads other stream into this one
	bool			AppendStream(IFileStream* pStream, VSSize maxSize = 0);

	// writes constents of this stream into the other stream
	void			WriteToStream(IFileStream* pStream, VSSize maxSize = 0);

	// resizes buffer to specified size (finalize buffer for reading)
	void			ShrinkBuffer(VSSize size);

	// returns current pointer to the stream (only memory stream)
	ubyte*			GetCurrentPointer() { return m_currentPtr; }
	const ubyte*	GetCurrentPointer() const { return m_currentPtr; }

	// returns base pointer to the stream (only memory stream)
	ubyte*			GetBasePointer() { return m_start; }
	const ubyte*	GetBasePointer() const { return m_start; }

	bool			IsValid() const { return m_start != nullptr; }

protected:

	// reallocates memory
	void			ReAllocate(VSSize newSize);

private:
	PPSourceLine	m_sl;
	ubyte*			m_start{ nullptr };
	ubyte*			m_currentPtr{ nullptr };
	VSSize			m_writeTop{ 0 };

	VSSize			m_allocatedSize{ 0 };
	int				m_openFlags{ 0 };
	bool			m_ownBuffer{ false };
};