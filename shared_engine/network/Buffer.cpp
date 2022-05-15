//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network message for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#include "Buffer.h"
#include "utils/strtools.h"
#include "utils/KeyValues.h"
#include <wchar.h>

#define MAX_SUBMESSAGE_LENGTH UDP_CDP_MAX_MESSAGEPAYLOAD

namespace Networking
{

Buffer::Buffer()
{
	m_data.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 128);
}

Buffer::~Buffer()
{
}

void Buffer::ResetPos()
{
	m_data.Seek(0, VS_SEEK_SET);
}

void Buffer::WriteToStream(IVirtualStream* stream)
{
	stream->Write(m_data.GetBasePointer(), GetMessageLength(), 1);
}

int Buffer::GetMessageLength() const
{
	return m_data.Tell();
}

void Buffer::WriteString(const char* pszStr)
{
	uint16 len = strlen(pszStr) + 1;
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
	uint16 len = wcslen(pszStr) + 1;
	WriteInt16(len);
	WriteData((ubyte*)pszStr, len*sizeof(wchar_t));
}

void Buffer::WriteKeyValues(kvkeybase_t* kbase)
{
	CMemoryStream stream;
	stream.Open(NULL, VS_OPEN_WRITE, 2048);
	KV_WriteToStreamBinary(&stream, kbase);

	char zerochar = '\0';
	stream.Write(&zerochar, 1, 1);

	WriteInt(stream.Tell()+1);
	WriteData(stream.GetBasePointer(), stream.Tell()+1);
}

void Buffer::ReadKeyValues(kvkeybase_t* kbase)
{
	int len = ReadInt();

	char* data = PPNew char[len];
	ReadData(data, len);

	KV_ParseBinary(data, len, kbase);

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

		ReadData((ubyte*)pData, length);

		return pData;
	}

	return NULL;
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

	return NULL;
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
