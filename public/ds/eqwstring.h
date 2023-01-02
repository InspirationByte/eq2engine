//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine wide string base
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define _EWs EqWString

class EqString;

class EqWString
{
	friend class EqString;

public:
	EqWString();
	~EqWString();

	EqWString(const wchar_t c);

	// convert from UTF8 string
	EqWString(const char* pszString, int len = -1);
	EqWString(const EqString& str, int nStart = 0, int len = -1);

	EqWString(const wchar_t* pszString, int len = -1);
	EqWString(const EqWString &str, int nStart = 0, int len = -1);

	static EqWString Format(const wchar_t* pszFormat, ...);
	static EqWString FormatVa(const wchar_t* pszFormat, va_list args);

	// data for printing
	const wchar_t*	GetData() const;

	// nice std thing :)
	const wchar_t*	ToCString() const {return GetData();}

	// length of it
	uint			Length() const;

	// string allocated size in bytes
	uint			GetSize() const;

	// erases and deallocates data
	void			Clear();

	// empty the string and restore base buffer
	void			Empty();

	// an internal operation of allocation/extend
	bool			ExtendAlloc(int nSize);

	// just a resize
	bool			Resize(uint nSize, bool bCopy = true);

	// string assignment with conversion (or setvalue)
	void			Assign(const char* pszStr, int len = -1);
	void			Assign(const EqString &str, int nStart = 0, int len = -1);

	// string assignment (or setvalue)
	void			Assign(const wchar_t* pszStr, int len = -1);
	void			Assign(const EqWString &str, int nStart = 0, int len = -1);

	// appends another string
	void			Append(const wchar_t c);
	void			Append(const wchar_t* pszStr, int nCount = -1);
	void			Append(const EqWString &str);

	// inserts another string at position
	void			Insert(const wchar_t* pszStr, int nInsertPos);
	void			Insert(const EqWString &str, int nInsertPos);

	// removes characters
	void			Remove(uint nStart, uint nCount);

	// replaces characters
	void			Replace( wchar_t whichChar, wchar_t to );

	// rightmost\leftmost string extractors
	EqWString		Left(int nCount) const;
	EqWString		Right(int nCount) const;
	EqWString		Mid(int nStart, int nCount) const;

	// converters
	EqWString		LowerCase() const;
	EqWString		UpperCase() const;

	// comparators
	int				Compare(const wchar_t* pszStr) const;
	int				Compare(const EqWString &str) const;

	int				CompareCaseIns(const wchar_t* pszStr) const;
	int				CompareCaseIns(const EqWString &str) const;

	// search, returns char index
	int				Find(const wchar_t* pszSub, bool bCaseSensetive = false, int nStart = 0) const;

	// searches for substring and replaces it
	int				ReplaceSubstr(const wchar_t* find, const wchar_t* replaceTo, bool bCaseSensetive = false, int nStart = 0);

	//------------------------------------------------------------------------------------------------

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

	operator const wchar_t* () 
	{
		return this->ToCString();
	}

	operator const wchar_t* () const
	{
		return this->ToCString();
	}

protected:
	wchar_t*	m_pszString;

	uint16		m_nLength;			// length of string
	uint16		m_nAllocated;		// allocation size
};

STRING_OPERATORS(static inline, EqWString)