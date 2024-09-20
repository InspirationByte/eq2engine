//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Special String tools to do lesser memory errors
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include "ArrayRef.h"

template<typename CH>
class EqTStrRef;

template<typename CH>
class EqTStr;

using EqString = EqTStr<char>;
using EqWString = EqTStr<wchar_t>;

using EqStringRef = EqTStrRef<char>;
using EqWStringRef = EqTStrRef<wchar_t>;

#ifdef _WIN32
constexpr int CORRECT_PATH_SEPARATOR	= '\\';
constexpr int INCORRECT_PATH_SEPARATOR	= '/';
#else
constexpr int CORRECT_PATH_SEPARATOR	= '/';
constexpr int INCORRECT_PATH_SEPARATOR	= '\\';
#endif // _WIN32

static constexpr const char CORRECT_PATH_SEPARATOR_STR[2] = {CORRECT_PATH_SEPARATOR, '\0'};
static constexpr const char INCORRECT_PATH_SEPARATOR_STR[2] = {INCORRECT_PATH_SEPARATOR, '\0'};

//------------------------------------------------------
// String hash
//------------------------------------------------------

static constexpr const int StringHashBits = 24;
static constexpr const int StringHashMask = ((1 << StringHashBits) - 1);

template<int idx, std::size_t N>
struct StringToHashHelper {
	static constexpr int compute(const char(&str)[N], int hash) {
		const int v1 = hash >> 19;
		const int v0 = hash << 5;
		const int chr = str[N - idx - 1];
		hash = ((v0 | v1) + chr) & StringHashMask;
		return StringToHashHelper<idx - 1, N>::compute(str, hash);
	}
};

template<std::size_t N>
struct StringToHashHelper<0, N> {
	static constexpr int compute(const char(&)[N], int hash) {
		return hash;
	}
};

template <auto V> static constexpr auto force_consteval = V;
#define _StringToHashConst(x) StringToHashHelper<sizeof(x) - 1, sizeof(x)>::compute(x, sizeof(x) - 1)
#define StringToHashConst(x) force_consteval<_StringToHashConst(x)>

// generates string hash
int			StringToHash(EqStringRef str, bool caseIns = false);

//------------------------------------------------------
// String split helper
//------------------------------------------------------

// Split string by multiple separators
void		StringSplit(const char* pString, ArrayCRef<const char*> separators, Array<EqString>& outStrings);

// Split string by one separator
void		StringSplit(const char* pString, const char* separator, Array<EqString>& outStrings);

//------------------------------------------------------
// Path utils
//------------------------------------------------------

// strip operators
bool		fnmPathHasExt(EqStringRef path);
EqString	fnmPathApplyExt(EqStringRef path, EqStringRef ext);
EqString	fnmPathStripExt(EqStringRef path);
EqString	fnmPathStripName(EqStringRef path);
EqString	fnmPathStripPath(EqStringRef path);

EqString	fnmPathExtractExt(EqStringRef path, bool autoLowerCase = true);
EqString	fnmPathExtractName(EqStringRef path);
EqString	fnmPathExtractPath(EqStringRef path);

// changes path separator to correct one for platform
void		fnmPathFixSeparators(EqString& str);
void		fnmPathFixSeparators(char* str);

// combines paths
void		fnmPathCombineF(EqString& outPath, int num, ...);

template<typename ...Args> // requires std::same_as<Args, const char*>...
void		fnmPathCombine(EqString& outPath, const Args&... args)
{
	fnmPathCombineF(outPath, sizeof...(Args), ToCString(args)...);
}

//------------------------------------------------------

namespace CType
{
template<typename CH> bool IsAlphabetic(CH chr);
template<typename CH> bool IsAlphaNumeric(CH chr);
template<typename CH> bool IsDigit(CH chr);
template<typename CH> bool IsSpace(CH chr);

template<typename CH> CH LowerChar(CH chr);
template<typename CH> CH UpperChar(CH chr);

template<> bool IsAlphabetic(char chr);
template<> bool IsAlphaNumeric(char chr);
template<> bool IsDigit(char chr);
template<> bool IsSpace(char chr);

template<> char LowerChar(char chr);
template<> char UpperChar(char chr);

template<> bool IsAlphabetic(wchar_t chr);
template<> bool IsAlphaNumeric(wchar_t chr);
template<> bool IsDigit(wchar_t chr);
template<> bool IsSpace(wchar_t chr);

template<> wchar_t LowerChar(wchar_t chr);
template<> wchar_t UpperChar(wchar_t chr);
}

namespace CString
{
template<typename CH> int Length(const CH* str);
template<typename CH, typename CCH> CH* SubString(CH* str, const CCH* search);
template<typename CH, typename CCH> CH* SubStringCaseIns(CH* str, const CCH* search);

template<typename CH> const CH* SubString(const CH* str, const CH* search) { return SubString<CH, CH>(const_cast<CH*>(str), search); }
template<typename CH> const CH* SubStringCaseIns(const CH* str, const CH* search) { return SubStringCaseIns<CH, CH>(const_cast<CH*>(str), search); }

template<typename CH> CH* LowerCase(CH* str);
template<typename CH> CH* UpperCase(CH* str);

template<typename CH> int Compare(const CH* strA, const CH* strB);
template<typename CH> int CompareCaseIns(const CH* strA, const CH* strB);

template<typename CH> int PrintFV(CH* buffer, int bufferCnt, const CH* fmt, va_list argList);
template<typename CH> int PrintF(CH* buffer, int bufferCnt, const CH* fmt, ...);

char*		DuplicateNew(const char* s);		// duplicates string. Must be freed with SAFE_DELETE_ARRAY
wchar_t*	DuplicateNew(const wchar_t* s);		// duplicates string. Must be freed with SAFE_DELETE_ARRAY
}

//------------------------------------------------------
// String Comparison Operators

// TODO: move somewhere else
template<typename Self, typename Other = Self>
struct EMPTY_BASES ComparisonEqualsOpsMixin
{
	friend bool operator!=(const Self& a, const Other& b) { return !(a == b); }
};

template<typename TStr, typename TOther = TStr>
struct EMPTY_BASES StringComparisonOpsMixin
	: public ComparisonEqualsOpsMixin<TStr, TOther>
{
	/* (A == B) case-sensitive comparison */
	friend bool operator==(const TStr& a, const TOther& b) { return CString::Compare(a.ToCString(), b.ToCString()) == 0; }
};

template<typename TStr, typename CH>
struct EMPTY_BASES CStringComparisonOpsMixin
	: public ComparisonEqualsOpsMixin<TStr, const CH*>
{
	/* (A == B) case-sensitive comparison */
	friend bool operator==(const TStr& a, const CH* b) { return CString::Compare(a.ToCString(), b) == 0; }
};

//------------------------------------------------------
// String Combination Operators

template<typename R, typename TStr, typename CH>
struct EMPTY_BASES StringBaseCombinationOpsMixin
{
	friend R operator+(const TStr& a, const TStr& b)
	{
		R result(a);
		result.Append(b);
		return result;
	}
	friend R operator+( const TStr& a, const CH *b ) 
	{
		R result(a);
		result.Append(b);
		return result;
	}
	friend R operator+( const CH *a, const TStr& b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
};

template<typename R, typename TStr, typename CH>
struct EMPTY_BASES StringCombinationOpsMixin
	: public StringBaseCombinationOpsMixin<R, TStr, CH>
{
	friend R operator+( const TStr &a, EqTStrRef<CH> b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
	friend R operator+(EqTStrRef<CH> a, const TStr &b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
};

//------------------------------------------------------
// String ref itself

template<typename CH>
class EMPTY_BASES EqTStrRef
	: public StringBaseCombinationOpsMixin<EqTStr<CH>, EqTStrRef<CH>, char>
	, public CStringComparisonOpsMixin<EqTStrRef<CH>, CH>
	, public StringComparisonOpsMixin<EqTStrRef<CH>>
{
public:
	using Str = EqTStr<CH>;

	constexpr EqTStrRef()
		: m_pszString(nullptr)
		, m_nLength(-1)
	{
	}

	constexpr EqTStrRef(std::nullptr_t)
		: m_pszString(nullptr)
		, m_nLength(-1)
	{
	}

	constexpr EqTStrRef(const CH* str)
		: m_pszString(str)
		, m_nLength(-1)
	{
	}

	EqTStrRef(const CH* str, int length)
		: m_pszString(str)
		, m_nLength(length == -1 ? -1 : length)
	{
	}

	EqTStrRef(const EqTStrRef& other)
		: m_pszString(other.m_pszString)
		, m_nLength(other.m_nLength)
	{
	}
	
	bool		IsValid() const { return m_pszString != nullptr; }
	const CH*	GetData() const { return m_pszString; }
	const CH*	ToCString() const { return GetData(); }

	int			Length() const { return (m_nLength == -1) ? m_nLength = CString::Length(m_pszString) : m_nLength; }

	// comparison operations
	int			Compare(EqTStrRef otherStr) const;
	int			CompareCaseIns(EqTStrRef otherStr) const;
	int			GetMathingChars(EqTStrRef otherStr) const;
	int			GetMathingCharsCaseIns(EqTStrRef otherStr) const;

	// searches for substring, returns value
	int			Find(EqTStrRef otherStr, bool caseSensitive = false, int start = 0) const;

	// converters
	Str			LowerCase() const;
	Str			UpperCase() const;

	// rightmost\leftmost string extractors
	Str			Left(int nCount) const;
	Str			Right(int nCount) const;
	Str			Mid(int nStart, int nCount) const;

	Str			EatWhiteSpaces() const;
	Str			TrimSpaces(bool left = true, bool right = true) const;
	Str			TrimChar(const CH* ch, bool left = true, bool right = true) const;
	Str			TrimChar(CH ch, bool left = true, bool right = true) const;

	CH operator[](int idx) const
	{
		ASSERT(idx >= 0 && idx <= Length());
		return m_pszString[idx];
	}

	operator const CH* () const { return ToCString(); }
	operator bool() const { return IsValid(); }

private:
	const CH*	m_pszString{ nullptr };
	mutable int	m_nLength{ 0 };
};

template <typename T>
decltype(auto) ToCString(const T& value)
{
	if constexpr (
		std::is_same_v<T, EqStringRef> ||
		std::is_same_v<T, EqWStringRef> ||
		std::is_same_v<T, EqString> ||
		std::is_same_v<T, EqWString>)
	{
		return value.ToCString();
	}
#if 0
	else if constexpr (
		std::is_same_v<T, std::string> ||
		std::is_same_v<T, std::wstring>)
	{
		return value.c_str();
	}
#endif
	else
	{
		return value;
	}
}

class AnsiUnicodeConverter
{
public:
	AnsiUnicodeConverter(EqString& outStr, EqWStringRef sourceStr);
	AnsiUnicodeConverter(EqWString& outStr, EqStringRef sourceStr);
};