//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//
//				Some things was lovely hardcoded (like m_nLength)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqstring.h"

#ifdef _WIN32
#define xstricmp stricmp
#else
#define xstricmp strcasecmp
#endif // _WIN32

#ifdef _MSC_VER
#pragma warning(disable: 4267)
#endif

#define EXTEND_CHARS	32	// 32 characters for extending
#define EQSTRING_BASE_BUFFER	32

const EqString EqString::EmptyStr;
static const PPSourceLine EqStringSL = PPSourceLine::Make(nullptr, 0);

EqString::EqString()
{
	Empty();
}

EqString::~EqString()
{
	Clear();
}

EqString::EqString(const char* pszString, int len)
{
	Assign( pszString, len );
}

EqString::EqString(const EqString &str, int nStart, int len)
{
	Assign( str, nStart, len );
}

EqString::EqString(EqString&& str) noexcept
{
	m_nAllocated = str.m_nAllocated;
	m_nLength = str.m_nLength;
	m_pszString = str.m_pszString;
	str.m_nAllocated = 0;
	str.m_nLength = 0;
	str.m_pszString = nullptr;
}

EqString::EqString(const wchar_t* pszString, int len)
{
	Assign( pszString, len );
}

EqString::EqString(const EqWString &str, int nStart, int len)
{
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

	va_list varg;
	va_copy(varg, argptr);
	const int reqSize = vsnprintf(newString.m_pszString, newString.m_nAllocated, pszFormat, varg);

	if (reqSize < newString.m_nAllocated)
	{
		newString.m_nLength = reqSize;
		return newString;
	}

	newString.Resize(reqSize + 1, false);
	newString.m_nLength = vsnprintf(newString.m_pszString, newString.m_nAllocated, pszFormat, argptr);

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
uint16 EqString::Length() const
{
	return m_nLength;
}

bool EqString::IsValid() const
{
	// check the data is non overflowing
	return strlen(m_pszString) <= m_nLength;
}

// string allocated size in bytes
uint16 EqString::GetSize() const
{
	return m_nAllocated;
}

// erases and deallocates data
void EqString::Clear()
{
	SAFE_DELETE_ARRAY(m_pszString);

	m_nLength = 0;
	m_nAllocated = 0;
}

// empty the string, but do not deallocate
void EqString::Empty()
{
	*m_pszString = 0;
	m_nLength = 0;
}

// an internal operation of allocation/extend
bool EqString::ExtendAlloc(int nSize, bool bCopy)
{
	if(nSize+1u > m_nAllocated || m_pszString == nullptr)
	{
		nSize += EXTEND_CHARS;
		if(!Resize( nSize - nSize % EXTEND_CHARS, bCopy ))
			return false;
	}

	return true;
}

// just a resize
bool EqString::Resize(int nSize, bool bCopy)
{
	const int newSize = max(EQSTRING_BASE_BUFFER, nSize + 1);

	// make new and copy
	char* pszNewBuffer = PPNewSL(EqStringSL) char[newSize];

	// allocation error!
	if (!pszNewBuffer)
		return false;

	pszNewBuffer[0] = 0;

	// copy and remove old if available
	if (m_pszString)
	{
		// if we have to copy
		if (bCopy && m_nLength)
		{
			// if string length if bigger, that the new alloc, cut off
			// for safety
			if (m_nLength > newSize)
				m_pszString[newSize] = 0;

			strcpy(pszNewBuffer, m_pszString);
		}

		// now it's not needed
		SAFE_DELETE_ARRAY(m_pszString);
	}

	// assign
	m_pszString = pszNewBuffer;
	m_nAllocated = newSize;

	if(nSize < m_nLength)
		m_nLength = strlen(m_pszString);

	return true;
}

// string assignment (or setvalue)
void EqString::Assign(const char* pszStr, int len)
{
	if(pszStr == nullptr)
	{
		if(m_pszString)
			*m_pszString = 0;
		m_nLength = 0;
		return;
	}

	if (len == -1)
		len = strlen(pszStr);

	if(m_pszString == pszStr && len <= m_nLength)
	{
		m_nLength = len;
		m_pszString[len] = 0;
	}

	if( ExtendAlloc(len+1, false ) )
	{
		if(pszStr != m_pszString)
			strncpy(m_pszString, pszStr, len);
		m_pszString[len] = 0;
	}
	m_nLength = len;
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
	if(pszStr == nullptr)
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
	if(pszStr == nullptr)
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
void EqString::Remove(int nStart, int nCount)
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

	char* strStart = m_pszString + min((uint16)nStart, m_nLength);

	char *st = nullptr;

	if(bCaseSensetive)
		st = strstr(strStart, pszSub);
	else
		st = xstristr(strStart, pszSub);

	if(st)
		nFound = (st - m_pszString);

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
		if ( m_pszString[i] == CORRECT_PATH_SEPARATOR || m_pszString[i] == INCORRECT_PATH_SEPARATOR )
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
		if ( m_pszString[i] == CORRECT_PATH_SEPARATOR || m_pszString[i] == INCORRECT_PATH_SEPARATOR)
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
	while(*begin && xisspace(*begin)) begin++;

	if(*begin == '\0')
		return EqString();

	const char* end = begin + strlen(begin) - 1;

	// trim whitespace from right
	while(end > begin && xisspace(*end))end--;

	return Mid(begin-m_pszString, end-begin+1);
}

EqString EqString::TrimChar(char ch, bool left, bool right) const
{
	char cch[2] = { ch, 0 };
	return TrimChar(cch, left, right);
}

EqString EqString::TrimChar(const char* ch, bool left, bool right) const
{
	const char* begin = m_pszString;

	auto ischr = [](const char* ch, char c) -> bool {
		while (*ch) { if (*ch++ == c) return true; }
		return false;
	};

	// trim whitespace from left
	while (*begin && ischr(ch, *begin)) ++begin;

	if (*begin == '\0')
		return EqString();

	const char* end = begin + strlen(begin) - 1;

	// trim whitespace from right
	while (end > begin && ischr(ch, *end)) --end;

	return Mid(begin - m_pszString, end - begin + 1);
}

void EqString::Path_FixSlashes() const
{
	if(!m_pszString)
		return;
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

size_t EqString::ReadString(IVirtualStream* stream, EqString& output)
{
	uint16 length = 0;
	stream->Read(&length, 1, sizeof(length));
	output.Resize(length, false);

	stream->Read(output.m_pszString, sizeof(char), length);
	output.m_nLength = length;
	return 1;
}
