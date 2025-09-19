//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: String reference
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template<typename CH>
class EqTStrRef;

template<typename CH>
class EqTStr;

using EqString = EqTStr<char>;
using EqWString = EqTStr<wchar_t>;

using EqStringRef = EqTStrRef<char>;
using EqWStringRef = EqTStrRef<wchar_t>;

// fwd of string_utils
template <typename T>
decltype(auto) ToCString(const T& value);

//------------------------------------------------------

namespace CType
{
constexpr int ASCIILower_Cexpr(int c)
{
	constexpr int d = 'a' - 'A';
	return c + (c >= 'A' && c <= 'Z' ? d : 0);
}

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
int Length(const char* str);
int Length(const wchar_t* str);

char* SubString(char* str, const char* search);
wchar_t* SubString(wchar_t* str, const wchar_t* search);

char* SubStringCaseIns(char* str, const char* search);
wchar_t* SubStringCaseIns(wchar_t* str, const wchar_t* search);

template<typename CH> const CH* SubString(const CH* str, const CH* search) { return SubString(const_cast<CH*>(str), search); }
template<typename CH> const CH* SubStringCaseIns(const CH* str, const CH* search) { return SubStringCaseIns(const_cast<CH*>(str), search); }

char* LowerCase(char* str);
wchar_t* LowerCase(wchar_t* str);
char* UpperCase(char* str);
wchar_t* UpperCase(wchar_t* str);

int Compare(const char* strA, const char* strB);
int Compare(const wchar_t* strA, const wchar_t* strB);
int CompareCaseIns(const char* strA, const char* strB);
int CompareCaseIns(const wchar_t* strA, const wchar_t* strB);

int PrintFV(char* buffer, int bufferCnt, const char* fmt, va_list argList);
int PrintFV(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, va_list argList);
int PrintF(char* buffer, int bufferCnt, const char* fmt, ...);
int PrintF(wchar_t* buffer, int bufferCnt, const wchar_t* fmt, ...);

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
	friend R& operator+( const TStr& a, const TStr& b )
	{
		R& result = EqTStrRef<CH>::GetTempString(EqTStrRef<CH>(a));
		result.Append(b);
		return result;
	}
	friend R& operator+( const TStr& a, const CH *b ) 
	{
		R& result = EqTStrRef<CH>::GetTempString(EqTStrRef<CH>(a));
		result.Append(b);
		return result;
	}
	friend R& operator+( const CH *a, const TStr& b )
	{
		R& result = EqTStrRef<CH>::GetTempString(a);
		result.Append(b);
		return result;
	}
};

template<typename R, typename TStr, typename CH>
struct EMPTY_BASES StringCombinationOpsMixin
	: public StringBaseCombinationOpsMixin<R, TStr, CH>
{
	friend R& operator+( const TStr &a, EqTStrRef<CH> b )
	{
		R& result = EqTStrRef<CH>::GetTempString(EqTStrRef<CH>(a));
		result.Append(b);
		return result;
	}
	friend R& operator+(EqTStrRef<CH> a, const TStr &b )
	{
		R& result = EqTStrRef<CH>::GetTempString(a);
		result.Append(b);
		return result;
	}
};

//------------------------------------------------------
// String ref itself

#define _Es EqTStrRef

template<typename CH>
class EMPTY_BASES EqTStrRef
	: public StringBaseCombinationOpsMixin<EqTStr<CH>, EqTStrRef<CH>, char>
	, public CStringComparisonOpsMixin<EqTStrRef<CH>, CH>
	, public StringComparisonOpsMixin<EqTStrRef<CH>>
{
public:
	using Str = EqTStr<CH>;

	static EqTStr<CH>&	GetTempString(const CH* str, int len = -1);
	static EqTStr<CH>&	GetTempString(const EqTStrRef<CH>& str, int len = -1);

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

	int			Length() const;

	// comparison operations
	int			Compare(EqTStrRef otherStr) const;
	int			CompareCaseIns(EqTStrRef otherStr) const;
	int			GetMathingChars(EqTStrRef otherStr) const;
	int			GetMathingCharsCaseIns(EqTStrRef otherStr) const;

	// searches for substring, returns value
	int			Find(EqTStrRef otherStr, bool caseSensitive = false, int start = 0) const;
	int			Find(CH chr, bool caseSensitive = false, int start = 0) const { return Find(EqTStrRef(&chr, 1), caseSensitive, start); }

	// converters
	EqTStrRef	LowerCase() const;
	EqTStrRef	UpperCase() const;

	// rightmost\leftmost string extractors
	EqTStrRef	Left(int nCount) const;
	EqTStrRef	Right(int nCount) const;
	EqTStrRef	Mid(int nStart, int nCount) const;

	EqTStrRef	EatWhiteSpaces() const;
	EqTStrRef	TrimSpaces(bool left = true, bool right = true) const;
	EqTStrRef	TrimChar(const CH* ch, bool left = true, bool right = true) const;
	EqTStrRef	TrimChar(CH ch, bool left = true, bool right = true) const;

	CH			operator[](int idx) const;

	operator 	const CH* () const { return ToCString(); }

private:
	const CH*	m_pszString{ nullptr };
	mutable int	m_nLength{ 0 };
};
