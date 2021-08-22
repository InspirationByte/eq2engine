//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//
//				Some things was lovely hardcoded (like m_nLength)//////////////////////////////////////////////////////////////////////////////////

#include "eqwstring.h"
#include "math/math_common.h"		// min/max

#include <stdio.h>
#include <malloc.h>
#include "core/platform/Platform.h"
#include "strtools.h"

#ifdef PLAT_POSIX
#include <wchar.h>
#endif // PLAT_POSIX

#define BASE_BUFFER		32	// 32 characters initial buffer
#define EXTEND_CHARS	32	// 32 characters for extending

EqWString::EqWString()
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = NULL;
	Empty();
}

EqWString::~EqWString()
{
	Clear();
}

EqWString::EqWString(const wchar_t c)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = NULL;

	Assign( &c, 1 );
}

// convert from UTF8 string
EqWString::EqWString(const char* pszString, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = NULL;

	Assign( pszString, len );
}

EqWString::EqWString(const EqString& str, int nStart, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = NULL;

	Assign( str, nStart, len );
}

EqWString::EqWString(const wchar_t* pszString, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = NULL;

	Assign( pszString, len );
}

EqWString::EqWString(const EqWString &str, int nStart, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = NULL;

	Assign( str, nStart, len );
}

EqWString EqWString::Format(const wchar_t* pszFormat, ...)
{
	va_list argptr;
	va_start(argptr, pszFormat);
	int length = _vsnwprintf(nullptr, 0, pszFormat, argptr);
	va_end(argptr);

	EqWString newString;
	newString.Resize(length + 1, false);

	va_start(argptr, pszFormat);
	_vsnwprintf(newString.m_pszString, length+1, pszFormat, argptr);
	newString.m_nLength = length;
	va_end(argptr);

	return newString;
}

// data for printing
const wchar_t* EqWString::GetData() const
{
	if(!m_pszString)
		return L"";

	return m_pszString;
}

// length of it
uint EqWString::Length() const
{
	return m_nLength;
}

// string allocated size in bytes
uint EqWString::GetSize() const
{
	return m_nAllocated;
}

// erases and deallocates data
void EqWString::Clear()
{
	delete [] m_pszString;
	m_pszString = NULL;
	m_nLength = 0;
	m_nAllocated = 0;
}

// empty the string, but do not deallocate
void EqWString::Empty()
{
	Resize(BASE_BUFFER, false);
}

// an internal operation of allocation/extend
bool EqWString::ExtendAlloc(int nSize)
{
	if((uint)nSize+1 > m_nAllocated)
	{
		if(!Resize( nSize + EXTEND_CHARS ))
			return false;
	}

	return true;
}

// just a resize
bool EqWString::Resize(uint nSize, bool bCopy)
{
	uint newSize = nSize+1;

	// make new and copy
	wchar_t* pszNewBuffer = new wchar_t[ newSize ];

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

			wcscpy( pszNewBuffer, m_pszString );
		}

		// now it's not needed
		delete [] m_pszString;

		m_pszString = NULL;
	}

	// assign
	m_pszString = pszNewBuffer;
	m_nAllocated = newSize;

	// update length
	m_nLength = wcslen( m_pszString );

	return true;
}

// string assignment with conversion (or setvalue)
void EqWString::Assign(const char* pszStr, int len)
{
	EqStringConv::utf8_to_wchar conv( (*this), pszStr, len);
}

void EqWString::Assign(const EqString &str, int nStart, int len)
{
	EqStringConv::utf8_to_wchar conv((*this), str.ToCString() + nStart);
}

// string assignment (or setvalue)
void EqWString::Assign(const wchar_t* pszStr, int len)
{
	if(pszStr == NULL)
	{
		m_pszString[0] = 0;
		m_nLength = 0;
		return;
	}

	int nLen = wcslen( pszStr );

	ASSERT(len <= nLen);

	if(len != -1)
		nLen = len;

	if( ExtendAlloc( nLen ) )
	{
		wcsncpy( m_pszString, pszStr, nLen );
		m_pszString[nLen] = 0;
		m_nLength = nLen;
	}
}

void EqWString::Assign(const EqWString &str, int nStart, int len)
{
	ASSERT(nStart >= 0);

	int nLen = str.Length();

	ASSERT(len <= nLen);

	if(len != -1)
		nLen = len;

	if( ExtendAlloc( nLen ) )
	{
		wcscpy( m_pszString+nStart, str.GetData() );
		m_pszString[nLen] = 0;
		m_nLength = nLen;
	}
}

void EqWString::Append(const wchar_t c)
{
	int nNewLen = m_nLength + 1;

	if( ExtendAlloc( nNewLen ) )
	{
		m_pszString[nNewLen-1] = c;
		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

// appends another string
void EqWString::Append(const wchar_t* pszStr, int nCount)
{
	if(pszStr == NULL)
		return;

	int nLen = wcslen( pszStr );

	ASSERT(nCount <= nLen);

	if(nCount != -1)
		nLen = nCount;

	int nNewLen = m_nLength + nLen;

	if( ExtendAlloc( nNewLen ) )
	{
		wcsncpy( (m_pszString + m_nLength), pszStr, nLen);
		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

void EqWString::Append(const EqWString &str)
{
	int nNewLen = m_nLength + str.Length();

	if( ExtendAlloc( nNewLen ) )
	{
		wcscpy( (m_pszString + m_nLength), str.GetData() );
		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

// inserts another string at position
void EqWString::Insert(const wchar_t* pszStr, int nInsertPos)
{
	if(pszStr == NULL)
		return;

	int nInsertCount = wcslen( pszStr );

	int nNewLen = m_nLength + nInsertCount;

	if( ExtendAlloc( nNewLen ) )
	{
		wchar_t* tmp = (wchar_t*)stackalloc(m_nLength - nInsertPos);
		wcscpy(tmp, &m_pszString[nInsertPos]);

		// copy the part to the far
		wcsncpy(&m_pszString[nInsertPos + nInsertCount], tmp, m_nLength - nInsertPos);

		// copy insertable
		wcsncpy(m_pszString + nInsertPos, pszStr, nInsertCount);

		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

void EqWString::Insert(const EqWString &str, int nInsertPos)
{
	int nNewLen = m_nLength + str.Length();

	if( ExtendAlloc( nNewLen ) )
	{
		wchar_t* tmp = (wchar_t*)stackalloc(m_nLength - nInsertPos);
		wcscpy(tmp, &m_pszString[nInsertPos]);

		// copy the part to the far
		wcsncpy(&m_pszString[nInsertPos + str.Length()], tmp, m_nLength - nInsertPos);

		// copy insertable
		wcsncpy(m_pszString + nInsertPos, str.GetData(), str.Length());

		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

// removes characters
void EqWString::Remove(uint nStart, uint nCount)
{
	wchar_t* temp = (wchar_t*)stackalloc( m_nAllocated*sizeof(wchar_t) );
	wcscpy(temp, m_pszString);

	wchar_t* pStr = m_pszString;

	uint realEnd = nStart+nCount;

	for(uint i = 0; i < m_nLength; i++)
	{
		if(i >= (uint)nStart && i < realEnd)
			continue;

		*pStr++ = temp[i];
	}
	*pStr = 0;

	int newLen = m_nLength-nCount;

	Resize( newLen );
}

// replaces characters
void EqWString::Replace( wchar_t whichChar, wchar_t to )
{
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
EqWString EqWString::Left(int nCount)
{
	return Mid(0, nCount);
}

EqWString EqWString::Right(int nCount)
{
	if ( (uint)nCount >= m_nLength )
		return (*this);

	return Mid( m_nLength - nCount, nCount );
}

EqWString EqWString::Mid(int nStart, int nCount)
{
	int n;
	EqWString result;

	n = m_nLength;
	if( n == 0 || nCount <= 0 || nStart >= n )
		return result;

	if( uint(nStart+nCount) >= m_nLength )
		nCount = n-nStart;

	result.Append( &m_pszString[nStart], nCount );

	return result;
}

// convert to lower case
EqWString EqWString::LowerCase()
{
	EqWString str(*this);
	xwcslwr(str.m_pszString);

	return str;
}

// convert to upper case
EqWString EqWString::UpperCase()
{
	EqWString str(*this);
	xwcsupr(str.m_pszString);

    return str;
}

// search, returns char index
int	EqWString::Find(const wchar_t* pszSub, bool bCaseSensetive, int nStart)
{
	if (!m_pszString)
		return -1;

	int nFound = -1;

	wchar_t* strStart = m_pszString+min((uint16)nStart,m_nLength);

	wchar_t* st = NULL;

	if(bCaseSensetive)
		st = wcsstr(strStart, pszSub);
	else
		st = xwcsistr(strStart, pszSub);
	 
	if(st)
		nFound = st-m_pszString;

	return nFound;
}

// searches for substring and replaces it
int EqWString::ReplaceSubstr(const wchar_t* find, const wchar_t* replaceTo, bool bCaseSensetive /*= false*/, int nStart /*= 0*/)
{
	// replace substring
	int foundStrIdx = Find(find, bCaseSensetive, nStart);
	if (foundStrIdx != -1)
		Assign(Left(foundStrIdx) + replaceTo + Mid(foundStrIdx + wcslen(find), Length()));

	return foundStrIdx;
}

/*
EqWString EqWString::StripFileExtension()
{
	for ( int i = m_nLength-1; i >= 0; i-- )
	{
		if ( m_pszString[i] == '.' )
		{
			EqWString str;
			str.Append( m_pszString, i );

			return str;
		}
	}

	return (*this);
}

EqWString EqWString::StripFileName()
{
	ASSERT(!"EqWString::StripFileName() not impletented");
	EqWString result(*this);
	//stripFileName(result.m_pszString);
	//result.m_nLength = strlen(result.m_pszString);

	return result;
}

EqWString EqWString::ExtractFileExtension()
{
	for ( int i = m_nLength-1; i >= 0; i-- )
	{
		if ( m_pszString[i] == '.' )
		{
			EqWString str;
			str.Append( &m_pszString[i+1] );

			return str;
		}
	}

	return EqWString();
}*/

// comparators
int	EqWString::Compare(const wchar_t* pszStr)
{
	return xwcscmp(m_pszString, pszStr);
}

int	EqWString::Compare(const EqWString &str)
{
	return xwcscmp(m_pszString, str.GetData());
}

int	EqWString::CompareCaseIns(const wchar_t* pszStr)
{
	return xwcsicmp(m_pszString, pszStr);
}

int	EqWString::CompareCaseIns(const EqWString &str)
{
	return xwcsicmp(m_pszString, str.GetData());
}
