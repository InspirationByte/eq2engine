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

EqString::~EqString()
{
	Clear();
}

EqString::EqString(const char* pszString, int len)
{
	Assign( pszString, len );
}

EqString::EqString(const EqString& str, int nStart, int len)
{
	Assign(str, nStart, len);
}

EqString::EqString(EqStringRef str, int nStart, int len)
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

const char* EqString::StrPtr() const
{
	static const char s_zeroString[] = "";
	return m_pszString ? m_pszString : s_zeroString;
}

bool EqString::IsValid() const
{
	return strlen(StrPtr()) == m_nLength;
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
	if (!m_pszString)
		return;
	*m_pszString = 0;
	m_nLength = 0;
}

// an internal operation of allocation/extend
bool EqString::ExtendAlloc(int nSize, bool bCopy)
{
	if(nSize+1 > m_nAllocated || m_pszString == nullptr)
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

void EqString::Assign(const EqString& str, int nStart, int len)
{
	return Assign(str.Ref(), nStart, len);
}

void EqString::Assign(EqStringRef str, int nStart, int len)
{
	ASSERT(nStart >= 0);

	int nLen = str.Length();
	ASSERT(len <= nLen);

	if(len != -1)
		nLen = len;

	if (!ExtendAlloc(nLen + 1, false))
		return;

	strcpy( m_pszString+nStart, str.GetData() );
	m_pszString[nLen] = 0;
	m_nLength = nLen;
}

// string assignment (or setvalue)
void EqString::Assign(const wchar_t* pszStr, int len)
{
	EqStringConv::CUTF8Conv(*this, pszStr, len );
}

void EqString::Assign(const EqWString &str, int nStart, int len)
{
	EqStringConv::CUTF8Conv(*this, str.ToCString() + nStart, len);
}

void EqString::Append(const char c)
{
	const int nNewLen = m_nLength + 1;
	if (!ExtendAlloc(nNewLen))
		return;

	m_pszString[nNewLen-1] = c;
	m_pszString[nNewLen] = 0;
	m_nLength = nNewLen;
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

	const int nNewLen = m_nLength + nLen;
	if (!ExtendAlloc(nNewLen + 1))
		return;

	strncpy( (m_pszString + m_nLength), pszStr, nLen);
	m_pszString[nNewLen] = 0;
	m_nLength = nNewLen;
}

void EqString::Append(const EqString &str)
{
	const int nNewLen = m_nLength + str.Length();
	if (!ExtendAlloc(nNewLen + 1))
		return;

	strcpy( (m_pszString + m_nLength), str.GetData() );
	m_pszString[nNewLen] = 0;
	m_nLength = nNewLen;
}

void EqString::Append(EqStringRef str)
{
	Append(str.ToCString(), str.Length());
}

bool EqString::MakeInsertSpace(int startPos, int count)
{
	const int newLength = m_nLength + count;
	if (!ExtendAlloc(newLength + 1))
		return false;

	char* insStartPtr = m_pszString + startPos;
	char* insEndPtr = m_pszString + startPos + count;

	// move the right part of the string further
	memmove(insEndPtr, insStartPtr, sizeof(insStartPtr[0]) * (int(m_nLength) - startPos));

	m_pszString[newLength] = 0;
	m_nLength = newLength;

	return true;
}

// inserts another string at position
void EqString::Insert(const char* pszStr, int nInsertPos, int nInsertCount)
{
	if(pszStr == nullptr || nInsertCount == 0)
		return;

	const int strLen = strlen(pszStr);
	ASSERT(nInsertCount <= strLen);
	if (nInsertCount < 0)
		nInsertCount = strLen;

	if (!MakeInsertSpace(nInsertPos, nInsertCount))
		return;

	// copy the inserted string in to the middle
	memcpy(m_pszString + nInsertPos, pszStr, sizeof(m_pszString[0]) * nInsertCount);

	ASSERT(IsValid());
}

void EqString::Insert(const EqString &str, int nInsertPos)
{
	ASSERT(str.IsValid());
	Insert(str, nInsertPos, str.Length());
}

void EqString::Insert(EqStringRef str, int nInsertPos)
{
	Insert(str.GetData(), nInsertPos, str.Length());
}

// removes characters
void EqString::Remove(int nStart, int nCount)
{
	if (!m_pszString || nCount <= 0)
		return;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= m_nLength);
	if (nStart < 0 || nStart + nCount > m_nLength)
		return;

	char* remStartPtr = m_pszString + nStart;
	const char* remEndPtr = m_pszString + nStart + nCount;

	// move the right part of the string to the left
	memmove(remStartPtr, remEndPtr, sizeof(remStartPtr[0]) * (int(m_nLength) - (nStart + nCount)));

	// put terminator
	const int newLength = m_nLength - nCount;
	m_pszString[newLength] = 0;
	m_nLength = newLength;
}

// replaces characters
void EqString::ReplaceChar( char whichChar, char to )
{
	if (whichChar == 0 || to == 0) // can't replace to terminator
		return;

	if (!m_pszString)
		return;

	char* str = m_pszString;
	for(uint i = 0; i < m_nLength; i++)
	{
		if(*str == 0)
			break;
		if(*str == whichChar)
			*str = to;
		str++;
	}
}

// string extractors
EqString EqString::Left(int nCount) const
{
	return Mid(0, nCount);
}

EqString EqString::Right(int nCount) const
{
	if ( nCount >= m_nLength )
		return (*this);

	return Mid( m_nLength - nCount, nCount );
}

EqString EqString::Mid(int nStart, int nCount) const
{
	if (!m_pszString)
		return EqString::EmptyStr;

	ASSERT(nStart >= 0);
	ASSERT(nStart + nCount <= m_nLength);
	if (nStart < 0 || nStart + nCount > m_nLength)
		return EqString::EmptyStr;

	return EqString(&m_pszString[nStart], nCount);
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

// searches for substring and replaces it
int EqString::ReplaceSubstr(EqStringRef find, EqStringRef replaceTo, bool caseSensivite /*= false*/, int nStart /*= 0*/)
{
	// replace substring
	const int foundStartPos = Find(find, caseSensivite, nStart);
	if (foundStartPos == -1)
		return -1;

	const int findLength = strlen(find);
	const int replaceLength = strlen(replaceTo);

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
			return Right(m_nLength-1-i);
	}

	return EqString::EmptyStr;
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

	const char* cc = StrPtr();
	while(cc)
	{
		if( !CType::IsSpace(*cc) )
			out.Append(*cc++);
	}

	return out;
}

EqString EqString::TrimSpaces(bool left, bool right) const
{
	if(!m_pszString)
		return EqString::EmptyStr;

	const char* begin = m_pszString;

	// trim whitespace from left
	while(*begin && CType::IsSpace(*begin)) begin++;

	if(*begin == '\0')
		return EqString::EmptyStr;

	const char* end = begin + strlen(begin) - 1;

	// trim whitespace from right
	while(end > begin && CType::IsSpace(*end))end--;

	return Mid(begin-m_pszString, end-begin+1);
}

EqString EqString::TrimChar(char ch, bool left, bool right) const
{
	char cch[2] = { ch, 0 };
	return TrimChar(cch, left, right);
}

EqString EqString::TrimChar(const char* ch, bool left, bool right) const
{
	if (!m_pszString)
		return EqString::EmptyStr;

	const char* begin = m_pszString;

	auto ischr = [](const char* ch, char c) -> bool {
		while (*ch) { if (*ch++ == c) return true; }
		return false;
	};

	// trim whitespace from left
	while (*begin && ischr(ch, *begin)) ++begin;

	if (*begin == '\0')
		return EqString::EmptyStr;

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

size_t EqString::ReadString(IVirtualStream* stream, EqString& output)
{
	uint16 length = 0;
	stream->Read(&length, 1, sizeof(length));
	output.Resize(length, false);

	stream->Read(output.m_pszString, sizeof(char), length);
	output.m_pszString[length] = 0;
	output.m_nLength = length;
	return 1;
}
