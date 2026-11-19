//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: OS file find data
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <ctype.h>
#include <unistd.h> // rmdir()
#include <dirent.h> // opendir, readdir
#include <sys/stat.h>
#endif

#include "OSFindData.h"

#if !defined(PLAT_WIN)
// matches the entry name against a wildcard file name pattern;
// '*' matches any sequence of characters (including empty) and '?' matches any
// single character, case-insensitive - same as the Windows FindFirstFile behavior
static bool MatchWildcardName(const char* pattern, const char* name)
{
	const char* starPattern = nullptr;
	const char* starName = nullptr;

	while (*name != 0)
	{
		if (*pattern == '*')
		{
			// remember the star position and try to match the rest with an empty sequence
			starPattern = ++pattern;
			starName = name;
		}
		else if (*pattern == '?' || CType::LowerChar(*pattern) == CType::LowerChar(*name))
		{
			++pattern;
			++name;
		}
		else if (starPattern != nullptr)
		{
			// backtrack: let the last star consume one more character
			pattern = starPattern;
			name = ++starName;
		}
		else
			return false;
	}

	// trailing stars match an empty sequence
	while (*pattern == '*')
		++pattern;

	return *pattern == 0;
}
#endif

OSFindData::~OSFindData()
{
	Release();
}

void OSFindData::Release()
{
#if defined(PLAT_WIN)
	static_assert(sizeof(WIN32_FIND_DATAA) <= sizeof(EQWIN32_FIND_DATAA), "Fix EQWIN32_FIND_DATAA size!");

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
	m_wildcard = searchWildcard;

	Release();

#if defined(PLAT_WIN)
	m_fileHandle = FindFirstFileA(searchWildcard, (PWIN32_FIND_DATAA)&m_wfd);
	if (m_fileHandle != INVALID_HANDLE_VALUE)
		return true;
#elif defined(PLAT_POSIX)
	m_dirPath = fnmPathExtractPath(searchWildcard);
	m_dir = opendir(m_dirPath);
	if (m_dir)
		return GetNext();
#endif
	return false;
}

bool OSFindData::GetNext()
{
#if defined(PLAT_WIN)
	return FindNextFileA(m_fileHandle, (PWIN32_FIND_DATAA)&m_wfd);

#else
	do
	{
		m_entry = readdir(m_dir);

		if (!m_entry)
			break;

		if (*m_entry->d_name == 0)
			continue;

		// only the file name part of the wildcard is matched against the entry name
		const EqStringRef wildcardName = fnmPathExtractName(m_wildcard);
		if (MatchWildcardName(wildcardName, m_entry->d_name))
			break;
	} while (true);

	m_isEntryDir = false;

	struct stat st;
	if (m_entry && stat(EqString::Format("%s/%s", m_dirPath.TrimChar(CORRECT_PATH_SEPARATOR).ToCString(), m_entry->d_name), &st) == 0)
	{
		m_isEntryDir = (st.st_mode & S_IFDIR);
	}

	return m_entry;
#endif
}

const char* OSFindData::GetCurrentPath() const
{
#if defined(PLAT_WIN)
	return ((PWIN32_FIND_DATAA)&m_wfd)->cFileName;
#else
	return m_entry->d_name;
#endif
}

bool OSFindData::IsDirectory() const
{
#ifdef _WIN32
	return (((PWIN32_FIND_DATAA)&m_wfd)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	return m_isEntryDir;
#endif // _WIN32
}