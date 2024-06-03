#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "core/core_common.h"
#include "OSFile.h"

COSFile::COSFile()
{
#ifdef _WIN32
	static_assert(sizeof(m_fp) >= sizeof(HANDLE));

	m_fp = INVALID_HANDLE_VALUE;
#else
	m_fp = 0;
#endif
}

COSFile::COSFile(COSFile&& r) noexcept
{
	m_fp = r.m_fp;
	m_flags = r.m_flags;
	r.m_flags = 0;
#ifdef _WIN32
	r.m_fp = INVALID_HANDLE_VALUE;
#else
	r.m_fp = 0;
#endif
}

COSFile::~COSFile()
{
	Close();
}

bool COSFile::Open(const char* fileName, int modeFlags)
{
#ifdef _WIN32
	if (m_fp != INVALID_HANDLE_VALUE)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	DWORD desiredAccess;
	DWORD creationDisposition;

	if ((modeFlags & (READ | WRITE)) == (READ | WRITE))
	{
		desiredAccess = GENERIC_READ | GENERIC_WRITE;

		if (modeFlags & OPEN_EXIST)
			creationDisposition = OPEN_EXISTING;
		else
			creationDisposition = OPEN_ALWAYS;
	}
	else if (modeFlags & WRITE)
	{
		desiredAccess = GENERIC_WRITE;

		if (modeFlags & OPEN_EXIST)
			creationDisposition = OPEN_EXISTING;
		else if (modeFlags & APPEND)
			creationDisposition = OPEN_ALWAYS;
		else
			creationDisposition = CREATE_ALWAYS;
	}
	else
	{
		desiredAccess = GENERIC_READ;
		creationDisposition = OPEN_EXISTING;
	}
	m_fp = CreateFileA(fileName, desiredAccess, FILE_SHARE_READ, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (m_fp == INVALID_HANDLE_VALUE)
		return false;

	if (modeFlags & APPEND)
	{
		if (SetFilePointer(m_fp, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER)
		{
			DWORD lastError = GetLastError();
			CloseHandle((HANDLE)m_fp);
			m_fp = INVALID_HANDLE_VALUE;
			SetLastError(lastError);
			return false;
		}
	}
#else
	if (m_fp)
	{
		errno = EINVAL;
		return false;
	}

	int oflags;
	if ((modeFlags & (READ | WRITE)) == (READ | WRITE))
	{
		if (modeFlags & OPEN_EXIST)
			oflags = O_RDWR; // fail if not exists, rw mode
		else
			oflags = O_CREAT | O_RDWR; // create if not exists, rw mode
	}
	else if (modeFlags & WRITE)
	{
		if (modeFlags & OPEN_EXIST)
			oflags = O_WRONLY; // fail if not exists, write mode
		else if (modeFlags & APPEND)
			oflags = O_CREAT | O_WRONLY; // create if not exists, write mode
		else
			oflags = O_CREAT | O_TRUNC | O_WRONLY; // create if not exists, truncate if exist, write mode
	}
	else
		oflags = O_RDONLY; // do not create if not exists, read mode

	m_fp = (void*)(intptr_t)::open(fileName, oflags | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if ((int)(intptr_t)m_fp == -1)
	{
		m_fp = 0;
		return false;
	}

	if (modeFlags & APPEND)
	{
		if (lseek((int)(intptr_t)m_fp, 0, SEEK_END) == -1)
		{
			int lastError = errno;
			::close((int)(intptr_t)m_fp);
			m_fp = 0;
			errno = lastError;
			return false;
		}
	}
#endif
	m_flags = modeFlags;
	return true;
}

void COSFile::Close()
{
#ifdef _WIN32
	if (m_fp != INVALID_HANDLE_VALUE)
	{
		CloseHandle((HANDLE)m_fp);
		m_fp = INVALID_HANDLE_VALUE;
	}
#else
	if (m_fp)
	{
		::close((int)(intptr_t)m_fp);
		m_fp = 0;
	}
#endif
}

bool COSFile::IsOpen() const
{
#ifdef _WIN32
	return m_fp != INVALID_HANDLE_VALUE;
#else
	return m_fp != 0;
#endif
}

size_t COSFile::Read(void* buffer, int64 count)
{
	if (count <= 0)
		return 0;

#ifdef _WIN32
	ubyte* bufferStart = (ubyte*)buffer;
	DWORD i;
	while (count > (int64)INT_MAX)
	{
		if (!ReadFile((HANDLE)m_fp, buffer, INT_MAX, &i, nullptr))
			return -1;
		buffer = (ubyte*)buffer + i;
		if (i != INT_MAX)
			return (ubyte*)buffer - bufferStart;
		count -= INT_MAX;
	}

	if (!ReadFile((HANDLE)m_fp, buffer, count, &i, nullptr))
		return -1;

	buffer = (ubyte*)buffer + i;
	return (ubyte*)buffer - bufferStart;
#else
	return ::read((int)(intptr_t)m_fp, buffer, count);
#endif
}

size_t COSFile::Write(const void* buffer, int64 count)
{
	if (count <= 0)
		return 0;

	ASSERT_MSG(m_flags & (WRITE | APPEND), "COSFile - file was not open for read/append!");

#ifdef _WIN32
	const ubyte* bufferStart = (const ubyte*)buffer;
	DWORD i;
	while (count > (int64)INT_MAX)
	{
		if (!WriteFile((HANDLE)m_fp, buffer, INT_MAX, &i, nullptr))
			return -1;
		buffer = (const ubyte*)buffer + i;
		if (i != INT_MAX)
			return (const ubyte*)buffer - bufferStart;
		count -= INT_MAX;
	}

	if (!WriteFile((HANDLE)m_fp, buffer, count, &i, nullptr))
		return -1;

	buffer = (const ubyte*)buffer + i;
	return (const ubyte*)buffer - bufferStart;
#else
	return ::write((int)(intptr_t)m_fp, buffer, count);
#endif
}

size_t COSFile::Seek(int64 offset, ESeekPos pos)
{
#ifdef _WIN32
	DWORD moveMethod[3] = { 
		FILE_BEGIN,
		FILE_CURRENT, 
		FILE_END
	};
#ifndef _AMD64
	LARGE_INTEGER li;
	li.QuadPart = offset;
	li.LowPart = SetFilePointer((HANDLE)m_fp, li.LowPart, &li.HighPart, moveMethod[static_cast<int>(pos)]);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return -1;

	return li.QuadPart;
#else
	return SetFilePointer((HANDLE)m_fp, offset, nullptr, moveMethod[static_cast<int>(pos)]);
#endif
#else
	int whence[3] = {
		SEEK_SET,
		SEEK_CUR, 
		SEEK_END
	};
	return lseek64((int)(intptr_t)m_fp, offset, whence[static_cast<int>(pos)]);
#endif
}

size_t COSFile::Tell() const
{
#ifdef _WIN32
	LARGE_INTEGER li;
	li.QuadPart = 0;
	li.LowPart = SetFilePointer((HANDLE)m_fp, li.LowPart, &li.HighPart, FILE_CURRENT);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return -1;

	return li.QuadPart;
#else
	return lseek64((int)(intptr_t)m_fp, 0, SEEK_CUR);
#endif
}

bool COSFile::Flush()
{
#ifdef _WIN32
	return FlushFileBuffers((HANDLE)m_fp) != FALSE;
#else
	return fsync((int)(intptr_t)m_fp) == 0;
#endif
}
