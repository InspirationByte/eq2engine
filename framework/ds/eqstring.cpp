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
	PPFree(m_data);
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
	if (str.m_length == 0 && m_length == 0)
		return;

	if (str.m_size > m_size)
	{
		// move string with pointer as it has larger capacity
		m_size = str.m_size;
		m_length = str.m_length;
		m_data = str.m_data;

		str.m_data = nullptr;
	}
	else
	{
		// copy to the current buffer as it's large enough
		Assign(str);
		PPFree(str.m_data);
		str.m_data = nullptr;
	}

	str.m_size = 0;
	str.m_length = 0;
}

template<typename CH>
EqTStr<CH>& EqTStr<CH>::operator=(const EqTStr& other)
{
	this->Assign( other );
	return *this;
}

template<typename CH>
EqTStr<CH>& EqTStr<CH>::operator=(const StrRef& other)
{
	this->Assign(other);
	return *this;
}

template<typename CH>
EqTStr<CH>& EqTStr<CH>::operator=(EqTStr&& other) noexcept
{
	if (other.m_length == 0 && m_length == 0)
		return *this;

	if (other.m_size > m_size)
	{
		// move string with pointer as it has larger capacity
		PPFree(m_data);
		m_size = other.m_size;
		m_length = other.m_length;
		m_data = other.m_data;

		other.m_data = nullptr;
	}
	else
	{
		// copy to the current buffer as it's large enough
		Assign(other);
		PPFree(other.m_data);
	}

	other.m_size = 0;
	other.m_length = 0;
	other.m_data = nullptr;
	return *this;
}

template<typename CH>
EqTStr<CH>& EqTStr<CH>::operator=(const CH* pszStr)
{
	this->Assign( pszStr );
	return *this;
}

template<typename CH>
CH EqTStr<CH>::operator[](int idx) const
{
	ASSERT(idx >= 0 && idx <= m_length);
	return StrPtr()[idx];
}

template<typename CH>
EqTStrRef<CH> EqTStr<CH>::FormatF(const CH* pszFormat, ...)
{
	ASSERT(pszFormat);
	if (!pszFormat)
		return nullptr;

	EqTStrRef<CH> newString;
	va_list argptr;

	va_start(argptr, pszFormat);
	newString = FormatV(pszFormat, argptr);
	va_end(argptr);

	return newString;
}

template<typename CH>
EqTStrRef<CH> EqTStr<CH>::FormatV(const CH* pszFormat, va_list argptr)
{
	ASSERT(pszFormat);
	if (!pszFormat)
		return nullptr;

	EqTStr<CH>& newString = EqTStrRef<CH>::GetTempString(nullptr, 0);
	newString.ExtendAlloc(CString::Length(pszFormat) + 128, false);

	va_list varg;
	va_copy(varg, argptr);
	const int reqSize = CString::PrintFV(newString.m_data, newString.m_size, pszFormat, varg);
	va_end(varg);

	// formatting error
	if (reqSize < 0)
	{
		newString.Empty();
		return newString;
	}

	if (reqSize < newString.m_size)
	{
		newString.m_length = reqSize;
		return newString;
	}

	newString.Resize(reqSize + 1, false);
	newString.m_length = CString::PrintFV(newString.m_data, newString.m_size, pszFormat, argptr);

	return newString;
}

template<typename CH>
const CH* EqTStr<CH>::StrPtr() const
{
	static const CH s_zeroString[] = {0};
	return m_data ? m_data : s_zeroString;
}

template<typename CH>
bool EqTStr<CH>::IsValid() const
{
	return CString::Length(StrPtr()) == m_length;
}

// erases and deallocates data
template<typename CH>
void EqTStr<CH>::Clear()
{
	PPFree(m_data);
	m_data = nullptr;

	m_length = 0;
	m_size = 0;
}

// empty the string, but do not deallocate
template<typename CH>
void EqTStr<CH>::Empty()
{
	if (!m_data)
		return;
	*m_data = 0;
	m_length = 0;
}

// an internal operation of allocation/extend
template<typename CH>
bool EqTStr<CH>::ExtendAlloc(int nSize, bool keepContents)
{
	if(nSize+1 > m_size || m_data == nullptr)
	{
		nSize += STR_EXTEND_CHARS;
		if(!Resize( nSize - nSize % STR_EXTEND_CHARS, keepContents))
			return false;
	}

	return true;
}

// just a resize
template<typename CH>
bool EqTStr<CH>::Resize(int nSize, bool keepContents)
{
	const int newSize = max(STR_BASE_BUFFER, nSize + 1);
	if (!keepContents)
	{
		PPFree(m_data);
		m_data = nullptr;
		m_length = 0;
		m_size = 0;
	}

	CH* newBuffer = reinterpret_cast<CH*>(PPDReAlloc(m_data, newSize * sizeof(CH), EqStringSL));
	if (!newBuffer)
		return false;

	m_data = newBuffer;
	m_size = newSize;

	m_length = min(m_length, newSize - 1);
	m_data[m_length] = 0;

	return true;
}

// string assignment (or setvalue)
template<typename CH>
void EqTStr<CH>::Assign(const CH* pszStr, int len)
{
	if(pszStr == nullptr)
	{
		if(m_data)
			*m_data = 0;
		m_length = 0;
		return;
	}

	if (len == -1)
		len = CString::Length(pszStr);

	if(m_data == pszStr && len <= m_length)
	{
		m_length = len;
		m_data[len] = 0;
	}

	if( ExtendAlloc(len+1, false ) )
	{
		// source may be a part of this string
		if(pszStr != m_data)
			memmove(m_data, pszStr, len * sizeof(CH));
		m_data[len] = 0;
	}
	m_length = len;
}

template<typename CH>
void EqTStr<CH>::Assign(const EqTStr<CH>& str, int start, int len)
{
	return Assign(str.Ref(), start, len);
}

template<typename CH>
void EqTStr<CH>::Assign(StrRef str, int start, int len)
{
	if (!str)
	{
		if (m_data)
			*m_data = 0;
		m_length = 0;
		return;
	}
	const int strLen = str.Length();
	ASSERT(start >= 0 && start <= strLen);
	ASSERT(len < 0 || start + len <= strLen);

	const int maxLength = max(0, strLen - start);
	const int assignLen = (len < 0) ? maxLength : min(len, maxLength);
	if (!ExtendAlloc(assignLen + 1, false))
		return;

	// source may be a part of this string
	memmove(m_data, str.GetData() + start, assignLen * sizeof(CH));
	m_data[assignLen] = 0;
	m_length = assignLen;
}

template<typename CH>
void EqTStr<CH>::Append(const CH c)
{
	const int newLength = m_length + 1;
	if (!ExtendAlloc(newLength))
		return;

	m_data[newLength-1] = c;
	m_data[newLength] = 0;
	m_length = newLength;
}

// string length limited by maxCount, does not scan further than needed
template<typename CH>
static int StrLengthMax(const CH* str, int maxCount)
{
	int length = 0;
	while (length < maxCount && str[length])
		++length;
	return length;
}

// appends another string
template<typename CH>
void EqTStr<CH>::Append(const CH* pszStr, int count)
{
	if(pszStr == nullptr || count == 0)
		return;

	Append(StrRef(pszStr, (count < 0) ? -1 : StrLengthMax(pszStr, count)));
}

template<typename CH>
void EqTStr<CH>::Append(const EqTStr<CH>& str)
{
	Append(str.Ref());
}

template<typename CH>
void EqTStr<CH>::Append(StrRef str)
{
	const int appendLength = str.Length();
	if (appendLength == 0)
		return;

	const CH* strPtr = str.GetData();
	if (IsSubstring(strPtr))
		strPtr = EqTStrRef<CH>::GetTempString(strPtr, appendLength);

	const int newLength = m_length + appendLength;
	if (!ExtendAlloc(newLength + 1))
		return;

	memcpy(m_data + m_length, strPtr, appendLength * sizeof(CH));
	m_data[newLength] = 0;
	m_length = newLength;
}

template<typename CH>
void EqTStr<CH>::AppendV(const CH* pszFormat, va_list argptr)
{
	ASSERT(pszFormat);
	if (!pszFormat)
		return;

	// arguments may point into this string, so it can't be extended or overwritten
	// while formatting; format into temporary string and append the result
	Append(FormatV(pszFormat, argptr));
}

template<typename CH>
void EqTStr<CH>::AppendF(const CH* pszFormat, ...)
{
	ASSERT(pszFormat);
	if (!pszFormat)
		return;

	va_list argptr;
	va_start(argptr, pszFormat);
	AppendV(pszFormat, argptr);
	va_end(argptr);
}

template<typename CH>
bool EqTStr<CH>::MakeInsertSpace(int startPos, int count)
{
	const int newLength = m_length + count;
	if (!ExtendAlloc(newLength + 1))
		return false;

	CH* insStartPtr = m_data + startPos;
	CH* insEndPtr = m_data + startPos + count;

	// move the right part of the string further
	memmove(insEndPtr, insStartPtr, sizeof(insStartPtr[0]) * (int(m_length) - startPos));

	m_data[newLength] = 0;
	m_length = newLength;

	return true;
}

// inserts another string at position
template<typename CH>
void EqTStr<CH>::Insert(const CH* pszStr, int pos, int count)
{
	if(pszStr == nullptr || count == 0)
		return;

	Insert(StrRef(pszStr, (count < 0) ? -1 : StrLengthMax(pszStr, count)), pos);
}

template<typename CH>
void EqTStr<CH>::Insert(const EqTStr<CH>&str, int pos)
{
	ASSERT(str.IsValid());
	Insert(str.Ref(), pos);
}

template<typename CH>
void EqTStr<CH>::Insert(StrRef str, int pos)
{
	const int insertLength = str.Length();
	if (insertLength == 0)
		return;

	ASSERT(pos >= 0);
	ASSERT(pos <= Length());

	const CH* strPtr = str.GetData();
	if (IsSubstring(strPtr))
		strPtr = EqTStrRef<CH>::GetTempString(strPtr, insertLength);

	if (!MakeInsertSpace(pos, insertLength))
		return;

	// copy the inserted string in to the middle
	memcpy(m_data + pos, strPtr, insertLength * sizeof(CH));

	ASSERT(IsValid());
}

// removes characters
template<typename CH>
void EqTStr<CH>::Remove(int start, int count)
{
	if (!m_data || count <= 0)
		return;

	ASSERT(start >= 0);
	ASSERT(start + count <= m_length);
	if (start < 0 || start + count > m_length)
		return;

	CH* remStartPtr = m_data + start;
	const CH* remEndPtr = m_data + start + count;

	// move the right part of the string to the left
	memmove(remStartPtr, remEndPtr, sizeof(remStartPtr[0]) * (int(m_length) - (start + count)));

	// put terminator
	const int newLength = m_length - count;
	m_data[newLength] = 0;
	m_length = newLength;
}

// replaces characters
template<typename CH>
void EqTStr<CH>::ReplaceChar(CH whichChar, CH to)
{
	if (whichChar == 0 || to == 0) // can't replace to terminator
		return;

	if (!m_data)
		return;

	CH* str = m_data;
	for(int i = 0; i < m_length; i++)
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
	memcpy(m_data + foundStartPos, replaceTo, replaceLength * sizeof(CH));

	ASSERT(IsValid());

	return foundStartPos;
}

template<typename CH>
VSSize EqTStr<CH>::ReadString(IFileStream* stream, int length, EqTStr& output)
{
	output.Resize(length, false);
	stream->Read(output.m_data, sizeof(CH), length);
	output.m_data[length] = 0;
	output.m_length = length;
	return 1;
}

template<typename CH>
VSSize EqTStr<CH>::ReadString(IFileStream* stream, EqTStr<CH>& output)
{
	int length = 0;
	stream->Read(&length, 1, sizeof(length));
	return ReadString(stream, length, output);
}

template class EqTStr<char>;
template class EqTStr<wchar_t>;