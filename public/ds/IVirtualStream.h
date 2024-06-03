//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Virtual Stream class
//				For easy writing and reading, just like files
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/refcounted.h"

using VSSize = int64;

class IVirtualStream;

enum EStreamType : int
{
	VS_TYPE_MEMORY = 0,
	VS_TYPE_FILE,
	VS_TYPE_FILE_PACKAGE,
};

enum EVirtStreamSeek : int
{
	VS_SEEK_SET = 0,	// set current position
	VS_SEEK_CUR,		// seek from last position
	VS_SEEK_END,		// seek to the end
};

enum EVirtStreamOpenFlags : int
{
	VS_OPEN_READ	= (1 << 0),
	VS_OPEN_WRITE	= (1 << 1),
	VS_OPEN_APPEND	= (1 << 2),
};

template<typename T>
static VSSize VSRead(IVirtualStream* stream, T& obj);

template<typename T>
static VSSize VSWrite(IVirtualStream* stream, const T& obj);

//--------------------------
// IVirtualStream - data stream interface
//--------------------------

class IVirtualStream : public RefCountedObject<IVirtualStream>
{
public:
	// reads data from virtual stream
	virtual VSSize		Read(void *dest, VSSize count, VSSize size) = 0;

	// writes data to virtual stream
	virtual VSSize		Write(const void *src, VSSize count, VSSize size) = 0;

	template <typename T>
	VSSize				Read(T& obj) { return VSRead(this, obj); }

	template <typename T>
	VSSize				Read(T* obj, VSSize count = 1) { VSSize readcnt = 0; while (count--) readcnt += VSRead(this, *obj++); return readcnt; }

	template <typename T>
	VSSize				Write(const T& obj) { return VSWrite(this, obj); }

	template <typename T>
	VSSize				Write(const T* obj, VSSize count = 1) { size_t written = 0; while (count--) written += VSWrite(this, *obj++); return written; }

	// seeks pointer to position
	virtual VSSize		Seek(int64 offset, EVirtStreamSeek seekType) = 0;

	// fprintf analog
	virtual	void		Print(const char* fmt, ...);

	// returns current pointer position
	virtual VSSize		Tell() const = 0;

	// returns memory allocated for this stream
	virtual VSSize		GetSize() = 0;

	// flushes stream from memory
	virtual bool		Flush() = 0;

	// returns stream type
	virtual EStreamType	GetType() const = 0;

	// returns CRC32 checksum of stream
	virtual uint32		GetCRC32() = 0;

	// returns name of stream or file
	virtual const char*	GetName() const = 0;
};

using IVirtualStreamPtr = CRefPtr<IVirtualStream>;

// provide default implementation

template<typename T>
static VSSize VSRead(IVirtualStream* stream, T& obj)
{
	return stream->Read(&obj, 1, sizeof(T));
}

template<typename T>
static VSSize VSWrite(IVirtualStream* stream, const T& obj)
{
	return stream->Write(&obj, 1, sizeof(T));
}


template<typename T, VSSize N>
static VSSize VSWrite(IVirtualStream* stream, T(&obj)[N])
{
	return stream->Write(&obj, N, sizeof(T));
}