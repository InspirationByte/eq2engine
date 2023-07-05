//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Virtual Stream class
//				For easy writing and reading, just like files
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/refcounted.h"

enum VirtStreamType_e
{
	VS_TYPE_MEMORY = 0,
	VS_TYPE_FILE,
	VS_TYPE_FILE_PACKAGE,
};

// fancy check
#define IsFileType(type) ((type) >= VS_TYPE_FILE)

enum EVirtStreamSeek
{
	VS_SEEK_SET = 0,	// set current position
	VS_SEEK_CUR,		// seek from last position
	VS_SEEK_END,		// seek to the end
};

enum EVirtStreamOpenFlags
{
	VS_OPEN_READ	= (1 << 0),
	VS_OPEN_WRITE	= (1 << 1),
	VS_OPEN_APPEND	= (1 << 2),
};

//--------------------------
// IVirtualStream - Virtual stream interface
//--------------------------

class IVirtualStream;
using IVirtualStreamPtr = CRefPtr<IVirtualStream>;

template<typename T>
static size_t VSRead(IVirtualStream* stream, T& obj);

template<typename T>
static size_t VSWrite(IVirtualStream* stream, const T& obj);

class IVirtualStream : public RefCountedObject<IVirtualStream>
{
public:
	virtual ~IVirtualStream() {}

	// reads data from virtual stream
	virtual size_t				Read( void *dest, size_t count, size_t size) = 0;

	// writes data to virtual stream
	virtual size_t				Write(const void *src, size_t count, size_t size) = 0;

	template <typename T>
	size_t						Read(T& obj) { return VSRead(this, obj); }

	template <typename T>
	size_t						Read(T* obj, size_t count = 1) { size_t readcnt = 0; while (count--) readcnt += VSRead(this, *obj++); return readcnt; }

	template <typename T>
	size_t						Write(const T& obj) { return VSWrite(this, obj); }

	template <typename T>
	size_t						Write(const T* obj, size_t count = 1) { size_t written = 0; while (count--) written += VSWrite(this, *obj++); return written; }

	// seeks pointer to position
	virtual int					Seek( long nOffset, EVirtStreamSeek seekType) = 0;

	// fprintf analog
	virtual	void				Print(const char* fmt, ...);

	// returns current pointer position
	virtual long				Tell() const = 0;

	// returns memory allocated for this stream
	virtual long				GetSize() = 0;

	// flushes stream from memory
	virtual bool				Flush() = 0;

	// returns stream type
	virtual VirtStreamType_e	GetType() const = 0;

	// returns CRC32 checksum of stream
	virtual uint32				GetCRC32() = 0;
};

// provide default implementation

template<typename T>
static size_t VSRead(IVirtualStream* stream, T& obj)
{
	return stream->Read(&obj, 1, sizeof(T));
}

template<typename T>
static size_t VSWrite(IVirtualStream* stream, const T& obj)
{
	return stream->Write(&obj, 1, sizeof(T));
}