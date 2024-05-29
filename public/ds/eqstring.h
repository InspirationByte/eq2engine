//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define _Es		EqString

#ifndef STRING_OPERATORS
#define STRING_OPERATORS(spec, classname, CH)								\
	spec classname operator+( const classname &a, const classname &b ) {	\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	spec classname operator+( const classname &a, const CH *b ) {			\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	spec classname operator+( const CH *a, const classname &b ) {			\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	spec classname operator+( const classname &a, const StrRef<CH>& b ) {	\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	spec classname operator+( const StrRef<CH>& a, const classname &b ) {	\
		classname result(a);												\
		result.Append(b);													\
		return result;														\
	}																		\
	/* (A == B) case-sensitive comparison */													\
	spec bool operator==(const classname& a, const classname& b) { return !a.Compare(b); }		\
	/* (A == B) case-sensitive comparison */													\
	spec bool operator==(const classname& a, const StrRef<CH>& b) { return !a.Compare(b); }			\
	spec bool operator==(const classname& a, const CH* b) { return !a.Compare(b); }					\
	/* (A==B) case-sensitive comparison */														\
	spec bool operator==(const StrRef<CH>& a, const classname& b) { return !a.Compare(b); }			\
	spec bool operator==(const CH* a, const classname& b) { return !b.Compare(a); }					\
	/* (A == B) case-insensitive comparison */													\
	spec bool operator^(const classname& a, const classname& b) { return !a.CompareCaseIns(b); }\
	/* (A==B) case-insensitive comparison */													\
	spec bool operator^(const classname& a, const StrRef<CH>& b) { return !a.CompareCaseIns(b); }		\
	spec bool operator^(const classname& a, const CH* b) { return !a.CompareCaseIns(b); }		\
	/* (A == B) case-insensitive comparison */													\
	spec bool operator^(const StrRef<CH>& a, const classname& b) { return !a.CompareCaseIns(b); }		\
	spec bool operator^(const CH* a, const classname& b) { return !b.CompareCaseIns(a); }		\
	/* (A != B) case-sensitive comparison */													\
	spec bool operator!=(const classname& a, const classname& b) { return !(a == b); }			\
	/* (A != B) case-sensitive comparison */													\
	spec bool operator!=(const classname& a, const StrRef<CH>& b) { return !(a == b); }				\
	spec bool operator!=(const classname& a, const CH* b) { return !(a == b); }				\
	/* (A != B) case-sensitive comparison */													\
	spec bool operator!=(const StrRef<CH>& a, const classname& b) { return !(a == b); }				\
	spec bool operator!=(const CH* a, const classname& b) { return !(a == b); }				\
	/* (A != B) case-insensitive comparison */													\
	spec bool operator%(const classname& a, const classname& b) { return !(a ^ b); }			\
	/* (A != B) case-insensitive comparison */													\
	spec bool operator%(const classname& a, const StrRef<CH>& b) { return !(a ^ b); }					\
	spec bool operator%(const classname& a, const CH* b) { return !(a ^ b); }					\
	/* (A != B) case-insensitive comparison*/													\
	spec bool operator%(const StrRef<CH>& a, const classname& b) { return !(a ^ b); }			\
	spec bool operator%(const CH* a, const classname& b) { return !(a ^ b); }
#endif // STRING_OPERATORS

class EqWString;

class EqString
{
	friend class EqWString;
public:
	static const EqString EmptyStr;

	~EqString();

	EqString() = default;
	EqString(const char* pszString, int len = -1);
	EqString(EqStringRef str, int nStart = 0, int len = -1);
	EqString(const EqString &str, int nStart = 0, int len = -1);
	EqString(EqString&& str) noexcept;

	EqString(const wchar_t* pszString, int len = -1);
	EqString(const EqWString &str, int nStart = 0, int len = -1);

	static EqString Format(const char* pszFormat, ...);
	static EqString FormatVa(const char* pszFormat, va_list argptr);
	static size_t	ReadString(IVirtualStream* stream, EqString& output);

	bool		IsValid() const;
	const char* GetData() const { return StrPtr(); }
	const char*	ToCString() const { return GetData(); }
	EqStringRef	Ref() const { return EqStringRef(GetData(), Length()); }

	ushort		Length() const { return m_nLength; }
	ushort		GetSize() const { return m_nAllocated; }
	
	// comparison operations
	int			Compare(EqStringRef strRef) const { return Ref().Compare(strRef); }
	int			CompareCaseIns(EqStringRef strRef) const { return Ref().CompareCaseIns(strRef); }
	int			GetMathingChars(EqStringRef strRef) const { return Ref().GetMathingChars(strRef); }
	int			GetMathingCharsCaseIns(EqStringRef strRef) const { return Ref().GetMathingCharsCaseIns(strRef); }
	
	// searches for substring, returns position
	int			Find(EqStringRef strSub, bool bCaseSensitive = false, int nStart = 0) const { return Ref().Find(strSub, bCaseSensitive, nStart); }

	// modifying
	void		Clear();
	void		Empty();

	bool		ExtendAlloc(int nSize, bool bCopy = true);
	bool		Resize(int nSize, bool bCopy = true);

	void		Assign(const char* pszStr, int len = -1);
	void		Assign(const EqString &str, int nStart = 0, int len = -1);
	void		Assign(EqStringRef str, int nStart = 0, int len = -1);
	void		Assign(const wchar_t* pszStr, int len = -1);
	void		Assign(const EqWString &str, int nStart = 0, int len = -1);

	void		Append(const char c);
	void		Append(const char* pszStr, int nCount = -1);
	void		Append(const EqString &str);
	void		Append(EqStringRef str);

	void		Insert(const char* pszStr, int nInsertPos, int nInsertCount = -1);
	void		Insert(const EqString &str, int nInsertPos);
	void		Remove(int nStart, int nCount);

	void		ReplaceChar( char whichChar, char to );
	int			ReplaceSubstr(EqStringRef find, EqStringRef replaceTo, bool bCaseSensetive = false, int nStart = 0);

	void		Path_FixSlashes() const;

	// converters
	EqString	LowerCase() const;
	EqString	UpperCase() const;

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
	
	//------------------------------------------------------------------------------------------------

	EqString& operator = (const EqString& other)
	{
		this->Assign( other );
		return *this;
	}

	EqString& operator = (const EqStringRef& other)
	{
		this->Assign(other);
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

	operator const char* () { return ToCString(); }
	operator const char* () const { return ToCString(); }
	operator EqStringRef () const { return Ref(); }

protected:

	bool		MakeInsertSpace(int startPos, int count);

	const char* StrPtr() const;
	char*		m_pszString{ nullptr };

	uint16		m_nLength{ 0 };			// length of string
	uint16		m_nAllocated{ 0 };		// allocation size
};

STRING_OPERATORS(static inline, EqString, char)

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