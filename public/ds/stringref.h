//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Special String tools to do lesser memory errors
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class EqString;
class EqWString;

#ifdef _WIN32
#	define CORRECT_PATH_SEPARATOR '\\'
#	define INCORRECT_PATH_SEPARATOR '/'
#else
#	define CORRECT_PATH_SEPARATOR '/'
#	define INCORRECT_PATH_SEPARATOR '\\'
#endif // _WIN32

static constexpr const char CORRECT_PATH_SEPARATOR_STR[2] = {CORRECT_PATH_SEPARATOR, '\0'};
static constexpr const char INCORRECT_PATH_SEPARATOR_STR[2] = {INCORRECT_PATH_SEPARATOR, '\0'};

#ifdef PLAT_POSIX

#define _vsnwprintf vswprintf
#define _snprintf snprintf

#define stricmp(a, b)			strcasecmp(a, b)

#endif // PLAT_POSIX

#ifdef PLAT_ANDROID

typedef __builtin_va_list	va_list;
#ifndef va_start
#	define va_start(v,l)		__builtin_va_start(v,l)
#endif

#ifndef va_end
#	define va_end(v)			__builtin_va_end(v)
#endif

#ifndef va_arg
#	define va_arg(v,l)			__builtin_va_arg(v,l)
#endif

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L || defined(__GXX_EXPERIMENTAL_CXX0X__)

#	ifndef va_copy
#		define va_copy(d,s)		__builtin_va_copy(d,s)
#	endif

#endif

#ifndef __va_copy
#	define __va_copy(d,s)		__builtin_va_copy(d,s)
#endif

typedef __builtin_va_list	__gnuc_va_list;
typedef __gnuc_va_list		va_list;
typedef va_list				__va_list;

#endif // PLAT_ANDROID

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
int			StringToHash(const char* str, bool caseIns = false);

//------------------------------------------------------
// Path utils
//------------------------------------------------------

// combines paths
void		CombinePathN(EqString& outPath, int num, ...);

template<typename ...Args> // requires std::same_as<Args, const char*>...
void		CombinePath(EqString& outPath, const Args&... args)
{
	CombinePathN(outPath, sizeof...(Args), static_cast<const char*>(args)...);
}

// fixes slashes in the directory name
void		FixSlashes( char* str );

//------------------------------------------------------
// General string utilities
//------------------------------------------------------

// Split string by multiple separators
void		xstrsplit2(const char* pString, const char** pSeparators, int nSeparators, Array<EqString>& outStrings);

// Split string by one separator
void		xstrsplit(const char* pString, const char* pSeparator, Array<EqString>& outStrings);

char const* xstristr(char const* pStr, char const* pSearch);
char*		xstristr(char* pStr, char const* pSearch);

// fast duplicate c string
char*		xstrdup(const char*  s);

// converts string to lower case
char*		xstrupr(char* s1);
char*		xstrlwr(char* s1);

wchar_t*	xwcslwr(wchar_t* str);
wchar_t*	xwcsupr(wchar_t* str);

//------------------------------------------------------
// wide string
//------------------------------------------------------

// Compares string
int			xwcscmp ( const wchar_t *s1, const wchar_t *s2);

// compares two strings case-insensetive
int			xwcsicmp( const wchar_t* s1, const wchar_t* s2 );

// finds substring in string case insensetive
wchar_t*	xwcsistr( wchar_t* pStr, wchar_t const* pSearch );

// finds substring in string case insensetive
wchar_t const* xwcsistr( wchar_t const* pStr, wchar_t const* pSearch );

//------------------------------------------------------
// string encoding conversion
//------------------------------------------------------

namespace EqStringConv
{
class CUTF8Conv
{
public:
	CUTF8Conv(EqString& outStr, const wchar_t* val, int length = -1);
	CUTF8Conv(EqWString& outStr, const char* val, int length = -1);
};
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
template<typename CH> CH* SubString(CH* str, const CH* search, bool caseSensitive);
template<typename CH> int Compare(const CH* strA, const CH* strB);
template<typename CH> int CompareCaseIns(const CH* strA, const CH* strB);
}

//------------------------------------------------------
// String Comparison Operators

// TODO: move somewhere else
template<typename Self, typename Other = Self>
struct ComparisonEqualsOpsMixin
{
	friend bool operator!=(const Self& a, const Other& b) { return !(a == b); }
};

template<typename CH>
class StrRef;

template<typename TStr, typename TOther = TStr>
struct StringComparisonOpsMixin
	: public ComparisonEqualsOpsMixin<TStr, TOther>
{
	/* (A == B) case-sensitive comparison */
	friend bool operator==(const TStr& a, const TOther& b) { return CString::Compare(a.ToCString(), b.ToCString()) == 0; }
};

//------------------------------------------------------
// String Combination Operators

template<typename R, typename TStr, typename CH>
struct StringBaseCombinationOpsMixin
{
	friend R operator+( const TStr &a, const TStr &b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
	friend R operator+( const TStr &a, const CH *b ) 
	{
		R result(a);
		result.Append(b);
		return result;
	}
	friend R operator+( const CH *a, const TStr &b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
};

template<typename R, typename TStr, typename CH>
struct StringCombinationOpsMixin 
	: public StringBaseCombinationOpsMixin<R, TStr, CH>
{
	friend R operator+( const TStr &a, StrRef<CH> b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
	friend R operator+( StrRef<CH> a, const TStr &b )
	{
		R result(a);
		result.Append(b);
		return result;
	}
};

//------------------------------------------------------
// String ref itself

template<typename CH>
class StrRef
	: public StringBaseCombinationOpsMixin<EqString, StrRef<CH>, char>
	, public StringComparisonOpsMixin<StrRef<CH>>
{
public:
	constexpr StrRef()
		: m_pszString(nullptr)
		, m_nLength(-1)
	{
	}

	constexpr StrRef(std::nullptr_t)
		: m_pszString(nullptr)
		, m_nLength(-1)
	{
	}

	constexpr StrRef(const CH* str)
		: m_pszString(str)
		, m_nLength(-1)
	{
	}

	StrRef(const CH* str, int length)
		: m_pszString(str)
		, m_nLength(length == -1 ? -1 : length)
	{
	}

	StrRef(const StrRef& other)
		: m_pszString(other.m_pszString)
		, m_nLength(other.m_nLength)
	{
	}
	
	bool		IsValid() const { return m_pszString != nullptr; }
	const CH*	GetData() const { return m_pszString; }
	const CH*	ToCString() const { return GetData(); }

	int			Length() const { return (m_nLength == -1) ? m_nLength = CString::Length(m_pszString) : m_nLength; }

	// comparison operations
	int			Compare(StrRef otherStr) const;
	int			CompareCaseIns(StrRef otherStr) const;
	int			GetMathingChars(StrRef otherStr) const;
	int			GetMathingCharsCaseIns(StrRef otherStr) const;

	// searches for substring, returns value
	int			Find(StrRef otherStr, bool caseSensitive = false, int start = 0) const;

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

using EqStringRef = StrRef<char>;
using EqWStringRef = StrRef<wchar_t>;