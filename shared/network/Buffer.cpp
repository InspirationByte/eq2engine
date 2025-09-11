//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network message for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "Buffer.h"

namespace Networking
{

Buffer::Buffer() 
	: m_data(m_intData)
{
	m_data.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 128);
}

Buffer::Buffer(CMemoryStream& stream, int startOfs /*= -1*/)
	: m_data(stream)
{
	m_startOfs = startOfs == -1 ? m_data.Tell() : startOfs;
	if (startOfs != -1)
		m_data.Seek(startOfs, FS_SEEK_SET);
}

void Buffer::ResetPos()
{
	m_data.Seek(m_startOfs, FS_SEEK_SET);
}

void Buffer::WriteToStream(IFileStream* stream)
{
	stream->Write(m_data.GetBasePointer(), GetMessageLength(), 1);
}

int Buffer::GetMessageLength() const
{
	return m_data.Tell() - m_startOfs;
}

void Buffer::WriteString(const char* pszStr)
{
	uint16 len = CString::Length(pszStr) + 1;
	WriteInt16(len);
	WriteData((ubyte*)pszStr, len);
}

void Buffer::WriteString(const EqString& str)
{
	uint16 len = str.Length() + 1;

	WriteInt16( len );
	WriteData(str.GetData(), len);
}

void Buffer::WriteWString(const wchar_t* pszStr)
{
	uint16 len = CString::Length(pszStr) + 1;
	WriteInt16(len);
	WriteData((ubyte*)pszStr, len*sizeof(wchar_t));
}

void Buffer::WriteKeyValues(const KVSection& kbase)
{
	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_WRITE, nullptr, 8192);
	KeyValues::WriteBinary(&stream, kbase);

	char zerochar = '\0';
	stream.Write(&zerochar, 1, 1);

	WriteInt(stream.Tell()+1);
	WriteData(stream.GetBasePointer(), stream.Tell()+1);
}

void Buffer::ReadKeyValues(KVSection& kbase)
{
	int len = ReadInt();

	char* data = PPNew char[len];
	ReadData(data, len);

	CMemoryStream memstr(PP_SL);
	memstr.Open(FS_OPEN_READ, (ubyte*)data, len);
	KeyValues::ParseBinary(&memstr, kbase);

	delete [] data;
}

void Buffer::ReadString(char* pszDestStr)
{
	uint16 len = ReadInt16();
	ReadData((ubyte*)pszDestStr, len);
}

void Buffer::ReadWString(wchar_t* pszDestStr)
{
	uint16 len = ReadInt16();
	ReadData((ubyte*)pszDestStr, len*sizeof(wchar_t));
}

char* Buffer::ReadString(int& length)
{
	length = ReadInt16();

	if(length)
	{
		char* pData = PPNew char[length];
		ReadData(pData, length);
		return pData;
	}

	return nullptr;
}

EqString Buffer::ReadString()
{
	uint16 length = ReadInt16();

	char* temp = PPNew char[length];

	// not so safe
	ReadData((ubyte*)temp, length);

	EqString str(temp, length-1);

	delete [] temp;

	return str;
}

wchar_t* Buffer::ReadWString(int& length)
{
	length = ReadInt16();

	if(length)
	{
		wchar_t* pData = PPNew wchar_t[length];

		ReadData((ubyte*)pData, length*sizeof(wchar_t));

		return pData;
	}

	return nullptr;
}

void Buffer::WriteData(const void* pData, int nBytes)
{
	m_data.Write(pData, nBytes, 1);
}

void Buffer::ReadData(void* pDst, int nBytes)
{
	m_data.Read(pDst, nBytes, 1);
}

}; // namespace Networking
