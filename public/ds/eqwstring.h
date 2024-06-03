//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine wide string base
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define _EWs EqWString

class EqString;

class EMPTY_BASES EqWString
	: public StringCombinationOpsMixin<EqWString, EqWString, wchar_t>
	, public StringComparisonOpsMixin<EqWString>
	, public StringComparisonOpsMixin<EqWString, EqTStrRef<wchar_t>>
	, public CStringComparisonOpsMixin<EqString, const wchar_t*>
{
	friend class EqString;

public:
	static const EqWString EmptyStr;

	EqWString();
	~EqWString();

	// convert from UTF8 string
	EqWString(const char* pszString, int len = -1);
	EqWString(const EqString& str, int nStart = 0, int len = -1);

	EqWString(const wchar_t* pszString, int len = -1);
	EqWString(const EqWString &str, int nStart = 0, int len = -1);

	EqWString(EqWString&& str) noexcept;

	static EqWString FormatF(const wchar_t* pszFormat, ...);
	static EqWString FormatV(const wchar_t* pszFormat, va_list args);
	template <typename... Args>
	static EqWString Format(const wchar_t* pszFormat, Args&&... args);

	static size_t	 ReadString(IVirtualStream* stream, EqWString& output);

	bool			IsValid() const;
	const wchar_t*	GetData() const { return StrPtr(); }
	const wchar_t*	ToCString() const { return GetData(); }
	EqWStringRef	Ref() const { return EqWStringRef(GetData(), Length()); }

	ushort			Length() const { return m_nLength; }
	ushort			GetSize() const { return m_nAllocated; }

	// comparison operations
	int				Compare(const wchar_t* pszStr) const { return Ref().Compare(pszStr); }
	int				CompareCaseIns(const wchar_t* pszStr) const { return Ref().CompareCaseIns(pszStr); }
	int				GetMathingChars(const wchar_t* pszStr) const { return Ref().GetMathingChars(pszStr); }
	int				GetMathingCharsCaseIns(const wchar_t* pszStr) const { return Ref().GetMathingCharsCaseIns(pszStr); }

	// searches for substring, returns position
	int				Find(const wchar_t* pszSub, bool bCaseSensitive = false, int nStart = 0) const { return Ref().Find(pszSub, bCaseSensitive, nStart); }

	// modifying
	void			Clear();
	void			Empty();

	bool			ExtendAlloc(int nSize);
	bool			Resize(int nSize, bool bCopy = true);

	void			Assign(const char* pszStr, int len = -1);
	void			Assign(const EqString &str, int nStart = 0, int len = -1);
	void			Assign(const wchar_t* pszStr, int len = -1);
	void			Assign(const EqWString &str, int nStart = 0, int len = -1);

	void			Append(const wchar_t c);
	void			Append(const wchar_t* pszStr, int nCount = -1);
	void			Append(const EqWString &str);

	void			Insert(const wchar_t* pszStr, int nInsertPos, int nInsertCount = -1);
	void			Insert(const EqWString &str, int nInsertPos);

	void			Remove(int nStart, int nCount);
	void			ReplaceChar( wchar_t whichChar, wchar_t to );
	int				ReplaceSubstr(const wchar_t* find, const wchar_t* replaceTo, bool bCaseSensetive = false, int nStart = 0);

	// converters
	EqWString		LowerCase() const;
	EqWString		UpperCase() const;

	EqWString		Left(int nCount) const;
	EqWString		Right(int nCount) const;
	EqWString		Mid(int nStart, int nCount) const;


	//------------------------------------------------------------------------------------------------

	EqWString& operator = (EqWString&& other) noexcept
	{
		Clear();
		m_nAllocated = other.m_nAllocated;
		m_nLength = other.m_nLength;
		m_pszString = other.m_pszString;
		other.m_nAllocated = 0;
		other.m_nLength = 0;
		other.m_pszString = nullptr;
		return *this;
	}

	EqWString& operator = (const EqWString& other)
	{
		this->Assign( other );
		return *this;
	}

	EqWString& operator = (const wchar_t* pszStr)
	{
		this->Assign( pszStr );
		return *this;
	}

	wchar_t operator[](int idx) const
	{
		ASSERT(idx >= 0 && idx <= m_nLength);
		return m_pszString[idx];
	}

	operator const wchar_t* () { return ToCString(); }
	operator const wchar_t* () const { return ToCString(); }
	operator EqWStringRef () const { return Ref(); }

protected:
	const wchar_t*	StrPtr() const;
	bool			MakeInsertSpace(int startPos, int count);

	wchar_t*		m_pszString{ nullptr };

	uint16			m_nLength{ 0 };			// length of string
	uint16			m_nAllocated{ 0 };		// allocation size
};

//STRING_OPERATORS(static inline, EqWString, wchar_t)

static size_t VSRead(IVirtualStream* stream, EqWString& str)
{
	return EqWString::ReadString(stream, str);
}

static size_t VSWrite(IVirtualStream* stream, const EqWString& str)
{
	const uint16 length = str.Length();
	stream->Write(&length, 1, sizeof(uint16));
	stream->Write(str.GetData(), sizeof(wchar_t), length);
	return 1;
}

template <typename... Args>
inline EqWString EqWString::Format(const wchar_t* pszFormat, Args&&... args)
{
	return FormatF(pszFormat, StrToFmt(std::forward<Args>(args))...);
}