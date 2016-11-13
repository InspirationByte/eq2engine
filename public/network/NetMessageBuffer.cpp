//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network message for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#include "NetMessageBuffer.h"
#include "utils/strtools.h"
#include "utils/KeyValues.h"

#ifdef LINUX
#include <wchar.h>
#endif // LINUX

namespace Networking
{

static int s_nMessageId = 0;

void DefaultProcess(CNetMessageBuffer* pMsg)
{
	Msg("Unknown message from client %d\n", pMsg->GetClientID());
}

CNetMessageBuffer::CNetMessageBuffer()
{
	m_nMsgLen = 0;
	m_nSubMessageId = -1;
	m_pInterface = NULL;
	m_nMsgPos = 0;
	m_nClientID = -1;
}

CNetMessageBuffer::CNetMessageBuffer(INetworkInterface* pInterfaceToReceive)
{
	m_nMsgLen = 0;
	m_nSubMessageId = -1;
	m_pInterface = pInterfaceToReceive;
	m_nMsgPos = 0;
	m_nClientID = -1;
}

CNetMessageBuffer::~CNetMessageBuffer()
{
	for(int i = 0; i < m_pMessage.numElem(); i++)
	{
		delete m_pMessage[i];
	}
}

void CNetMessageBuffer::ResetPos()
{
	m_nSubMessageId = 0;
	m_nMsgPos = 0;

	for(int i = 0; i < m_pMessage.numElem(); i++)
		m_pMessage[i]->pos = 0;
}

void CNetMessageBuffer::DebugWriteToFile(const char* fileprefix)
{
	char filename[260] = {0};

	for(int i = 0; i < m_pMessage.numElem(); i++)
	{
		strcpy(filename, varargs("%s_frag_%d", fileprefix, i));

		FILE* pFile = fopen(filename, "wb");
		if(pFile)
		{
			fwrite(m_pMessage[i]->data, m_pMessage[i]->len, 1, pFile);
			fclose(pFile);
		}
	}
}

bool CNetMessageBuffer::Send( int nFlags )
{
	for(int i = 0; i < m_pMessage.numElem(); i++)
	{
		if(m_pMessage[i]->msgId != -1)
			return true;
	}

	static netMessage_t message;

	message.header.clientid = m_nClientID;
	message.header.nfragments = m_pMessage.numElem();
	message.header.fragmentid = 0;
	message.header.messageid = s_nMessageId++;

	if(s_nMessageId >= 32766)
		s_nMessageId = 0;

	bool status = false;

	for(int i = 0; i < m_pMessage.numElem(); i++)
	{
		message.header.fragmentid = i;
		message.header.message_size = m_pMessage[i]->len;

		memcpy(message.data, m_pMessage[i]->data, m_pMessage[i]->len);

		m_pMessage[i]->confirmed = false;

		// send fragment
		status = m_pInterface->Send( &message, m_pMessage[i]->len, m_pMessage[i]->msgId, nFlags);

		if(!status)
			break;
	}

	return status;
}

int CNetMessageBuffer::GetOverallStatus() const
{
	for(int i = 0; i < m_pMessage.numElem(); i++)
	{
		if( !m_pMessage[i]->confirmed )
		{
			if(m_pMessage[i]->msgId == CUDP_MESSAGE_ID_ERROR)
				return DELIVERY_FAILED;

			return DELIVERY_IN_PROGRESS;
		}
	}

	return DELIVERY_SUCCESS;
}

void CNetMessageBuffer::SetMessageStatus(short msgId, int status)
{
	for(int i = 0; i < m_pMessage.numElem(); i++)
	{
		if(!m_pMessage[i]->confirmed && m_pMessage[i]->msgId == msgId)
		{
			if(status == DELIVERY_SUCCESS)
				m_pMessage[i]->confirmed = true;
			else
				m_pMessage[i]->msgId = CUDP_MESSAGE_ID_ERROR;
		}
	}
}

int	CNetMessageBuffer::GetClientID() const
{
	return m_nClientID;
}

void CNetMessageBuffer::GetClientInfo(sockaddr_in& addr, int& clientId) const
{
	clientId = m_nClientID;
	addr = m_address;
}

void CNetMessageBuffer::SetClientInfo(const sockaddr_in& addr, int clientID)
{
	m_nClientID = clientID;
	m_address = addr;
}

int CNetMessageBuffer::GetMessageLength() const
{
	return m_nMsgLen;
}

void CNetMessageBuffer::WriteString(const char* pszStr)
{
	int len = strlen(pszStr) + 1;
	WriteInt(len);
	WriteData((ubyte*)pszStr, len);
}

void CNetMessageBuffer::WriteString(const EqString& str)
{
	int len = str.Length() + 1;

	WriteInt( len );
	WriteData(str.GetData(), len);
}

void CNetMessageBuffer::WriteWString(const wchar_t* pszStr)
{
	int len = wcslen(pszStr) + 1;
	WriteInt(len);
	WriteData((ubyte*)pszStr, len*sizeof(wchar_t));
}

void CNetMessageBuffer::WriteKeyValues(kvkeybase_t* kbase)
{
	CMemoryStream stream;
	stream.Open(NULL, VS_OPEN_WRITE, 2048);
	KV_WriteToStream(&stream, kbase, 0, false);

	char zerochar = '\0';
	stream.Write(&zerochar, 1, 1);

	WriteInt(stream.Tell()+1);
	WriteData(stream.GetBasePointer(), stream.Tell()+1);
}

void CNetMessageBuffer::ReadKeyValues(kvkeybase_t* kbase)
{
	int len = ReadInt();

	char* data = new char[len];
	ReadData(data, len);

	KV_ParseSection(data, "netbuf", kbase, 0);

	delete [] data;
}

void CNetMessageBuffer::ReadString(char* pszDestStr)
{
	int len = ReadInt();
	ReadData((ubyte*)pszDestStr, len);
}

void CNetMessageBuffer::ReadWString(wchar_t* pszDestStr)
{
	int len = ReadInt();
	ReadData((ubyte*)pszDestStr, len*sizeof(wchar_t));
}

char* CNetMessageBuffer::ReadString(int& length)
{
	length = ReadInt();

	if(length)
	{
		char* pData = new char[length];

		ReadData((ubyte*)pData, length);

		return pData;
	}

	return NULL;
}

EqString CNetMessageBuffer::ReadString()
{
	int length = ReadInt();

	char* temp = new char[length];

	// not so safe
	ReadData((ubyte*)temp, length);

	EqString str(temp, length-1);

	delete [] temp;

	return str;
}

wchar_t* CNetMessageBuffer::ReadWString(int& length)
{
	length = ReadInt();

	if(length)
	{
		wchar_t* pData = new wchar_t[length];

		ReadData((ubyte*)pData, length*sizeof(wchar_t));

		return pData;
	}

	return NULL;
}

void CNetMessageBuffer::WriteData(const void* pData, int nBytes)
{
	int pos = 0;
	while(pos < nBytes)
	{
		ubyte* pDataW = (ubyte*)pData + pos;

		int copy_count = nBytes - pos;

		TryExtendSubMsg();

		int submsg_max = (MAX_SUBMESSAGE_LENGTH-m_pMessage[m_nSubMessageId]->pos);

		if(copy_count > submsg_max)
			copy_count = submsg_max;

		if(!WriteToCurrentSubMsg(pDataW, copy_count))
			m_nSubMessageId++;

		pos += copy_count;
	}

	m_nMsgPos += nBytes;
	m_nMsgLen += nBytes;
}

void CNetMessageBuffer::ReadData(void* pDst, int nBytes)
{
	if(m_nMsgLen == 0)
	{
		ASSERTMSG(false, "CNetMessageBuffer::ReadData - message empty!");
		memset(pDst, 0, nBytes);
		return;
	}

	if(m_nMsgPos + nBytes > m_nMsgLen )
	{
		ASSERTMSG(false, "CNetMessageBuffer::ReadData outbound!");
		memset(pDst, 0, nBytes);
		return;
	}

	int pos = 0;
	while(pos < nBytes)
	{
		ubyte* pDataW = (ubyte*)pDst + pos;
		int copy_count = nBytes - pos;

		if(m_nSubMessageId > m_pMessage.numElem()-1)
		{
			Msg("Message is invalid!\n");
			memset(pDataW, 0, copy_count);
			return;
		}

		int submsg_max = (MAX_SUBMESSAGE_LENGTH - m_pMessage[m_nSubMessageId]->pos);

		if(copy_count > submsg_max)
			copy_count = submsg_max;

		if(!ReadCurrentSubMsg(pDataW, copy_count))
			m_nSubMessageId++;

		pos += copy_count;
	}

	m_nMsgPos += nBytes;
}

void CNetMessageBuffer::WriteNetBuffer(CNetMessageBuffer* pMsg, bool fromCurPos, int nLength)
{
	if(fromCurPos)
	{
		if(nLength == -1)
			nLength = (pMsg->m_nMsgLen - pMsg->m_nMsgPos);

		ubyte* pTemp = (ubyte*)malloc(nLength);
		pMsg->ReadData(pTemp, nLength);

		// TODO: reset pMsg position back

		WriteData(pTemp, nLength);

		free(pTemp);
	}
	else
	{
		for(int i = 0; i < pMsg->m_pMessage.numElem(); i++)
			WriteData(pMsg->m_pMessage[i]->data, pMsg->m_pMessage[i]->len);
	}
}

void CNetMessageBuffer::TryExtendSubMsg()
{
	if((m_nSubMessageId < 0) || (m_nSubMessageId >= m_pMessage.numElem()))
	{
		submsg_t* newmsg = new submsg_t;
#ifdef DEBUG
		memset(newmsg->data, 0, sizeof(newmsg->data));
#endif // DEBUG

		// make a new message id
		m_nSubMessageId = m_pMessage.append( newmsg );
	}
}

bool CNetMessageBuffer::WriteToCurrentSubMsg(ubyte* pData, int nBytes)
{
	ubyte* ptr = m_pMessage[m_nSubMessageId]->data + m_pMessage[m_nSubMessageId]->pos;
	memcpy(ptr, pData, nBytes);
	m_pMessage[m_nSubMessageId]->len += nBytes;
	m_pMessage[m_nSubMessageId]->pos += nBytes;

	return (m_pMessage[m_nSubMessageId]->pos < MAX_SUBMESSAGE_LENGTH) ? true : false;
}

bool CNetMessageBuffer::ReadCurrentSubMsg(ubyte* pDst, int nBytes)
{
	ubyte* ptr = m_pMessage[m_nSubMessageId]->data + m_pMessage[m_nSubMessageId]->pos;
	memcpy(pDst, ptr, nBytes);
	m_pMessage[m_nSubMessageId]->pos += nBytes;

	return (m_pMessage[m_nSubMessageId]->pos < MAX_SUBMESSAGE_LENGTH) ? true : false;
}

}; // namespace Networking
