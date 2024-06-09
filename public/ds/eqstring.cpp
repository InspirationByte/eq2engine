//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: String template
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqstring.h"

static const PPSourceLine EqStringSL = PPSourceLine::Make(nullptr, 0);

#ifdef _MSC_VER
#pragma warning(disable: 4267)
#endif

constexpr int STR_EXTEND_CHARS	= 32;
constexpr int STR_BASE_BUFFER	= 32;

template<typename CH>
const EqTStr<CH> EqTStr<CH>::EmptyStr;

template<typename CH>
EqTStr<CH>::~EqTStr()
{
	Clear();
}

template<typename CH>
EqTStr<CH>::EqTStr(const CH* pszString, int len)
{
	Assign( pszString, len );
}

template<typename CH>
EqTStr<CH>::EqTStr(const EqTStr<CH>& str, int start, int len)
{
	Assign(str, start, len);
}

template<typename CH>
EqTStr<CH>::EqTStr(StrRef str, int start, int len)
{
	Assign( str, start, len );
}

template<typename CH>
EqTStr<CH>::EqTStr(EqTStr<CH>&& str) noexcept
{
	m_nAllocated = str.m_nAllocated;
	m_nLength = str.m_nLength;
	m_pszString = str.m_pszString;
	str.m_nAllocated = 0;
	str.m_nLength = 0;
	str.m_pszString = nullptr;
}

template<typename CH>
EqTStr<CH> EqTStr<CH>::FormatF(const CH* pszFormat, ...)
{
	EqTStr<CH> newString;
	va_list argptr;

	va_start(argptr, pszFormat);
	newString = FormatV(pszFormat, argptr);
	va_end(argptr);

	return newString;
}

template<typename CH>
EqTStr<CH> EqTStr<CH>::FormatV(const CH* pszFormat, va_list argptr)
{
	EqTStr<CH> newString;
	newString.Resize(512, false);

	va_list varg;
	va_copy(varg, argptr);
	const int reqSize = CString::PrintFV(newString.m_pszString, newString.m_nAllocated, pszFormat, varg);

	if (reqSize < newString.m_nAllocated)
	{
		newString.m_nLength = reqSize;
		return newString;
	}

	newString.Resize(reqSize + 1, false);
	newString.m_nLength = CString::PrintFV(newString.m_pszString, newString.m_nAllocated, pszFormat, argptr);

	return newString;
}

template<typename CH>
const CH* EqTStr<CH>::StrPtr() const
{
	static const CH s_zeroString[] = {0};
	return m_pszString ? m_pszString : s_zeroString;
}

template<typename CH>
bool EqTStr<CH>::IsValid() const
{
	return CString::Length(StrPtr()) == m_nLength;
}

// erases and deallocates data
template<typename CH>
void EqTStr<CH>::Clear()
{
	SAFE_DELETE_ARRAY(m_pszString);

	m_nLength = 0;
	m_nAllocated = 0;
}

// empty the string, but do not deallocate
template<typename CH>
void EqTStr<CH>::Empty()
{
	if (!m_pszString)
		return;
	*m_pszString = 0;
	m_nLength = 0;
}

// an internal operation of allocation/extend
template<typename CH>
bool EqTStr<CH>::ExtendAlloc(int nSize, bool bCopy)
{
	if(nSize+1 > m_nAllocated || m_pszString == nullptr)
	{
		nSize += STR_EXTEND_CHARS;
		if(!Resize( nSize - nSize % STR_EXTEND_CHARS, bCopy ))
			return false;
	}

	return true;
}

// just a resize
template<typename CH>
bool EqTStr<CH>::Resize(int nSize, bool bCopy)
{
	const int newSize = max(STR_BASE_BUFFER, nSize + 1);

	// make new and copy
	CH* pszNewBuffer = PPNewSL(EqStringSL) CH[newSize];

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

			memcpy(pszNewBuffer, m_pszString, min(m_nAllocated, newSize) * sizeof(CH));
		}

		// now it's not needed
		SAFE_DELETE_ARRAY(m_pszString);
	}

	// assign
	m_pszString = pszNewBuffer;
	m_nAllocated = newSize;

	if(nSize < m_nLength)
		m_nLength = CString::Length(m_pszString);

	return true;
}

// string assignment (or setvalue)
template<typename CH>
void EqTStr<CH>::Assign(const CH* pszStr, int len)
{
	if(pszStr == nullptr)
	{
		if(m_pszString)
			*m_pszString = 0;
		m_nLength = 0;
		return;
	}

	if (len == -1)
		len = CString::Length(pszStr);

	if(m_pszString == pszStr && len <= m_nLength)
	{
		m_nLength = len;
		m_pszString[len] = 0;
	}

	if( ExtendAlloc(len+1, false ) )
	{
		if(pszStr != m_pszString)
			memcpy(m_pszString, pszStr, len * sizeof(CH));
		m_pszString[len] = 0;
	}
	m_nLength = len;
}

template<typename CH>
void EqTStr<CH>::Assign(const EqTStr<CH>& str, int start, int len)
{
	return Assign(str.Ref(), start, len);
}

template<typename CH>
void EqTStr<CH>::Assign(StrRef str, int start, int len)
{
	const int strLen = str.Length();
	ASSERT(start >= 0);
	ASSERT(start + len < strLen);

	const int assignLength = (len < 0) ? (strLen - start) : min(len, strLen);
	if (!ExtendAlloc(assignLength + 1, false))
		return;

	memcpy(m_pszString, str.GetData() + start, assignLength * sizeof(CH));
	m_pszString[assignLength] = 0;
	m_nLength = assignLength;
}

template<typename CH>
void EqTStr<CH>::Append(const CH c)
{
	const int newLength = m_nLength + 1;
	if (!ExtendAlloc(newLength))
		return;

	m_pszString[newLength-1] = c;
	m_pszString[newLength] = 0;
	m_nLength = newLength;
}

// appends another string
template<typename CH>
void EqTStr<CH>::Append(const CH* pszStr, int count)
{
	if(pszStr == nullptr || count == 0)
		return;

	const int strLen = CString::Length(pszStr);
	const int appendLength = (count < 0) ? strLen : min(count, strLen);

	const int newLength = m_nLength + appendLength;
	if (!ExtendAlloc(newLength + 1))
		return;

	memcpy(m_pszString + m_nLength, pszStr, appendLength * sizeof(CH));
	m_pszString[newLength] = 0;
	m_nLength = newLength;
}

template<typename CH>
void EqTStr<CH>::Append(const EqTStr<CH>& str)
{
	const int newLength = m_nLength + str.Length();
	if (!ExtendAlloc(newLength + 1))
		return;

	memcpy(m_pszString + m_nLength, str.GetData(), str.Length() * sizeof(CH));
	m_pszString[newLength] = 0;
	m_nLength = newLength;
}

template<typename CH>
void EqTStr<CH>::Append(StrRef str)
{
	Append(str.ToCString(), str.Length());
}

template<typename CH>
bool EqTStr<CH>::MakeInsertSpace(int startPos, int count)
{
	const int newLength = m_nLength + count;
	if (!ExtendAlloc(newLength + 1))
		return false;

	CH* insStartPtr = m_pszString + startPos;
	CH* insEndPtr = m_pszString + startPos + count;

	// move the right part of the string further
	memmove(insEndPtr, insStartPtr, sizeof(insStartPtr[0]) * (int(m_nLength) - startPos));

	m_pszString[newLength] = 0;
	m_nLength = newLength;

	return true;
}

// inserts another string at position
template<typename CH>
void EqTStr<CH>::Insert(const CH* pszStr, int pos, int count)
{
	if(pszStr == nullptr || count == 0)
		return;

	const int strLen = CString::Length(pszStr);
	ASSERT(pos >= 0);
	ASSERT(pos <= Length());

	const int insertLength = (count < 0) ? strLen : min(count, strLen);
	if (!MakeInsertSpace(pos, insertLength))
		return;

	// copy the inserted string in to the middle
	memcpy(m_pszString + pos, pszStr, insertLength * sizeof(CH));

	ASSERT(IsValid());
}

template<typename CH>
void EqTStr<CH>::Insert(const EqTStr<CH>&str, int pos)
{
	ASSERT(str.IsValid());
	Insert(str, pos, str.Length());
}

template<typename CH>
void EqTStr<CH>::Insert(StrRef str, int pos)
{
	Insert(str.GetData(), pos, str.Length());
}

// removes characters
template<typename CH>
void EqTStr<CH>::Remove(int start, int count)
{
	if (!m_pszString || count <= 0)
		return;

	ASSERT(start >= 0);
	ASSERT(start + count <= m_nLength);
	if (start < 0 || start + count > m_nLength)
		return;

	CH* remStartPtr = m_pszString + start;
	const CH* remEndPtr = m_pszString + start + count;

	// move the right part of the string to the left
	memmove(remStartPtr, remEndPtr, sizeof(remStartPtr[0]) * (int(m_nLength) - (start + count)));

	// put terminator
	const int newLength = m_nLength - count;
	m_pszString[newLength] = 0;
	m_nLength = newLength;
}

// replaces characters
template<typename CH>
void EqTStr<CH>::ReplaceChar(CH whichChar, CH to)
{
	if (whichChar == 0 || to == 0) // can't replace to terminator
		return;

	if (!m_pszString)
		return;

	CH* str = m_pszString;
	for(uint i = 0; i < m_nLength; i++)
	{
		if(*str == 0)
			break;
		if(*str == whichChar)
			*str = to;
		str++;
	}
}

// searches for substring and replaces it
template<typename CH>
int EqTStr<CH>::ReplaceSubstr(StrRef find, StrRef replaceTo, bool caseSensivite /*= false*/, int start /*= 0*/)
{
	// replace substring
	const int foundStartPos = Find(find, caseSensivite, start);
	if (foundStartPos == -1)
		return -1;

	const int findLength = find.Length();
	const int replaceLength = replaceTo.Length();

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
	memcpy(m_pszString + foundStartPos, replaceTo, replaceLength * sizeof(CH));

	ASSERT(IsValid());

	return foundStartPos;
}

template<typename CH>
size_t EqTStr<CH>::ReadString(IVirtualStream* stream, EqTStr<CH>& output)
{
	uint16 length = 0;
	stream->Read(&length, 1, sizeof(length));
	output.Resize(length, false);

	stream->Read(output.m_pszString, sizeof(CH), length);
	output.m_pszString[length] = 0;
	output.m_nLength = length;
	return 1;
}

template class EqTStr<char>;
template class EqTStr<wchar_t>;