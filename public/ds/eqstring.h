//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: String template
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define _Es EqTStr

template<typename CH>
class EMPTY_BASES EqTStr
	: public StringCombinationOpsMixin<EqTStr<CH>, EqTStr<CH>, char>
	, public StringComparisonOpsMixin<EqTStr<CH>>
	, public StringComparisonOpsMixin<EqTStr<CH>, EqTStrRef<char>>
	, public CStringComparisonOpsMixin<EqTStr<CH>, char>
{
public:
	using StrRef = EqTStrRef<CH>;

	static const EqTStr EmptyStr;

	~EqTStr();

	EqTStr() = default;
	EqTStr(const CH* pszString, int len = -1);
	EqTStr(StrRef str, int nStart = 0, int len = -1);
	EqTStr(const EqTStr& str, int nStart = 0, int len = -1);
	EqTStr(EqTStr&& str) noexcept;

	static EqTStr FormatF(const CH* pszFormat, ...);
	static EqTStr FormatV(const CH* pszFormat, va_list argptr);
	template <typename... Args>
	static EqTStr Format(const CH* pszFormat, Args&&... args);

	static size_t	ReadString(IVirtualStream* stream, EqTStr& output);

	bool		IsValid() const;
	const CH*	GetData() const { return StrPtr(); }
	CH*			GetData() { return m_pszString; }
	const CH*	ToCString() const { return GetData(); }
	StrRef		Ref() const { return StrRef(GetData(), Length()); }

	ushort		Length() const { return m_nLength; }
	ushort		GetSize() const { return m_nAllocated; }
	
	// comparison operations
	int			Compare(StrRef strRef) const { return Ref().Compare(strRef); }
	int			CompareCaseIns(StrRef strRef) const { return Ref().CompareCaseIns(strRef); }
	int			GetMathingChars(StrRef strRef) const { return Ref().GetMathingChars(strRef); }
	int			GetMathingCharsCaseIns(StrRef strRef) const { return Ref().GetMathingCharsCaseIns(strRef); }
	
	// searches for substring, returns position
	int			Find(StrRef strSub, bool bCaseSensitive = false, int nStart = 0) const { return Ref().Find(strSub, bCaseSensitive, nStart); }

	// modifying
	void		Clear();
	void		Empty();

	bool		ExtendAlloc(int nSize, bool bCopy = true);
	bool		Resize(int nSize, bool bCopy = true);

	void		Assign(const CH* pszStr, int len = -1);
	void		Assign(const EqTStr& str, int nStart = 0, int len = -1);
	void		Assign(StrRef str, int nStart = 0, int len = -1);

	void		Append(const CH c);
	void		Append(const CH* pszStr, int nCount = -1);
	void		Append(const EqTStr& str);
	void		Append(StrRef str);

	void		Insert(const CH* pszStr, int nInsertPos, int nInsertCount = -1);
	void		Insert(const EqTStr& str, int nInsertPos);
	void		Insert(StrRef str, int nInsertPos);
	void		Remove(int nStart, int nCount);

	void		ReplaceChar(CH whichChar, CH to);
	int			ReplaceSubstr(StrRef find, StrRef replaceTo, bool bCaseSensetive = false, int nStart = 0);

	// converters
	EqTStr		LowerCase() const { return Ref().LowerCase(); }
	EqTStr		UpperCase() const { return Ref().UpperCase(); }

	// rightmost\leftmost string extractors
	EqTStr		Left(int nCount) const { return Ref().Left(nCount); }
	EqTStr		Right(int nCount) const { return Ref().Right(nCount); }
	EqTStr		Mid(int nStart, int nCount) const { return Ref().Mid(nStart, nCount); }

	EqTStr		EatWhiteSpaces() const { return Ref().EatWhiteSpaces(); }
	EqTStr		TrimSpaces(bool left = true, bool right = true) const { return Ref().TrimSpaces(left, right); }
	EqTStr		TrimChar(const CH* ch, bool left = true, bool right = true) const { return Ref().TrimChar(ch, left, right); }
	EqTStr		TrimChar(CH ch, bool left = true, bool right = true) const { return Ref().TrimChar(ch, left, right); }

	//------------------------------------------------------------------------------------------------

	EqTStr& operator = (const EqTStr& other)
	{
		this->Assign( other );
		return *this;
	}

	EqTStr& operator = (const StrRef& other)
	{
		this->Assign(other);
		return *this;
	}

	EqTStr& operator = (EqTStr&& other) noexcept
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

	EqTStr& operator = (const CH* pszStr)
	{
		this->Assign( pszStr );
		return *this;
	}

	CH operator[](int idx) const
	{
		ASSERT(idx >= 0 && idx <= m_nLength);
		return m_pszString[idx];
	}

	operator const CH* () const { return ToCString(); }
	operator StrRef () const { return Ref(); }

protected:

	bool		MakeInsertSpace(int startPos, int count);

	const CH*	StrPtr() const;
	CH*			m_pszString{ nullptr };

	uint16		m_nLength{ 0 };			// length of string
	uint16		m_nAllocated{ 0 };		// allocation size
};

template<typename CH>
static size_t VSRead(IVirtualStream* stream, EqTStr<CH>& str)
{
	return EqTStr<CH>::ReadString(stream, str);
}

template<typename CH>
static size_t VSWrite(IVirtualStream* stream, const EqTStr<CH>& str)
{
	const uint16 length = str.Length();
	stream->Write(&length, 1, sizeof(uint16));
	stream->Write(str.GetData(), sizeof(CH), length);
	return 1;
}

template <typename CH>
template <typename... Args>
inline EqTStr<CH> EqTStr<CH>::Format(const CH* pszFormat, Args&&... args)
{
	return FormatF(pszFormat, StrToFmt(std::forward<Args>(args))...);
}