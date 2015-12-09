//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network message for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGES_H
#define MESSAGES_H

#include "net_defs.h"
#include "NetInterfaces.h"

#include "DebugInterface.h"
#include <string.h>
#include <time.h>

#include "math/Vector.h"

#include "VirtualStream.h"

#ifndef NO_LUA
#include "luabinding/LuaBinding_Math.h"
#endif // NO_LUA

namespace Networking
{

// ------------------------------------------
// Network message stuff
// ------------------------------------------

#define MAX_SUBMESSAGE_LENGTH MAX_MESSAGE_LENGTH

struct submsg_t
{
	int		len;
	int		pos;
	ubyte	data[MAX_SUBMESSAGE_LENGTH];
};

// ------------------------------------------
// Network message, for fast send/receive
// and read data
// ------------------------------------------
class CNetMessageBuffer
{
public:
						CNetMessageBuffer();
						CNetMessageBuffer(INetworkInterface* pInterfaceToReceive);

						~CNetMessageBuffer();

	void				ResetPos();

	bool				Receive(int wait_timeout = 0);

	void				DebugWriteToFile(const char* fileprefix);

	bool				Send( short& outProtoMsgId, int nFlags = 0 );

	int					GetClientID() const;
	void				GetClientInfo(sockaddr_in& addr, int& clientId) const;
	void				SetClientInfo(const sockaddr_in& addr, int clientID);

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

	void				WriteKeyValues(kvkeybase_t* kbase);

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

	void				ReadKeyValues(kvkeybase_t* kbase);


	void				ReadString(char* pszDestStr);
	void				ReadWString(wchar_t* pszDestStr);
	EqString			ReadString();

	char*				ReadString(int& length);
	wchar_t*			ReadWString(int& length);

	void				WriteData(const void* pData, int nBytes);
	void				ReadData(void* pDst, int nBytes);

	void				WriteNetBuffer(CNetMessageBuffer* pMsg, bool fromCurPos = false, int nLength = -1);

protected:
	void				TryExtendSubMsg();

	bool				WriteToCurrentSubMsg(ubyte* pData, int nBytes);

	bool				ReadCurrentSubMsg(ubyte* pDst, int nBytes);

	int					m_nMsgLen;
	int					m_nMsgPos;
	int					m_nSubMessageId;

	DkList<submsg_t*>	m_pMessage;

	INetworkInterface*	m_pInterface;

	// client ID is now necessary for server
	int					m_nClientID;
	sockaddr_in			m_address;
};

}; // namespace Networking

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY( Networking::CNetMessageBuffer )

	OOLUA_MFUNC( WriteByte )
	OOLUA_MFUNC( WriteUByte )
	OOLUA_MFUNC( WriteInt16 )
	OOLUA_MFUNC( WriteUInt16 )
	OOLUA_MFUNC( WriteInt )
	OOLUA_MFUNC( WriteUInt )
	OOLUA_MFUNC( WriteBool )
	OOLUA_MFUNC( WriteFloat )

	OOLUA_MFUNC( WriteVector2D )
	OOLUA_MFUNC( WriteVector3D )
	OOLUA_MFUNC( WriteVector4D )

	OOLUA_MFUNC( ReadByte )
	OOLUA_MFUNC( ReadUByte )
	OOLUA_MFUNC( ReadInt16 )
	OOLUA_MFUNC( ReadUInt16 )
	OOLUA_MFUNC( ReadInt )
	OOLUA_MFUNC( ReadUInt )
	OOLUA_MFUNC( ReadBool )
	OOLUA_MFUNC( ReadFloat )

	OOLUA_MFUNC( ReadVector2D )
	OOLUA_MFUNC( ReadVector3D )
	OOLUA_MFUNC( ReadVector4D )

	//OOLUA_MFUNC( WriteString )
	//OOLUA_MEM_FUNC( std::string, ReadString )

	OOLUA_MFUNC( WriteNetBuffer )

	//OOLUA_MFUNC( WriteKeyValues )
	//OOLUA_MFUNC( ReadKeyValues )

	OOLUA_MFUNC_CONST( GetMessageLength )
	OOLUA_MFUNC_CONST( GetClientID )

OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // MESSAGES_H
