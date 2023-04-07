//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: OS file find data
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(PLAT_LINUX)
struct __dirstream;
typedef struct __dirstream DIR;
#elif defined(PLAT_ANDROID)
struct DIR;
typedef struct DIR DIR;
#endif

class OSFindData
{
public:
	OSFindData() = default;
	~OSFindData();

	bool		Init(const EqString& searchWildcard);
	void		Release();

	bool		GetNext();
	const char* GetCurrentPath() const;
	bool		IsDirectory() const;

private:

	EqString			m_wildcard;
	int					m_searchPathId{ -1 };

#if defined(PLAT_WIN)
	WIN32_FIND_DATAA	m_wfd{ 0 };
	HANDLE				m_fileHandle{ INVALID_HANDLE_VALUE };
#else // assume POSIX by default
	EqString			m_dirPath;
	DIR*				m_dir{ nullptr };
	struct dirent*		m_entry{ nullptr };
	bool				m_isEntryDir{ false };
#endif
};