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
		const wint_t c1 = towlower(*s1);
		const wint_t c2 = towlower(*s2);
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;	// strings not equal

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
		if (towlower(*pLetter) == towlower(*pSearch))
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

template<> char LowerChar(char chr) { return tolower(static_cast<uint8>(chr)); }
template<> char UpperChar(char chr) { return toupper(static_cast<uint8>(chr)); }

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
int Length(const char* str)
{
	ASSERT(str);
	return static_cast<int>(strlen(str));
}

int Length(const wchar_t* str)
{
	ASSERT(str);
	return static_cast<int>(wcslen(str));
}

char* SubString(char* str, const char* search)
{
	ASSERT(str && search);
	return strstr(str, search);
}

char* SubStringCaseIns(char* str, const char* search)
{
	ASSERT(str && search);
	return x_stristr(str, search);
}

wchar_t* SubString(wchar_t* str, const wchar_t* search)
{
	ASSERT(str && search);
	return wcsstr(str, search);
}

wchar_t* SubStringCaseIns(wchar_t* str, const wchar_t* search)
{
	ASSERT(str && search);
	return x_wcsistr(str, search);
}

char* LowerCase(char* str)
{
	ASSERT(str);
	return x_strlwr(str);
}

wchar_t* LowerCase(wchar_t* str)
{
	ASSERT(str);
	return x_wcslwr(str);
}

char* UpperCase(char* str)
{
	ASSERT(str);
	return x_strupr(str);
}

wchar_t* UpperCase(wchar_t* str)
{
	ASSERT(str);
	return x_wcsupr(str);
}

int Compare(const char* strA, const char* strB)
{
	ASSERT(strA && strB);
	if(strA == strB) return 0;	// what if?
	return strcmp(strA, strB);
}

int Compare(const wchar_t* strA, const wchar_t* strB)
{
	ASSERT(strA && strB);
	if(strA == strB) return 0;	// what if?
	return wcscmp(strA, strB);
}

int CompareCaseIns(const char* strA, const char* strB)
{
	ASSERT(strA && strB);
	if(strA == strB) return 0;	// what if?
	return stricmp(strA, strB);
}

int CompareCaseIns(const wchar_t* strA, const wchar_t* strB)
{
	ASSERT(strA && strB);
	if(strA == strB) return 0;	// what if?
	return x_wcsicmp(strA, strB);
}

// compares strings of known length, strings are not required to be null-terminated
template<typename CH, bool CASE_INS>
static int CompareLen(const CH* strA, int lenA, const CH* strB, int lenB)
{
	using UCH = std::make_unsigned_t<CH>;

	ASSERT(lenA >= 0 && lenB >= 0);
	ASSERT((strA || lenA == 0) && (strB || lenB == 0));

	const int len = min(lenA, lenB);
	if (strA != strB)
	{
		for (int i = 0; i < len; ++i)
		{
			const UCH a = CASE_INS ? CType::LowerChar(strA[i]) : strA[i];
			const UCH b = CASE_INS ? CType::LowerChar(strB[i]) : strB[i];
			if (a != b)
				return a < b ? -1 : 1;
		}
	}

	if (lenA == lenB)
		return 0;
	return lenA < lenB ? -1 : 1;
}

int Compare(const char* strA, int lenA, const char* strB, int lenB)
{
	return CompareLen<char, false>(strA, lenA, strB, lenB);
}

int Compare(const wchar_t* strA, int lenA, const wchar_t* strB, int lenB)
{
	return CompareLen<wchar_t, false>(strA, lenA, strB, lenB);
}

int CompareCaseIns(const char* strA, int lenA, const char* strB, int lenB)
{
	return CompareLen<char, true>(strA, lenA, strB, lenB);
}

int CompareCaseIns(const wchar_t* strA, int lenA, const wchar_t* strB, int lenB)
{
	return CompareLen<wchar_t, true>(strA, lenA, strB, lenB);
}

int PrintFV(char* buffer, int bufferCnt, const char* fmt, va_list argList)
{
	ASSERT(buffer && fmt);
	return vsnprintf(buffer, bufferCnt, fmt, argList);
}

// unlike vsnprintf, wide printf returns -1 when output is truncated,
// so required length is computed separately to match narrow PrintFV behaviour
int PrintFV(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, va_list argList)
{
	ASSERT(buffer && fmt);

	va_list argCopy;
	va_copy(argCopy, argList);
	int result = _vsnwprintf(buffer, bufferCnt, fmt, argList);

	if (result < 0 || result >= bufferCnt)
	{
		if (bufferCnt > 0)
			buffer[bufferCnt - 1] = 0;

#ifdef _WIN32
		if (result < 0)
			result = _vscwprintf(fmt, argCopy);
#else
		// no way to query the length, print into growing scratch buffer until it fits
		constexpr int MAX_PRINT_LENGTH = 16 * 1024 * 1024;
		for (int scratchCnt = max(bufferCnt * 2, 256); result < 0 && scratchCnt <= MAX_PRINT_LENGTH; scratchCnt *= 2)
		{
			wchar_t* scratch = PPAllocStructArray(wchar_t, scratchCnt);
			va_list scratchArgs;
			va_copy(scratchArgs, argCopy);
			result = vswprintf(scratch, scratchCnt, fmt, scratchArgs);
			va_end(scratchArgs);
			PPFree(scratch);
		}
#endif
	}

	va_end(argCopy);
	return result;
}

int PrintF(char* buffer, int bufferCnt, const char* fmt, ...)
{
	ASSERT(buffer && fmt);
	va_list argptr;
	va_start(argptr, fmt);
	int result = PrintFV(buffer, bufferCnt, fmt, argptr);
	va_end(argptr);
	return result;
}

int PrintF(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, ...)
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
EqTStr<CH>& EqTStrRef<CH>::GetTempString(const CH* str, int len)
{
	static constexpr int MAX_TEMP_STRINGS = 512;

	static thread_local int tempStrIdx = 0;
	static thread_local EqTStr<CH> tmpStr[MAX_TEMP_STRINGS];

	const int nextStr = tempStrIdx;
	tempStrIdx = (tempStrIdx + 1) % MAX_TEMP_STRINGS;

	if (str && len)
		tmpStr[nextStr].Assign(str, len);
	else
		tmpStr[nextStr].Empty();

	return tmpStr[nextStr];
}

template<typename CH>
EqTStr<CH>& EqTStrRef<CH>::GetTempString(const EqTStrRef<CH>& str, int len)
{
	return GetTempString(str.ToCString(), len != -1 ? len : str.Length());
}

template<typename CH>
int EqTStrRef<CH>::Length() const
{
	if (!m_data)
		return 0;
	return (m_length == -1) ? m_length = CString::Length(m_data) : m_length; 
}

template<typename CH>
int EqTStrRef<CH>::Compare(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return (IsValid() == otherStr.IsValid()) ? 0 : -1;

	return CString::Compare(m_data, Length(), otherStr.m_data, otherStr.Length());
}

template<typename CH>
int EqTStrRef<CH>::CompareCaseIns(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return (IsValid() == otherStr.IsValid()) ? 0 : -1;

	return CString::CompareCaseIns(m_data, Length(), otherStr.m_data, otherStr.Length());
}

template<typename CH>
int EqTStrRef<CH>::GetMathingChars(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return 0;

	const CH* s1 = m_data;
	const CH* s2 = otherStr;

	int matching = 0;
	while (*s1 && *s1 == *s2) { ++s1; ++s2; ++matching; }

	return matching;
}

template<typename CH>
int EqTStrRef<CH>::GetMathingCharsCaseIns(EqTStrRef otherStr) const
{
	if (!IsValid() || !otherStr.IsValid())
		return 0;

	const CH* s1 = m_data;
	const CH* s2 = otherStr;

	int matching = 0;
	while (*s1 && CType::LowerChar(*s1) == CType::LowerChar(*s2)) { ++s1; ++s2; ++matching; }

	return matching;
}

template<typename CH>
int EqTStrRef<CH>::Find(EqTStrRef subStr, bool bCaseSensetive, int nStart) const
{
	if (!IsValid() || !subStr.IsValid() || nStart < 0)
		return -1;

	// search only within known lengths, both strings may be not null-terminated
	const int length = Length();
	const int subLength = subStr.Length();
	for (int i = nStart; i + subLength <= length; ++i)
	{
		const int cmp = bCaseSensetive
			? CString::Compare(m_data + i, subLength, subStr.m_data, subLength)
			: CString::CompareCaseIns(m_data + i, subLength, subStr.m_data, subLength);
		if (cmp == 0)
			return i;
	}

	return -1;
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::LowerCase() const
{
	EqTStr<CH>& str = GetTempString(*this);
	CH* data = str.GetData();
	for (int i = 0; i < str.Length(); ++i)
		data[i] = CType::LowerChar(data[i]);

	return str;
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::UpperCase() const
{
	EqTStr<CH>& str = GetTempString(*this);
	CH* data = str.GetData();
	for (int i = 0; i < str.Length(); ++i)
		data[i] = CType::UpperChar(data[i]);

	return str;
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::Left(int nCount) const
{
	return Mid(0, min(nCount, Length()));
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::Right(int nCount) const
{
	if (nCount >= Length())
		return (*this);

	return Mid(Length() - nCount, nCount);
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::Mid(int nStart, int nCount) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= Length());
	if (nStart < 0 || nStart + nCount > Length())
		return EqTStr<CH>::EmptyStr;

	return GetTempString(&m_data[nStart], nCount);
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::EatWhiteSpaces() const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	EqTStr<CH>& out = GetTempString(nullptr, 0);
	for (const CH* cc = m_data; *cc; ++cc)
	{
		if (!CType::IsSpace(*cc))
			out.Append(*cc);
	}

	return out;
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::TrimSpaces(bool left, bool right) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	const CH* begin = m_data;

	// trim whitespace from left
	while (left && *begin && CType::IsSpace(*begin))
		begin++;

	if (*begin == 0)
		return EqTStr<CH>::EmptyStr;

	const CH* end = m_data + Length();

	// trim whitespace from right
	while (right && end > begin && CType::IsSpace(end[-1]))
		end--;

	return Mid(begin - m_data, end - begin);
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::TrimChar(const CH* ch, bool left, bool right) const
{
	if (!IsValid())
		return EqTStr<CH>::EmptyStr;

	const CH* begin = m_data;

	auto ischr = [](const CH* ch, CH c) -> bool {
		while (*ch) { if (*ch++ == c) return true; }
		return false;
	};

	// trim whitespace from left
	while (left && *begin && ischr(ch, *begin))
		++begin;

	if (*begin == 0)
		return EqTStr<CH>::EmptyStr;

	const CH* end = m_data + Length();

	// trim whitespace from right
	while (right && end > begin && ischr(ch, end[-1]))
		--end;

	return Mid(begin - m_data, end - begin);
}

template<typename CH>
EqTStrRef<CH> EqTStrRef<CH>::TrimChar(CH ch, bool left, bool right) const
{
	CH cch[2] = { ch, 0 };
	return TrimChar(cch, left, right);
}

template<typename CH>
CH EqTStrRef<CH>::operator[](int idx) const
{
	ASSERT(idx >= 0 && idx <= Length());
	return m_data[idx];
}

// define implementations below
template class EqTStrRef<char>;
template class EqTStrRef<wchar_t>;