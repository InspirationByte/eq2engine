//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: OS file find data
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#ifndef PLAT_WIN
#include <unistd.h> // rmdir()
#include <dirent.h> // opendir, readdir
#endif

#include "OSFindData.h"

OSFindData::~OSFindData()
{
	Release();
}

void OSFindData::Release()
{
#if defined(PLAT_WIN)
	if (m_fileHandle != INVALID_HANDLE_VALUE)
		FindClose(m_fileHandle);
	ZeroMemory(&m_wfd, sizeof(m_wfd));
	m_fileHandle = INVALID_HANDLE_VALUE;
#else
	closedir(m_dir);
	m_dir = nullptr;
	m_entry = nullptr;
#endif
}

bool OSFindData::Init(const EqString& searchWildcard)
{
	Release();

#if defined(PLAT_WIN)
	m_fileHandle = FindFirstFileA(searchWildcard, &m_wfd);
	if (m_fileHandle != INVALID_HANDLE_VALUE)
		return true;
#elif defined(PLAT_POSIX)
	m_dirPath = searchWildcard.Path_Extract_Path();
	m_dir = opendir(m_dirPath);
	if (m_dir)
		return GetNext();
#endif
	return false;
}

bool OSFindData::GetNext()
{
#if defined(PLAT_WIN)
	return FindNextFileA(m_fileHandle, &m_wfd);

#else
	do
	{
		m_entry = readdir(m_dir);

		if (!m_entry)
			break;

		if (*m_entry->d_name == 0)
			continue;

		const int wildcardFileStart = m_wildcard.Find("*");
		if (wildcardFileStart != -1)
		{
			const char* wildcardFile = m_wildcard.ToCString() + m_wildcardFileStart + 1;
			if (*wildcardFile == 0)
				break;

			const char* found = xstristr(m_entry->d_name, wildcardFile);
			if (found && strlen(found) == strlen(wildcardFile))
				break;
		}
		else
			break;
	} while (true);

	m_isEntryDir = false;

	struct stat st;
	if (m_entry && stat(EqString::Format("%s/%s", m_dirPath.TrimChar(CORRECT_PATH_SEPARATOR).ToCString(), entry->d_name), &st) == 0)
	{
		m_isEntryDir = (st.st_mode & S_IFDIR);
	}

	return entry;
#endif
}

const char* OSFindData::GetCurrentPath() const
{
#if defined(PLAT_WIN)
	return m_wfd.cFileName;
#else
	return m_entry->d_name;
#endif
}

bool OSFindData::IsDirectory() const
{
#ifdef _WIN32
	return (m_wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	return m_isEntryDir;
#endif // _WIN32
}