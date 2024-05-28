//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define _Es		EqString

#ifndef STRING_OPERATORS
#define STRING_OPERATORS(spec, classname)									\
	spec classname operator+( const classname &a, const classname &b ) {	\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	spec classname operator+( const classname &a, const char *b ) {			\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	spec classname operator+( const char *a, const classname &b ) {			\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	/* (A == B) case-sensitive comparison */													\
	spec bool operator==(const classname& a, const classname& b) { return !a.Compare(b); }		\
	/* (A == B) case-sensitive comparison */													\
	spec bool operator==(const classname& a, const char* b) { return !a.Compare(b); }			\
	/* (A==B) case-sensitive comparison */														\
	spec bool operator==(const char* a, const classname& b) { return !b.Compare(a); }			\
	/* (A == B) case-insensitive comparison */													\
	spec bool operator^(const classname& a, const classname& b) { return !a.CompareCaseIns(b); }\
	/* (A==B) case-insensitive comparison */													\
	spec bool operator^(const classname& a, const char* b) { return !a.CompareCaseIns(b); }		\
	/* (A == B) case-insensitive comparison */													\
	spec bool operator^(const char* a, const classname& b) { return !b.CompareCaseIns(a); }		\
	/* (A != B) case-sensitive comparison */													\
	spec bool operator!=(const classname& a, const classname& b) { return !(a == b); }			\
	/* (A != B) case-sensitive comparison */													\
	spec bool operator!=(const classname& a, const char* b) { return !(a == b); }				\
	/* (A != B) case-sensitive comparison */													\
	spec bool operator!=(const char* a, const classname& b) { return !(a == b); }				\
	/* (A != B) case-insensitive comparison */													\
	spec bool operator%(const classname& a, const classname& b) { return !(a ^ b); }			\
	/* (A != B) case-insensitive comparison */													\
	spec bool operator%(const classname& a, const char* b) { return !(a ^ b); }					\
	/* (A != B) case-insensitive comparison*/													\
	spec bool operator%(const char* a, const classname& b) { return !(a ^ b); }
#endif // STRING_OPERATORS

class EqWString;

class EqString
{
	friend class EqWString;
public:

	static const EqString EmptyStr;

	EqString();
	~EqString();

	EqString(const char* pszString, int len = -1);
	EqString(const EqString &str, int nStart = 0, int len = -1);

	EqString(EqString&& str) noexcept;
	
	// conversion from wide char string
	EqString(const wchar_t* pszString, int len = -1);
	EqString(const EqWString &str, int nStart = 0, int len = -1);

	static EqString Format(const char* pszFormat, ...);
	static EqString FormatVa(const char* pszFormat, va_list argptr);
	static size_t	ReadString(IVirtualStream* stream, EqString& output);

	// returns true if data is valid and length is valid
	bool		IsValid() const;

	// data for printing
	const char* GetData() const;

	// nice std thing :)
	const char*	ToCString() const {return GetData();}

	// length of it
	ushort		Length() const;

	// string allocated size in bytes
	ushort		GetSize() const;

	// erases and deallocates data
	void		Clear();

	// empty the string and restore base buffer
	void		Empty();

	// an internal operation of allocation/extend
	bool		ExtendAlloc(int nSize, bool bCopy = true);

	// just a resize
	bool		Resize(int nSize, bool bCopy = true);

	// string assignment (or setvalue)
	void		Assign(const char* pszStr, int len = -1);
	void		Assign(const EqString &str, int nStart = 0, int len = -1);

	// wide char string assignment (or setvalue)
	void		Assign(const wchar_t* pszStr, int len = -1);
	void		Assign(const EqWString &str, int nStart = 0, int len = -1);

	// appends another string
	void		Append(const char c);
	void		Append(const char* pszStr, int nCount = -1);
	void		Append(const EqString &str);

	// inserts another string at position
	void		Insert(const char* pszStr, int nInsertPos, int nInsertCount = -1);
	void		Insert(const EqString &str, int nInsertPos);

	// removes characters
	void		Remove(int nStart, int nCount);

	// replaces characters
	void		Replace( char whichChar, char to );

	// converters
	EqString	LowerCase() const;
	EqString	UpperCase() const;

	// comparators
	int			Compare(const char* pszStr) const;
	int			Compare(const EqString &str) const;

	int			CompareCaseIns(const char* pszStr) const;
	int			CompareCaseIns(const EqString &str) const;

	int			GetMathingChars(const char* pszStr) const;
	int			GetMathingChars(const EqString &str) const;

	// rightmost\leftmost string extractors
	EqString	Left(int nCount) const;
	EqString	Right(int nCount) const;
	EqString	Mid(int nStart, int nCount) const;

	// strip operators
	EqString	Path_Strip_Ext() const;
	EqString	Path_Strip_Name() const;
	EqString	Path_Strip_Path() const;

	EqString	Path_Extract_Ext() const;
	EqString	Path_Extract_Name() const;
	EqString	Path_Extract_Path() const;

	EqString	EatWhiteSpaces() const;
	EqString	TrimSpaces(bool left = true, bool right = true) const;
	EqString	TrimChar(const char* ch, bool left = true, bool right = true) const;
	EqString	TrimChar(char ch, bool left = true, bool right = true) const;

	void		Path_FixSlashes() const;

	// search, returns char index
	int			Find(const char* pszSub, bool bCaseSensetive = false, int nStart = 0) const;

	// searches for substring and replaces it
	int			ReplaceSubstr(const char* find, const char* replaceTo, bool bCaseSensetive = false, int nStart = 0);

	// swaps two strings
	void		Swap(EqString& otherStr);
	
	//------------------------------------------------------------------------------------------------

	EqString& operator = (const EqString& other)
	{
		this->Assign( other );
		return *this;
	}

	EqString& operator = (EqString&& other) noexcept
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

	EqString& operator = (const char* pszStr)
	{
		this->Assign( pszStr );
		return *this;
	}

	char operator[](int idx) const
	{
		ASSERT(idx >= 0 && idx <= m_nLength);
		return m_pszString[idx];
	}

	operator const char* () 
	{
		return this->ToCString();
	}

	operator const char* () const
	{
		return this->ToCString();
	}

protected:

	bool		MakeInsertSpace(int startPos, int count);

	const char* StrPtr() const;
	char*		m_pszString{ nullptr };

	uint16		m_nLength{ 0 };			// length of string
	uint16		m_nAllocated{ 0 };		// allocation size
};

STRING_OPERATORS(static inline, EqString)

static size_t VSRead(IVirtualStream* stream, EqString& str)
{
	return EqString::ReadString(stream, str);
}

static size_t VSWrite(IVirtualStream* stream, const EqString& str)
{
	const uint16 length = str.Length();
	stream->Write(&length, 1, sizeof(uint16));
	stream->Write(str.GetData(), sizeof(char), length);
	return 1;
}