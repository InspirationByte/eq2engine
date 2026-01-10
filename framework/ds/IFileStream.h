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

class IFileStream;
using IFileStreamPtr = CRefPtr<IFileStream>;

enum EFileStreamType : int
{
	FS_TYPE_MEMORY = 0,
	FS_TYPE_FILE,
	FS_TYPE_FILE_PACKAGE,
};

enum EFileStreamSeek : int
{
	FS_SEEK_SET = 0,	// set current position
	FS_SEEK_CUR,		// seek from last position
	FS_SEEK_END,		// seek to the end
};

enum EFileOpenFlags : int
{
	FS_OPEN_READ	= (1 << 0),
	FS_OPEN_WRITE	= (1 << 1),
	FS_OPEN_APPEND	= (1 << 2),
};

template<typename T>
static VSSize VSRead(IFileStream* stream, T& obj);

template<typename T>
static VSSize VSWrite(IFileStream* stream, const T& obj);

//--------------------------
// IFileStream - data stream interface
//--------------------------

class IFileStream : public RefCountedObject<IFileStream>
{
public:
	// reads data from virtual stream
	virtual VSSize		Read(void *dest, VSSize count, VSSize size) = 0;

	// writes data to virtual stream
	virtual VSSize		Write(const void *src, VSSize count, VSSize size) = 0;

	template <typename T>
	VSSize				ReadObj(T& obj) { return VSRead(this, obj); }

	template <typename T>
	VSSize				ReadArray(T* obj, VSSize count = 1) { VSSize readcnt = 0; while (count--) readcnt += VSRead(this, *obj++); return readcnt; }

	template <typename T>
	VSSize				WriteObj(const T& obj) { return VSWrite(this, obj); }

	template <typename T>
	VSSize				WriteArray(const T* obj, VSSize count) { size_t written = 0; while (count--) written += VSWrite(this, *obj++); return written; }

	template <typename TArray>
	VSSize				WriteArray(const TArray& arr) { return WriteArray(arr.ptr(), arr.numElem()); }

	// seeks pointer to position
	virtual VSSize		Seek(int64 offset, EFileStreamSeek seekType) = 0;

	// fprintf analog
	void				PrintF(const char* fmt, ...);

	template <typename... Args>
	void				Print(const char* pszFormat, Args&&... args);

	// returns current pointer position
	virtual VSSize		Tell() const = 0;

	// returns memory allocated for this stream
	virtual VSSize		GetSize() = 0;

	// flushes stream from memory
	virtual bool		Flush() = 0;

	// returns stream type
	virtual EFileStreamType	GetType() const = 0;

	// returns CRC32 checksum of stream
	virtual uint32		GetCRC32() = 0;

	// returns name of stream or file
	virtual const char*	GetName() const = 0;
};

// provide default implementation

template<typename T>
static VSSize VSRead(IFileStream* stream, T& obj)
{
	return stream->Read(&obj, 1, sizeof(T));
}

template<typename T>
static VSSize VSWrite(IFileStream* stream, const T& obj)
{
	return stream->Write(&obj, 1, sizeof(T));
}


template<typename T, VSSize N>
static VSSize VSWrite(IFileStream* stream, T(&obj)[N])
{
	return stream->Write(&obj, N, sizeof(T));
}

template <typename T>
decltype(auto) ToCString(const T& value);

template <typename... Args>
inline void IFileStream::Print(const char* pszFormat, Args&&... args)
{
	return PrintF(pszFormat, ToCString(std::forward<Args>(args))...);
}
