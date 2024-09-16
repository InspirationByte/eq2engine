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
// Converts string to 24-bit integer hash
//------------------------------------------
int StringToHash(EqStringRef str, bool caseIns )
{
	ASSERT(str);
	int len = str.Length();
	const char* ptr = str.GetData();

	int hash = len;
	for (; len > 0; --len)
	{
		int v1 = hash >> 19;
		int v0 = hash << 5;

		int chr = caseIns ? CType::LowerChar(*ptr) : *ptr;

		hash = ((v0 | v1) + chr) & StringHashMask;
		++ptr;
	}

	return hash;
}

//------------------------------------------
// String split helper
//------------------------------------------

void StringSplit(const char* pString, ArrayCRef<const char*> separators, Array<EqString>& outStrings)
{
	if (!pString || *pString == 0)
		return;

	outStrings.clear();
	const char* pCurPos = pString;
	while (1)
	{
		int iFirstSeparator = -1;
		const char* pFirstSeparator = nullptr;
		for (int i = 0; i < separators.numElem(); i++)
		{
			const char* pTest = xstristr(pCurPos, separators[i]);
			if (pTest && (!pFirstSeparator || pTest < pFirstSeparator))
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if (pFirstSeparator)
		{
			// Split on this separator and continue on.
			const int separatorLen = strlen(separators[iFirstSeparator]);
			if (pFirstSeparator > pCurPos)
			{
				outStrings.append(_Es(pCurPos, pFirstSeparator - pCurPos));
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string
			if (strlen(pCurPos))
			{
				outStrings.append(_Es(pCurPos));
			}
			return;
		}
	}
}

void StringSplit(const char* pString, const char* separator, Array<EqString>& outStrings)
{
	StringSplit(pString, ArrayCRef(&separator, 1), outStrings);
}

//------------------------------------------

static char* xstrupr(char* str)
{
	ASSERT(str);
	char* it = str;

	while (*it != 0) { *it = toupper(*it); ++it; }

	return str;
}

static char* xstrlwr(char* str)
{
	ASSERT(str);
	char* it = str;

	while (*it != 0) { *it = tolower(*it); ++it; }

	return str;
}

static wchar_t* xwcslwr(wchar_t* str)
{
	ASSERT(str);

	wchar_t* it = str;

#ifdef _WIN32
	while (*it != 0) { *it = *CharLowerW(&(*it)); ++it; }
#else
	while (*it != 0) { *it = towlower_l(*it, xgetlocale()); ++it; }
#endif // _WIN32

	return str;
}

static wchar_t* xwcsupr(wchar_t* str)
{
	ASSERT(str);

	wchar_t* it = str;

#ifdef _WIN32
	while (*it != 0) { *it = *CharUpperW(&(*it)); ++it; }
#else
	while (*it != 0) { *it = towupper_l(*it, xgetlocale()); ++it; }
#endif // _WIN32

	return str;
}

static char* xstrstr(  const char* s1, const char* search )
{
	ASSERT( s1 );
	ASSERT( search );

	return strstr( (char* )s1, search );
}

// Finds a string in another string with a case insensitive test
static char const* xstristr( char const* pStr, char const* pSearch )
{
	ASSERT(pStr);
	ASSERT(pSearch);

	if (!pStr || !pSearch)
		return 0;

	char const* pLetter = pStr;

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

static char* xstristr( char* pStr, char const* pSearch )
{
	return (char*)xstristr( (char const*)pStr, pSearch );
}

//------------------------------------------------------
// wide string
//------------------------------------------------------

// compares two strings
static int xwcscmp( const wchar_t *s1, const wchar_t *s2)
{
	ASSERT( s1 );
	ASSERT( s2 );

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
static int xwcsicmp( const wchar_t* s1, const wchar_t* s2 )
{
	ASSERT( s1 );
	ASSERT( s2 );

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
static wchar_t const* xwcsistr( wchar_t const* pStr, wchar_t const* pSearch )
{
	ASSERT(pStr);
	ASSERT(pSearch);

	if (!pStr || !pSearch)
		return 0;

	wchar_t const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			// Check for match
			wchar_t const* pMatch = pLetter + 1;
			wchar_t const* pTest = pSearch + 1;
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

// finds substring in string case insensetive
static wchar_t* xwcsistr(wchar_t* pStr, wchar_t const* pSearch)
{
	ASSERT(pStr);
	ASSERT(pSearch);

	return (wchar_t*)xwcsistr((wchar_t const*)pStr, pSearch);
}

//------------------------------------------------------
// Path utils
//------------------------------------------------------

bool fnmPathHasExt(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
			return true;
	}
	return false;
}

EqString fnmPathApplyExt(EqStringRef path, EqStringRef ext)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
			return path.Left(i+1) + ext;
	}

	if (path.Length() > 0 && path[path.Length() - 1] == '.')
		return path + ext;
	return path + "." + ext;
}

EqString fnmPathStripExt(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
			return path.Left(i);
	}
	return path;
}

EqString fnmPathStripName(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == CORRECT_PATH_SEPARATOR || path[i] == INCORRECT_PATH_SEPARATOR)
			return path.Left(i + 1);
	}
	return path;
}

EqString fnmPathStripPath(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == CORRECT_PATH_SEPARATOR || path[i] == INCORRECT_PATH_SEPARATOR)
			return path.Right(path.Length() - 1 - i);
	}
	return path;
}

		 
EqString fnmPathExtractExt(EqStringRef path, bool autoLowerCase)
{
	EqString result;
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
		{
			result = path.Right(path.Length() - 1 - i);
			break;
		}
	}

	if (autoLowerCase)
	{
		char* data = result.GetData();
		for (int i = 0; i < result.Length(); ++i)
			data[i] = CType::LowerChar(data[i]);
	}
	return result;
}

EqString fnmPathExtractName(EqStringRef path)
{
	return fnmPathStripPath(path);
}

EqString fnmPathExtractPath(EqStringRef path)
{
	return fnmPathStripName(path);
}

void fnmPathCombineF(EqString& outPath, int num, ...)
{
	outPath.Empty();

	va_list	argptr;
	va_start(argptr, num);

	int maxLength = 0;
	FixedArray<EqStringRef, 32> paths;
	for (int i = 0; i < num; ++i)
	{
		EqStringRef pathPart = va_arg(argptr, const char*);
		const int length = pathPart.Length();
		if (!length)
			continue;
		paths.append(pathPart);
		maxLength += length + 1;
	}
	va_end(argptr);

	outPath.Resize(maxLength);
	for (int i = 0; i < paths.numElem(); ++i)
	{
		outPath.Append(paths[i]);
		if (i < paths.numElem()-1 && outPath[outPath.Length()-1] != CORRECT_PATH_SEPARATOR)
			outPath.Append(CORRECT_PATH_SEPARATOR);
	}
	fnmPathFixSeparators(outPath);
}

void fnmPathFixSeparators(EqString& str)
{
	const int length = str.Length();
	char* data = str.GetData();
	for (int i = 0; i < length; ++i)
	{
		if (data[i] == INCORRECT_PATH_SEPARATOR)
			data[i] = CORRECT_PATH_SEPARATOR;
	}
}

void fnmPathFixSeparators(char* str)
{
	if (!str)
		return;

	while (*str)
	{
		if (*str == INCORRECT_PATH_SEPARATOR)
			*str = CORRECT_PATH_SEPARATOR;
		str++;
	}
}

//------------------------------------------------------
// string conversion
//------------------------------------------------------
namespace EqStringConv
{
static uint32 GetUTF8NextByte(ubyte** utf8)
{
	if (!*(*utf8))
		return 0;

	return *(*utf8)++;
}

static uint32 udec(uint32 val)
{
	return (val & 0x3f);
}

static uint32 GetWideChar(ubyte** utf8)
{
	const uint32 b1 = GetUTF8NextByte(utf8);
	if (!b1)
		return 0;

	// Determine whether we are dealing
	// with a one-, two-, three-, or four-
	// byte sequence.
	if ((b1 & 0x80) == 0)
	{
		// 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
		return b1;
	}
	else if ((b1 & 0xe0) == 0xc0)
	{
		// 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
		uint32 r = (b1 & 0x1f) << 6;
		r |= udec(GetUTF8NextByte(utf8));
		return r;
	}
	else if ((b1 & 0xf0) == 0xe0)
	{
		// 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
		uint32 r = (b1 & 0x0f) << 12;
		r |= udec(GetUTF8NextByte(utf8)) << 6;
		r |= udec(GetUTF8NextByte(utf8));
		return r;
	}
	else if ((b1 & 0xf8) == 0xf0)
	{
		// 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx
		//     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
		// (uuuuu = wwww + 1)
		int b2 = udec(GetUTF8NextByte(utf8));
		int b3 = udec(GetUTF8NextByte(utf8));
		int b4 = udec(GetUTF8NextByte(utf8));
		return ((b1 & 7) << 18) | ((b2 & 0x3f) << 12) |
			((b3 & 0x3f) << 6) | (b4 & 0x3f);
	}

	//bad start for UTF-8 multi-byte sequence
	return '?';
}

static int GetUTF8Length(ubyte* utf8)
{
	int utfStringLength = 0;
	ubyte* tmp = utf8;
	while (GetWideChar(&tmp))
		++utfStringLength;

	return utfStringLength;
}

}

//--------------------------------------------------------------

AnsiUnicodeConverter::AnsiUnicodeConverter(EqWString& outStr, EqStringRef sourceStr)
{
	ASSERT(sourceStr.IsValid());

	ubyte* utf8 = (ubyte*)sourceStr.GetData();
	int length = EqStringConv::GetUTF8Length(utf8);

	outStr.ExtendAlloc(length);
	do {
		const uint32 wch = EqStringConv::GetWideChar(&utf8);
		if (!wch)
			break;
		outStr.Append(wch);
	} while (length--);
}

AnsiUnicodeConverter::AnsiUnicodeConverter(EqString& outStr, EqWStringRef sourceStr)
{
	ASSERT(sourceStr.IsValid());

	int len = sourceStr.Length() * 4;

	// to not call too many allocations
	outStr.ExtendAlloc(len);

	const wchar_t* val = sourceStr.GetData();
	uint32 code;
	do
	{
		code = *val++;

		if(code == 0)
			break;

		if (code <= 0x7F)
		{
			outStr.Append((char)code);
		}
		else if (code <= 0x7FF)
		{
			outStr.Append((code >> 6) + 192);
			outStr.Append((code & 63) + 128);
		}
		else if (code <= 0xFFFF)
		{
			outStr.Append((code >> 12) + 224);
			outStr.Append(((code >> 6) & 63) + 128);
			outStr.Append((code & 63) + 128);
		}
		else if (code <= 0x10FFFF)
		{
			outStr.Append((code >> 18) + 240);
			outStr.Append(((code >> 12) & 63) + 128);
			outStr.Append(((code >> 6) & 63) + 128);
			outStr.Append((code & 63) + 128);
		}
		else if (0xd800 <= code && code <= 0xdfff)
		{
			//invalid block of utf8
		}
	}
	while(len--);
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
	return *CharLowerW(&chr);
#else
	return towlower_l(chr, xgetlocale());
#endif // _WIN32
}

template<> wchar_t UpperChar(wchar_t chr)
{
#ifdef _WIN32
	return *CharUpperW(&chr);
#else
	return towupper_l(chr, xgetlocale());
#endif // _WIN32
}
}

namespace CString
{
template<> int Length<char>(const char* str)
{
	if (!str) return 0;
	return static_cast<int>(strlen(str));
}

template<> int Length<wchar_t>(const wchar_t* str)
{
	if (!str) return 0;
	return static_cast<int>(wcslen(str));
}

template<> char* SubString(char* str, const char* search)
{
	if (!str || !search) return nullptr;
	return strstr(str, search);
}

template<> char* SubStringCaseIns(char* str, const char* search)
{
	if (!str || !search) return nullptr;
	return xstristr(str, search);
}

template<> wchar_t* SubString(wchar_t* str, const wchar_t* search)
{
	if (!str || !search) return nullptr;
	return wcsstr(str, search);
}

template<> wchar_t* SubStringCaseIns(wchar_t* str, const wchar_t* search)
{
	if (!str || !search) return nullptr;
	return xwcsistr(str, search);
}

template<> char* LowerCase(char* str)
{
	return xstrlwr(str);
}

template<> wchar_t* LowerCase(wchar_t* str)
{
	return xwcslwr(str);
}

template<> char* UpperCase(char* str)
{
	return xstrupr(str);
}

template<> wchar_t* UpperCase(wchar_t* str)
{
	return xwcsupr(str);
}

template<> int Compare(const char* strA, const char* strB)
{
	return strcmp(strA, strB);
}

template<> int Compare(const wchar_t* strA, const wchar_t* strB)
{
	return wcscmp(strA, strB);
}

template<> int CompareCaseIns(const char* strA, const char* strB)
{
	return stricmp(strA, strB);
}

template<> int CompareCaseIns(const wchar_t* strA, const wchar_t* strB)
{
	return xwcsicmp(strA, strB);
}

template<> int PrintFV(char* buffer, int bufferCnt, const char* fmt, va_list argList)
{
	return vsnprintf(buffer, bufferCnt, fmt, argList);
}

template<> int PrintFV(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, va_list argList)
{
	return _vsnwprintf(buffer, bufferCnt, fmt, argList);
}

template<> int PrintF(char* buffer, int bufferCnt, const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	int result = PrintFV(buffer, bufferCnt, fmt, argptr);
	va_end(argptr);
	return result;
}

template<> int PrintF(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, ...)
{
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

	const int len = strlen(s) + 1;
	char* t = PPNew char[len];
	strncpy(t, s, len);
	return t;
}

wchar_t* DuplicateNew(const wchar_t* s)
{
	if (!s)
		return nullptr;

	const int len = wcslen(s) + 1;
	wchar_t* t = PPNew wchar_t[len];
	wcsncpy(t, s, len);
	return t;
}

}

//------------------------------------------------

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
	if (nCount >= m_nLength)
		return (*this);

	return Mid(m_nLength - nCount, nCount);
}

template<typename CH>
EqTStr<CH> EqTStrRef<CH>::Mid(int nStart, int nCount) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= m_nLength);
	if (nStart < 0 || nStart + nCount > m_nLength)
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