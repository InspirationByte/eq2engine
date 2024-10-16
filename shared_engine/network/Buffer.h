//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network message for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct KVSection;

namespace Networking
{

class Buffer
{
public:
						Buffer();
						Buffer(CMemoryStream& stream, int startOfs = -1);
						virtual ~Buffer() = default;

	void				ResetPos();

	void				WriteToStream(IVirtualStream* stream);

	int					GetMessageLength() const;

	void				WriteByte(int8 n)			{WriteData(&n, sizeof(int8));}
	void				WriteUByte(ubyte n)			{WriteData(&n, sizeof(ubyte));}
	void				WriteInt16(int16 n)			{WriteData((ubyte*)&n, sizeof(int16));}
	void				WriteUInt16(uint16 n)		{WriteData((ubyte*)&n, sizeof(uint16));}
	void				WriteInt(int n)				{WriteData((ubyte*)&n, sizeof(int));}
	void				WriteUInt(uint n)			{WriteData((ubyte*)&n, sizeof(uint));}
	void				WriteBool(bool n)			{WriteData((ubyte*)&n, sizeof(bool));}
	void				WriteFloat(float fl)		{WriteData((ubyte*)&fl, sizeof(float));}

	void				WriteVector2D(const Vector2D& v)	{WriteData((ubyte*)&v, sizeof(Vector2D));}
	void				WriteVector3D(const Vector3D& v)	{WriteData((ubyte*)&v, sizeof(Vector3D));}
	void				WriteVector4D(const Vector4D& v)	{WriteData((ubyte*)&v, sizeof(Vector4D));}

	void				WriteString(const char* pszStr);
	void				WriteString(const EqString& str);

	void				WriteWString(const wchar_t* pszStr);

	void				WriteKeyValues(KVSection* kbase);

	int8				ReadByte()					{int8 r; ReadData(&r, sizeof(int8)); return r;}
	ubyte				ReadUByte()					{ubyte r; ReadData(&r, sizeof(ubyte)); return r;}
	int16				ReadInt16()					{int16 r; ReadData((ubyte*)&r, sizeof(int16)); return r;}
	uint16				ReadUInt16()				{uint16 r; ReadData((ubyte*)&r, sizeof(uint16)); return r;}
	int					ReadInt()					{int r; ReadData((ubyte*)&r, sizeof(int)); return r;}
	uint				ReadUInt()					{uint r; ReadData((ubyte*)&r, sizeof(uint)); return r;}
	bool				ReadBool()					{bool r; ReadData((ubyte*)&r, sizeof(bool)); return r;}
	float				ReadFloat()					{float r; ReadData((ubyte*)&r, sizeof(float)); return r;}

	Vector2D			ReadVector2D()				{Vector2D v;ReadData((ubyte*)&v, sizeof(Vector2D));return v;}
	Vector3D			ReadVector3D()				{Vector3D v;ReadData((ubyte*)&v, sizeof(Vector3D));return v;}
	Vector4D			ReadVector4D()				{Vector4D v;ReadData((ubyte*)&v, sizeof(Vector4D));return v;}

	void				ReadKeyValues(KVSection* kbase);

	void				ReadString(char* pszDestStr);
	void				ReadWString(wchar_t* pszDestStr);
	EqString			ReadString();

	char*				ReadString(int& length);
	wchar_t*			ReadWString(int& length);

	void				WriteData(const void* pData, int nBytes);
	void				ReadData(void* pDst, int nBytes);

	CMemoryStream&		GetData() { return m_data; }

protected:

	CMemoryStream		m_intData{ PP_SL };
	CMemoryStream&		m_data;
	int					m_startOfs{ 0 };
};

}; // namespace Networking
