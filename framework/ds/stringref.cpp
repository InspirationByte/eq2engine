//////////////////////////////////////////////////////////////////////////////////
// Copyright Š Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Special String tools to do lesser memory errors
//////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>

#include "core/core_common.h"
#include "stringref.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <locale.h>
#include <wctype.h>

static locale_t xgetlocale()
{
	// HACK: Assume the user's system language is the same to the game's language
	static locale_t loc = newlocale(LC_CTYPE_MASK, getenv("LANG"), LC_CTYPE);
	return loc;
}

#define _vsnwprintf			vswprintf
#define _snprintf			snprintf
#define stricmp(a, b)		strcasecmp(a, b)

#endif

//------------------------------------------

static char* x_strupr(char* str)
{
	char* it = str;

	while (*it != 0) { *it = toupper(*it); ++it; }

	return str;
}

static char* x_strlwr(char* str)
{
	char* it = str;

	while (*it != 0) { *it = tolower(*it); ++it; }

	return str;
}

static wchar_t* x_wcslwr(wchar_t* str)
{
	wchar_t* it = str;

#ifdef _WIN32
	while (*it != 0) { *it = *CharLowerW(&(*it)); ++it; }
#else
	while (*it != 0) { *it = towlower_l(*it, xgetlocale()); ++it; }
#endif // _WIN32

	return str;
}

static wchar_t* x_wcsupr(wchar_t* str)
{
	wchar_t* it = str;

#ifdef _WIN32
	while (*it != 0) { *it = *CharUpperW(&(*it)); ++it; }
#else
	while (*it != 0) { *it = towupper_l(*it, xgetlocale()); ++it; }
#endif // _WIN32

	return str;
}

// Finds a string in another string with a case insensitive test
static char* x_stristr( char* pStr, const char* pSearch )
{
	char* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower(*pMatch) != tolower(*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

//------------------------------------------------------
// wide string
//------------------------------------------------------

// compares two strings
static int x_wcscmp( const wchar_t *s1, const wchar_t *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

// compares two strings case-insensetive
static int x_wcsicmp( const wchar_t* s1, const wchar_t* s2 )
{
	while (1)
	{
		if (towlower(*s1) != towlower(*s2))
			return -1;              // strings not equal

		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

// finds substring in string case insensetive
static wchar_t* x_wcsistr(wchar_t* pStr, const wchar_t* pSearch )
{
	ASSERT(pStr);
	ASSERT(pSearch);

	if (!pStr || !pSearch)
		return 0;

	wchar_t* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			// Check for match
			const wchar_t* pMatch = pLetter + 1;
			const wchar_t* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (towlower(*pMatch) != towlower(*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

namespace CType
{
template<> bool IsAlphabetic(char chr) { return isalpha(static_cast<uint8>(chr)); }
template<> bool IsAlphaNumeric(char chr) { return isalnum(static_cast<uint8>(chr)); }
template<> bool IsDigit(char chr) { return isdigit(static_cast<uint8>(chr)); }
template<> bool IsSpace(char chr) { return isspace(static_cast<uint8>(chr)); }

template<> bool IsAlphabetic(wchar_t chr) { return iswalpha(chr); }
template<> bool IsAlphaNumeric(wchar_t chr) { return iswalnum(chr); }
template<> bool IsDigit(wchar_t chr) { return iswdigit(chr); }
template<> bool IsSpace(wchar_t chr) { return iswspace(chr); }

template<> char LowerChar(char chr) { return tolower(chr); }
template<> char UpperChar(char chr) { return toupper(chr); }

template<> wchar_t LowerChar(wchar_t chr)
{
#ifdef _WIN32
	wchar_t tmp[] = { chr, 0 };
	return *CharLowerW(tmp);
#else
	return towlower_l(chr, xgetlocale());
#endif // _WIN32
}

template<> wchar_t UpperChar(wchar_t chr)
{
#ifdef _WIN32
	wchar_t tmp[] = { chr, 0 };
	return *CharUpperW(tmp);
#else
	return towupper_l(chr, xgetlocale());
#endif // _WIN32
}
}

namespace CString
{
template<> int Length<char>(const char* str)
{
	ASSERT(str);
	return static_cast<int>(strlen(str));
}

template<> int Length<wchar_t>(const wchar_t* str)
{
	ASSERT(str);
	return static_cast<int>(wcslen(str));
}

template<> char* SubString(char* str, const char* search)
{
	ASSERT(str && search);
	return strstr(str, search);
}

template<> char* SubStringCaseIns(char* str, const char* search)
{
	ASSERT(str && search);
	return x_stristr(str, search);
}

template<> wchar_t* SubString(wchar_t* str, const wchar_t* search)
{
	ASSERT(str && search);
	return wcsstr(str, search);
}

template<> wchar_t* SubStringCaseIns(wchar_t* str, const wchar_t* search)
{
	ASSERT(str && search);
	return x_wcsistr(str, search);
}

template<> char* LowerCase(char* str)
{
	ASSERT(str);
	return x_strlwr(str);
}

template<> wchar_t* LowerCase(wchar_t* str)
{
	ASSERT(str);
	return x_wcslwr(str);
}

template<> char* UpperCase(char* str)
{
	ASSERT(str);
	return x_strupr(str);
}

template<> wchar_t* UpperCase(wchar_t* str)
{
	ASSERT(str);
	return x_wcsupr(str);
}

template<> int Compare(const char* strA, const char* strB)
{
	ASSERT(strA && strB);
	return strcmp(strA, strB);
}

template<> int Compare(const wchar_t* strA, const wchar_t* strB)
{
	ASSERT(strA && strB);
	return wcscmp(strA, strB);
}

template<> int CompareCaseIns(const char* strA, const char* strB)
{
	ASSERT(strA && strB);
	return stricmp(strA, strB);
}

template<> int CompareCaseIns(const wchar_t* strA, const wchar_t* strB)
{
	ASSERT(strA && strB);
	return x_wcsicmp(strA, strB);
}

template<> int PrintFV(char* buffer, int bufferCnt, const char* fmt, va_list argList)
{
	ASSERT(buffer && fmt);
	return vsnprintf(buffer, bufferCnt, fmt, argList);
}

template<> int PrintFV(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, va_list argList)
{
	ASSERT(buffer && fmt);
	return _vsnwprintf(buffer, bufferCnt, fmt, argList);
}

template<> int PrintF(char* buffer, int bufferCnt, const char* fmt, ...)
{
	ASSERT(buffer && fmt);
	va_list argptr;
	va_start(argptr, fmt);
	int result = PrintFV(buffer, bufferCnt, fmt, argptr);
	va_end(argptr);
	return result;
}

template<> int PrintF(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, ...)
{
	ASSERT(buffer && fmt);
	va_list argptr;
	va_start(argptr, fmt);
	int result = PrintFV(buffer, bufferCnt, fmt, argptr);
	va_end(argptr);
	return result;
}

//------------------------------------------
// Duplicates string
//------------------------------------------
char* DuplicateNew(const char* s)
{
	if (!s)
		return nullptr;

	const int len = static_cast<int>(strlen(s) + 1);
	char* t = PPNew char[len];
	strncpy(t, s, len);
	return t;
}

wchar_t* DuplicateNew(const wchar_t* s)
{
	if (!s)
		return nullptr;

	const int len = static_cast<int>(wcslen(s) + 1);
	wchar_t* t = PPNew wchar_t[len];
	wcsncpy(t, s, len);
	return t;
}

}

//------------------------------------------------

template<typename CH>
int EqTStrRef<CH>::Length() const
{
	if (!m_pszString)
		return 0;
	return (m_nLength == -1) ? m_nLength = CString::Length(m_pszString) : m_nLength; 
}

template<typename CH>
int EqTStrRef<CH>::Compare(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return -1;

	return CString::Compare(m_pszString, otherStr.ToCString());
}

template<typename CH>
int EqTStrRef<CH>::CompareCaseIns(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return -1;

	return CString::CompareCaseIns(m_pszString, otherStr.ToCString());
}

template<typename CH>
int EqTStrRef<CH>::GetMathingChars(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return 0;

	const CH* s1 = m_pszString;
	const CH* s2 = otherStr;

	int matching = 0;
	while (*s1++ == *s2++) { matching++; }

	return matching;
}

template<typename CH>
int EqTStrRef<CH>::GetMathingCharsCaseIns(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return 0;

	const CH* s1 = m_pszString;
	const CH* s2 = otherStr;

	int matching = 0;
	while (CType::LowerChar(*s1++) == CType::LowerChar(*s2++)) { matching++; }

	return matching;
}

template<typename CH>
int EqTStrRef<CH>::Find(EqTStrRef subStr, bool bCaseSensetive, int nStart) const
{
	if (!IsValid() || !subStr.IsValid() || nStart < 0)
		return -1;

	const CH* strStart = const_cast<CH*>(m_pszString) + min(nStart, Length());
	const CH* subStrPtr = bCaseSensetive ? CString::SubString(strStart, subStr.ToCString()) : CString::SubStringCaseIns(strStart, subStr.ToCString());
	if (!subStrPtr)
		return -1;

	return (subStrPtr - m_pszString);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::LowerCase() const
{
	EqTStr<CH> str(*this);
	CH* data = str.GetData();
	for (int i = 0; i < str.Length(); ++i)
		data[i] = CType::LowerChar(data[i]);

	return str;
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::UpperCase() const
{
	EqTStr<CH> str(*this);
	CH* data = str.GetData();
	for (int i = 0; i < str.Length(); ++i)
		data[i] = CType::UpperChar(data[i]);

	return str;
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::Left(int nCount) const
{
	return Mid(0, nCount);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::Right(int nCount) const
{
	if (nCount >= Length())
		return (*this);

	return Mid(Length() - nCount, nCount);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::Mid(int nStart, int nCount) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= Length());
	if (nStart < 0 || nStart + nCount > Length())
		return EqTStr<CH>::EmptyStr;

	return EqTStr<CH>(&m_pszString[nStart], nCount);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::EatWhiteSpaces() const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	EqTStr<CH> out;
	const CH* cc = m_pszString;
	while (cc)
	{
		if (!CType::IsSpace(*cc))
			out.Append(*cc++);
	}

	return out;
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::TrimSpaces(bool left, bool right) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	const CH* begin = m_pszString;

	// trim whitespace from left
	while (*begin && CType::IsSpace(*begin))
		begin++;

	if (*begin == 0)
		return EqTStr<CH>::EmptyStr;

	const CH* end = begin + CString::Length(begin) - 1;

	// trim whitespace from right
	while (end > begin && CType::IsSpace(*end))
		end--;

	return Mid(begin - m_pszString, end - begin + 1);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::TrimChar(const CH* ch, bool left, bool right) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	const CH* begin = m_pszString;

	auto ischr = [](const CH* ch, CH c) -> bool {
		while (*ch) { if (*ch++ == c) return true; }
		return false;
		};

	// trim whitespace from left
	while (*begin && ischr(ch, *begin))
		++begin;

	if (*begin == 0)
		return EqTStr<CH>::EmptyStr;

	const CH* end = begin + CString::Length(begin) - 1;

	// trim whitespace from right
	while (end > begin && ischr(ch, *end))
		--end;

	return Mid(begin - m_pszString, end - begin + 1);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::TrimChar(CH ch, bool left, bool right) const
{
	CH cch[2] = { ch, 0 };
	return TrimChar(cch, left, right);
}

// define implementations below
template class EqTStrRef<char>;
template class EqTStrRef<wchar_t>;