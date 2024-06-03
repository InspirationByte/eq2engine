//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//
//				Some things was lovely hardcoded (like m_nLength)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqwstring.h"

#ifdef _MSC_VER
#pragma warning(disable: 4267)
#endif

#define EXTEND_CHARS	32	// 32 characters for extending
#define EQSTRING_BASE_BUFFER	32

const EqWString EqWString::EmptyStr;
static const PPSourceLine EqWStringSL = PPSourceLine::Make(nullptr, 0);

EqWString::EqWString()
{
}

EqWString::~EqWString()
{
	Clear();
}

// convert from UTF8 string
EqWString::EqWString(const char* pszString, int len)
{
	Assign( pszString, len );
}

EqWString::EqWString(const EqString& str, int nStart, int len)
{
	Assign( str, nStart, len );
}

EqWString::EqWString(const wchar_t* pszString, int len)
{
	Assign( pszString, len );
}

EqWString::EqWString(const EqWString &str, int nStart, int len)
{
	Assign( str, nStart, len );
}

EqWString::EqWString(EqWString&& str) noexcept
{
	m_nAllocated = str.m_nAllocated;
	m_nLength = str.m_nLength;
	m_pszString = str.m_pszString;
	str.m_nAllocated = 0;
	str.m_nLength = 0;
	str.m_pszString = nullptr;
}

EqWString EqWString::FormatF(const wchar_t* pszFormat, ...)
{
	EqWString newString;
	va_list argptr;

	va_start(argptr, pszFormat);
	newString = EqWString::FormatV(pszFormat, argptr);
	va_end(argptr);

	return newString;
}

EqWString EqWString::FormatV(const wchar_t* pszFormat, va_list argptr)
{
	EqWString newString;
	newString.Resize(512, false);

	va_list varg;
	va_copy(varg, argptr);
	const int reqSize = _vsnwprintf(newString.m_pszString, newString.m_nAllocated, pszFormat, varg);

	if (reqSize < newString.m_nAllocated)
	{
		newString.m_nLength = reqSize;
		return newString;
	}

	newString.Resize(reqSize+1, false);
	newString.m_nLength = _vsnwprintf(newString.m_pszString, newString.m_nAllocated+1, pszFormat, argptr);

	return newString;
}

const wchar_t* EqWString::StrPtr() const
{
	static const wchar_t s_zeroString[] = L"";
	return m_pszString ? m_pszString : s_zeroString;
}

bool EqWString::IsValid() const
{
	return wcslen(StrPtr()) == m_nLength;
}

// erases and deallocates data
void EqWString::Clear()
{
	SAFE_DELETE_ARRAY(m_pszString);

	m_nLength = 0;
	m_nAllocated = 0;
}

// empty the string, but do not deallocate
void EqWString::Empty()
{
	if (!m_pszString)
		return;
	*m_pszString = 0;
	m_nLength = 0;
}

// an internal operation of allocation/extend
bool EqWString::ExtendAlloc(int nSize)
{
	if(nSize+1u > m_nAllocated || m_pszString == nullptr)
	{
		nSize += EXTEND_CHARS;
		if(!Resize( nSize - nSize % EXTEND_CHARS ))
			return false;
	}

	return true;
}

// just a resize
bool EqWString::Resize(int nSize, bool bCopy)
{
	const int newSize = max(EQSTRING_BASE_BUFFER, nSize + 1);

	// make new and copy
	wchar_t* pszNewBuffer = PPNewSL(EqWStringSL) wchar_t[ newSize ];

	// allocation error!
	if(!pszNewBuffer)
		return false;

	pszNewBuffer[0] = 0;

	// copy and remove old if available
	if( m_pszString )
	{
		// if we have to copy
		if(bCopy && m_nLength)
		{
			// if string length if bigger, that the new alloc, cut off
			// for safety
			if(m_nLength > newSize)
				m_pszString[newSize] = 0;

			wcscpy(pszNewBuffer, m_pszString);
		}

		// now it's not needed
		SAFE_DELETE_ARRAY(m_pszString);
	}

	// assign
	m_pszString = pszNewBuffer;
	m_nAllocated = newSize;

	if (nSize < m_nLength)
		m_nLength = wcslen( m_pszString );

	return true;
}

// string assignment with conversion (or setvalue)
void EqWString::Assign(const char* pszStr, int len)
{
	EqStringConv::CUTF8Conv(*this, pszStr, len);
}

void EqWString::Assign(const EqString &str, int nStart, int len)
{
	EqStringConv::CUTF8Conv(*this, str.ToCString() + nStart, len);
}

// string assignment (or setvalue)
void EqWString::Assign(const wchar_t* pszStr, int len)
{
	if(pszStr == nullptr)
	{
		if (m_pszString)
			m_pszString[0] = 0;
		m_nLength = 0;
		return;
	}

	if (len == -1)
		len = wcslen(pszStr);

	if (m_pszString == pszStr && len <= m_nLength)
	{
		m_nLength = len;
		m_pszString[len] = 0;
	}

	if (ExtendAlloc(len + 1))
	{
		if(pszStr != m_pszString)
			wcsncpy(m_pszString, pszStr, len);
		m_pszString[len] = 0;
	}
	m_nLength = len;
}

void EqWString::Assign(const EqWString &str, int nStart, int len)
{
	ASSERT(nStart >= 0);

	int nLen = str.Length();
	ASSERT(len <= nLen);

	if(len != -1)
		nLen = len;

	if (!ExtendAlloc(nLen))
		return;

	wcscpy( m_pszString+nStart, str.GetData() );
	m_pszString[nLen] = 0;
	m_nLength = nLen;
}

void EqWString::Append(const wchar_t c)
{
	const int nNewLen = m_nLength + 1;
	if (!ExtendAlloc(nNewLen))
		return;

	m_pszString[nNewLen-1] = c;
	m_pszString[nNewLen] = 0;
	m_nLength = nNewLen;
}

// appends another string
void EqWString::Append(const wchar_t* pszStr, int nCount)
{
	if(pszStr == nullptr)
		return;

	int nLen = wcslen( pszStr );
	ASSERT(nCount <= nLen);

	if(nCount != -1)
		nLen = nCount;

	const int nNewLen = m_nLength + nLen;
	if (!ExtendAlloc(nNewLen))
		return;

	wcsncpy( (m_pszString + m_nLength), pszStr, nLen);
	m_pszString[nNewLen] = 0;
	m_nLength = nNewLen;
}

void EqWString::Append(const EqWString &str)
{
	const int nNewLen = m_nLength + str.Length();
	if (!ExtendAlloc(nNewLen))
		return;

	wcscpy( (m_pszString + m_nLength), str.GetData() );
	m_pszString[nNewLen] = 0;
	m_nLength = nNewLen;
}

bool EqWString::MakeInsertSpace(int startPos, int count)
{
	const int newLength = m_nLength + count;
	if (!ExtendAlloc(newLength + 1))
		return false;

	wchar_t* insStartPtr = m_pszString + startPos;
	wchar_t* insEndPtr = m_pszString + startPos + count;

	// move the right part of the string further
	memmove(insEndPtr, insStartPtr, sizeof(insStartPtr[0]) * (int(m_nLength) - startPos));

	m_pszString[newLength] = 0;
	m_nLength = newLength;

	return true;
}

// inserts another string at position
void EqWString::Insert(const wchar_t* pszStr, int nInsertPos, int nInsertCount)
{
	if (pszStr == nullptr || nInsertCount == 0)
		return;

	const int strLen = wcslen(pszStr);
	ASSERT(nInsertCount <= strLen);
	if (nInsertCount < 0)
		nInsertCount = strLen;

	if (!MakeInsertSpace(nInsertPos, nInsertCount))
		return;

	// copy the inserted string in to the middle
	memcpy(m_pszString + nInsertPos, pszStr, sizeof(m_pszString[0]) * nInsertCount);

	ASSERT(IsValid());
}

void EqWString::Insert(const EqWString &str, int nInsertPos)
{
	ASSERT(str.IsValid());
	Insert(str, nInsertPos, str.Length());
}

// removes characters
void EqWString::Remove(int nStart, int nCount)
{
	if (!m_pszString || nCount <= 0)
		return;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= m_nLength);
	if (nStart < 0 || nStart + nCount > m_nLength)
		return;

	wchar_t* remStartPtr = m_pszString + nStart;
	const wchar_t* remEndPtr = m_pszString + nStart + nCount;

	// move the right part of the string to the left
	memmove(remStartPtr, remEndPtr, sizeof(remStartPtr[0]) * (int(m_nLength) - (nStart + nCount)));

	// put terminator
	const int newLength = m_nLength - nCount;
	m_pszString[newLength] = 0;
	m_nLength = newLength;
}

// replaces characters
void EqWString::ReplaceChar( wchar_t whichChar, wchar_t to )
{
	if (whichChar == 0 || to == 0) // can't replace to terminator
		return;

	if (!m_pszString)
		return;

	wchar_t* pStr = m_pszString;
	for(uint i = 0; i < m_nLength; i++)
	{
		if(*pStr == 0)
			break;

		if(*pStr == whichChar)
			*pStr = to;

		pStr++;
	}
}

// string extractors
EqWString EqWString::Left(int nCount) const
{
	return Mid(0, nCount);
}

EqWString EqWString::Right(int nCount) const
{
	if (nCount >= m_nLength)
		return (*this);

	return Mid(m_nLength - nCount, nCount);
}

EqWString EqWString::Mid(int nStart, int nCount) const
{
	if (!m_pszString)
		return EqWString::EmptyStr;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= m_nLength);
	if (nStart < 0 || nStart + nCount > m_nLength)
		return EqWString::EmptyStr;

	return EqWString(&m_pszString[nStart], nCount);
}

// convert to lower case
EqWString EqWString::LowerCase() const
{
	if (!m_pszString)
		return EqWString::EmptyStr;

	EqWString str(*this);
	xwcslwr(str.m_pszString);

	return str;
}

// convert to upper case
EqWString EqWString::UpperCase() const
{
	if (!m_pszString)
		return EqWString::EmptyStr;

	EqWString str(*this);
	xwcsupr(str.m_pszString);

    return str;
}

// searches for substring and replaces it
int EqWString::ReplaceSubstr(const wchar_t* find, const wchar_t* replaceTo, bool caseSensivite /*= false*/, int nStart /*= 0*/)
{
	// replace substring
	const int foundStartPos = Find(find, caseSensivite, nStart);
	if (foundStartPos == -1)
		return -1;

	const int findLength = wcslen(find);
	const int replaceLength = wcslen(replaceTo);

	if (replaceLength > findLength)
	{
		if (!MakeInsertSpace(foundStartPos + findLength, replaceLength - findLength))
			return -1;
	}
	else if (findLength > replaceLength)
	{
		Remove(foundStartPos + replaceLength, findLength - replaceLength);
	}

	// just copy part of the string
	memcpy(m_pszString + foundStartPos, replaceTo, sizeof(m_pszString[0]) * replaceLength);

	ASSERT(IsValid());

	return foundStartPos;
}

size_t EqWString::ReadString(IVirtualStream* stream, EqWString& output)
{
	uint16 length = 0;
	stream->Read(&length, 1, sizeof(length));
	output.Resize(length, false);

	stream->Read(output.m_pszString, sizeof(wchar_t), length);
	output.m_nLength = length;

	return 1;
}
