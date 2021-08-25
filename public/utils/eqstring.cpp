//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//
//				Some things was lovely hardcoded (like m_nLength)
//////////////////////////////////////////////////////////////////////////////////

#include "eqstring.h"

#include <stdio.h>
#include <malloc.h>

#include "core/platform/Platform.h"

#include "strtools.h"
#include "math/math_common.h"



#ifdef PLAT_POSIX
#include <string.h>
#endif // PLAT_POSIX

#define EXTEND_CHARS	32	// 32 characters for extending

#ifdef _WIN32
#define xstricmp stricmp
#else
#define xstricmp strcasecmp
#endif // _WIN32

EqString::EqString()
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = nullptr;
	Empty();
}

EqString::~EqString()
{
	Clear();
}

EqString::EqString(const char c)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = nullptr;

	Assign( &c, 1 );
}

EqString::EqString(const char* pszString, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = nullptr;

	Assign( pszString, len );
}

EqString::EqString(const EqString &str, int nStart, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = nullptr;

	Assign( str, nStart, len );
}

EqString::EqString(const wchar_t* pszString, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = nullptr;

	Assign( pszString, len );
}

EqString::EqString(const EqWString &str, int nStart, int len)
{
	m_nLength = 0;
	m_nAllocated = 0;
	m_pszString = nullptr;

	Assign( str, nStart, len );
}

EqString EqString::Format(const char* pszFormat, ...)
{
	EqString newString;
	va_list argptr;

	va_start(argptr, pszFormat);
	newString = EqString::FormatVa(pszFormat, argptr);
	va_end(argptr);

	return newString;
}

EqString EqString::FormatVa(const char* pszFormat, va_list argptr)
{
	EqString newString;
	newString.Resize(512, false);

	int length = vsnprintf(newString.m_pszString, newString.m_nAllocated, pszFormat, argptr);

	if (length < newString.m_nAllocated)
	{
		newString.m_nLength = length;
		return newString;
	}

	newString.Resize(length + 1, false);

	vsnprintf(newString.m_pszString, newString.m_nAllocated, pszFormat, argptr);
	newString.m_nLength = length;

	return newString;
}

// data for printing
const char* EqString::GetData() const
{
	if(!m_pszString)
		return "";

	return m_pszString;
}

// length of it
uint EqString::Length() const
{
	return m_nLength;
}

bool EqString::IsValid() const
{
	// check the data is non overflowing
	return strlen(m_pszString) <= m_nLength;
}

// string allocated size in bytes
uint EqString::GetSize() const
{
	return m_nAllocated;
}

// erases and deallocates data
void EqString::Clear()
{
	delete [] m_pszString;

	m_pszString = nullptr;

	m_nLength = 0;
	m_nAllocated = 0;
}

// empty the string, but do not deallocate
void EqString::Empty()
{
	Resize(EQSTRING_BASE_BUFFER, false);
	m_pszString[0] = 0;
	m_nLength = 0;
}

// an internal operation of allocation/extend
bool EqString::ExtendAlloc(uint nSize, bool bCopy)
{
	if(nSize > m_nAllocated)
	{
		nSize += EXTEND_CHARS;

		if(!Resize( nSize - nSize % EXTEND_CHARS, bCopy ))
			return false;
	}

	return true;
}

// just a resize
bool EqString::Resize(uint nSize, bool bCopy)
{
	uint newSize = nSize;

	// use base buffer we requesting size less than base buffer
	if(newSize < EQSTRING_BASE_BUFFER)
		newSize = EQSTRING_BASE_BUFFER;

	char* pszNewBuffer = new char[newSize]; // make new
	pszNewBuffer[0] = 0;

	if( m_pszString )
	{
		// copy contents to the new buffer
		if(bCopy && m_nLength)
		{
			int minLength = min((uint16)(newSize-1), m_nLength);

			memcpy( pszNewBuffer, m_pszString, minLength);
			pszNewBuffer[minLength] = 0;
		}
		else if(!bCopy)
			pszNewBuffer[0] = 0;

		delete [] m_pszString;
		m_pszString = NULL;
	}

	// assign
	m_pszString = pszNewBuffer;
	m_nAllocated = newSize;

	// update length
	m_nLength = strlen( m_pszString );


	return true;
}

// string assignment (or setvalue)
void EqString::Assign(const char* pszStr, int len)
{
	if(pszStr == NULL)
	{
		m_pszString[0] = 0;
		m_nLength = 0;
		return;
	}

	int nLen = strlen( pszStr );

	ASSERT(len <= nLen);

	if(len != -1)
		nLen = len;

	if( ExtendAlloc( nLen+1, false ) )
	{
		strncpy( m_pszString, pszStr, nLen );
		m_pszString[nLen] = 0;
		m_nLength = nLen;
	}
}

void EqString::Assign(const EqString &str, int nStart, int len)
{
	ASSERT(nStart >= 0);

	int nLen = str.Length();

	ASSERT(len <= nLen);

	if(len != -1)
		nLen = len;

	if( ExtendAlloc( nLen+1, false ) )
	{
		strcpy( m_pszString+nStart, str.GetData() );
		m_pszString[nLen] = 0;
		m_nLength = nLen;
	}
}

// string assignment (or setvalue)
void EqString::Assign(const wchar_t* pszStr, int len)
{
	EqStringConv::wchar_to_utf8 conv( *this, pszStr, len );
}

void EqString::Assign(const EqWString &str, int nStart, int len)
{
	EqStringConv::wchar_to_utf8 conv(*this, str.ToCString()+nStart, len);
}

void EqString::Append(const char c)
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
void EqString::Append(const char* pszStr, int nCount)
{
	if(pszStr == NULL)
		return;

	int nLen = strlen( pszStr );

	ASSERT(nCount <= nLen);

	if(nCount != -1)
		nLen = nCount;

	int nNewLen = m_nLength + nLen;

	if( ExtendAlloc( nNewLen+1 ) )
	{
		strncpy( (m_pszString + m_nLength), pszStr, nLen);
		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

void EqString::Append(const EqString &str)
{
	int nNewLen = m_nLength + str.Length();

	if( ExtendAlloc( nNewLen+1 ) )
	{
		strcpy( (m_pszString + m_nLength), str.GetData() );
		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

// inserts another string at position
void EqString::Insert(const char* pszStr, int nInsertPos)
{
	if(pszStr == NULL)
		return;

	int nInsertCount = strlen( pszStr );

	int nNewLen = m_nLength + nInsertCount;

	if( ExtendAlloc( nNewLen+1 ) )
	{
		char* tmp = (char*)stackalloc(m_nLength - nInsertPos + 1);
		strcpy(tmp, &m_pszString[nInsertPos]);

		// copy the part to the far
		strncpy(&m_pszString[nInsertPos + nInsertCount], tmp, m_nLength - nInsertPos);

		// copy insertable
		strncpy(m_pszString + nInsertPos, pszStr, nInsertCount);

		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

void EqString::Insert(const EqString &str, int nInsertPos)
{
	int nNewLen = m_nLength + str.Length();

	if( ExtendAlloc( nNewLen+1 ) )
	{
		char* tmp = (char*)stackalloc(m_nLength - nInsertPos + 1);
		strcpy(tmp, &m_pszString[nInsertPos]);

		// copy the part to the far
		strncpy(&m_pszString[nInsertPos + str.Length()], tmp, m_nLength - nInsertPos);

		// copy insertable
		strncpy(m_pszString + nInsertPos, str.GetData(), str.Length());

		m_pszString[nNewLen] = 0;
		m_nLength = nNewLen;
	}
}

// removes characters
void EqString::Remove(uint nStart, uint nCount)
{
	char* temp = (char*)stackalloc( m_nAllocated );
	strcpy(temp, m_pszString);

	char* pStr = m_pszString;

	uint realEnd = nStart+nCount;

	for(uint i = 0; i < m_nLength; i++)
	{
		if(i >= nStart && i < realEnd)
			continue;

		*pStr++ = temp[i];
	}
	*pStr = 0;

	int newLen = m_nLength-nCount+1;

	Resize( newLen );
}

// replaces characters
void EqString::Replace( char whichChar, char to )
{
	char* pStr = m_pszString;

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
EqString EqString::Left(int nCount) const
{
	return Mid(0, nCount);
}

EqString EqString::Right(int nCount) const
{
	if ( (uint)nCount >= m_nLength )
		return (*this);

	return Mid( m_nLength - nCount, nCount );
}

EqString EqString::Mid(int nStart, int nCount) const
{
	uint n;
	EqString result;

	n = m_nLength;
	if( n == 0 || nCount <= 0 || (uint)nStart >= n )
		return result;

	if( uint(nStart+nCount) >= m_nLength )
		nCount = n-nStart;

	result.Assign( &m_pszString[nStart], nCount );

	return result;
}

// convert to lower case
EqString EqString::LowerCase() const
{
	EqString str(*this);
	xstrlwr(str.m_pszString);

	return str;
}

// convert to upper case
EqString EqString::UpperCase() const
{
	EqString str(*this);
	xstrupr(str.m_pszString);

	return str;
}

// search, returns char index
int	EqString::Find(const char* pszSub, bool bCaseSensetive, int nStart) const
{
	if (!m_pszString)
		return -1;

	int nFound = -1;

	char* strStart = m_pszString+min((uint16)nStart,m_nLength);

	char *st = NULL;

	if(bCaseSensetive)
		st = strstr(strStart, pszSub);
	else
		st = xstristr(strStart, pszSub);

	if(st)
		nFound = st - m_pszString;

	return nFound;
}

// searches for substring and replaces it
int EqString::ReplaceSubstr(const char* find, const char* replaceTo, bool bCaseSensetive /*= false*/, int nStart /*= 0*/)
{
	// replace substring
	int foundStrIdx = Find(find, bCaseSensetive, nStart);
	if (foundStrIdx != -1)
		Assign(Left(foundStrIdx) + replaceTo + Mid(foundStrIdx + strlen(find), Length()));

	return foundStrIdx;
}

// swaps two strings
void EqString::Swap(EqString& otherStr)
{
	// swap pointers if any of the strings are allocated
	QuickSwap(m_pszString, otherStr.m_pszString);

	QuickSwap(m_nLength, otherStr.m_nLength);
	QuickSwap(m_nAllocated, otherStr.m_nAllocated);
}

// other
EqString EqString::Path_Strip_Ext() const
{
	// search back
	for ( int i = m_nLength-1; i >= 0; i-- )
	{
		if ( m_pszString[i] == '.' )
		{
			return Left(i);
		}
	}

	return (*this);
}

EqString EqString::Path_Strip_Name() const
{
	// search back
	for ( int i = m_nLength-1; i >= 0; i-- )
	{
		if ( m_pszString[i] == '/' || m_pszString[i] == '\\' )
		{
			return Left(i+1);
		}
	}

	return (*this);
}

EqString EqString::Path_Strip_Path() const
{
	// search back
	for ( int i = m_nLength-1; i >= 0; i-- )
	{
		if ( m_pszString[i] == '/' || m_pszString[i] == '\\' )
		{
			return Right(m_nLength-1-i);
		}
	}

	return (*this);
}

EqString EqString::Path_Extract_Ext() const
{
	// search back
	for ( int i = m_nLength-1; i >= 0; i-- )
	{
		if ( m_pszString[i] == '.' )
		{
			return Right(m_nLength-1-i);
		}
	}

	return EqString();
}

EqString EqString::Path_Extract_Name() const
{
	return Path_Strip_Path();
}

EqString EqString::Path_Extract_Path() const
{
	return Path_Strip_Name();
}

EqString EqString::EatWhiteSpaces() const
{
	EqString out;

	char* cc = m_pszString;

	while(cc)
	{
		if( !isspace(*cc) )
			out.Append(*cc++);
	}

	return out;
}

EqString EqString::TrimSpaces(bool left, bool right) const
{
	const char* begin = m_pszString;

	// trim whitespace from left
	while(*begin && xisspace(*begin))begin++;

	if(*begin == '\0')
		return EqString();

	const char* end = begin + strlen(begin) - 1;

	// trim whitespace from right
	while(end > begin && xisspace(*end))end--;

	return Mid(begin-m_pszString, end-begin+1);
}

EqString EqString::TrimChar(char ch, bool left, bool right) const
{
	const char* begin = m_pszString;

	// trim whitespace from left
	while (*begin && *begin == ch)begin++;

	if (*begin == '\0')
		return EqString();

	const char* end = begin + strlen(begin) - 1;

	// trim whitespace from right
	while (end > begin && *end == ch)end--;

	return Mid(begin - m_pszString, end - begin + 1);
}

void EqString::Path_FixSlashes() const
{
	FixSlashes( m_pszString );
}

// comparators
int	EqString::Compare(const char* pszStr) const
{
	return strcmp(m_pszString, pszStr);
}

int	EqString::Compare(const EqString &str) const
{
	return strcmp(m_pszString, str.GetData());
}

int	EqString::CompareCaseIns(const char* pszStr) const
{
	return xstricmp(m_pszString, pszStr);
}

int	EqString::CompareCaseIns(const EqString &str) const
{
	return xstricmp(m_pszString, str.GetData());
}

int	EqString::GetMathingChars(const char* pszStr) const
{
	char* s1 = m_pszString;
	char* s2 = (char*)pszStr;

	int matching = 0;

	while(*s1++ == *s2++) {matching++;}

	return matching;
}

int	EqString::GetMathingChars(const EqString &str) const
{
	char* s1 = m_pszString;
	char* s2 = (char*)str.m_pszString;

	int matching = 0;

	while(*s1++ == *s2++) {matching++;}

	return matching;
}
