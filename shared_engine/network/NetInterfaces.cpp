//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network components for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "NetInterfaces.h"
#include "core/DebugInterface.h"
#include "core/ConVar.h"

namespace Networking
{

DECLARE_CVAR(net_compress, 1, "Compress network traffic", CV_ARCHIVE)
DECLARE_CVAR_CLAMP(net_compress_level, 1, 1, 9, "Compression ratio", CV_ARCHIVE)

//-------------------------------------------------------------------------------------------------------

void INetworkInterface::Update( int timeMs, CDPRecvPipe_fn func, void* recvObj )
{
	ASSERT_MSG(m_pSocket != NULL, "INetworkInterface - not initialized.");

	m_pSocket->UpdateSendQueue( timeMs, func, recvObj);
	m_pSocket->UpdateRecieve( timeMs, func, recvObj);
}

//
// This decompresses recieved message\fragment
//
bool INetworkInterface::PreProcessRecievedMessage( ubyte* data, int size, netMessage_t& out )
{
	netMessage_t* msg = (netMessage_t*)data;

	out.header = msg->header;

	// decompress message and put to &out
	
	if( msg->header.compressed_size > 0 )
	{
		uLongf decompSize = msg->header.message_size;
		Bytef* decompData = (Bytef*)PPAlloc( decompSize );

		int z_result = uncompress(decompData, &decompSize, msg->data, msg->header.compressed_size );

		if(z_result == Z_OK)
			memcpy(out.data, decompData, decompSize);

		PPFree( decompData );

		if(z_result != Z_OK)
			return false;
	}
	else
	{
		memcpy(out.data, msg->data, msg->header.message_size);
	}

	return true;
}

bool INetworkInterface::Send( netMessage_t* msg, int msg_size, short& msg_id, int flags )
{
	msg->header.compressed_size = 0;

	if(net_compress.GetBool())
	{
		uLongf compSize = (msg->header.message_size * 1.1) + 12;
		Bytef* compData = (Bytef*)PPAlloc( compSize );

		int z_result = compress2(compData,&compSize,(ubyte*)msg->data, msg->header.message_size, net_compress_level.GetInt());

		if(z_result == Z_OK)
		{
			msg_size = compSize;
			msg->header.compressed_size = compSize;
			memcpy(msg->data, compData, compSize);
		}
		else
		{
			PPFree( compData );
			return false;
		}

		PPFree( compData );
	}

	return InternalSend(msg, msg_size, msg_id, flags);
}

//-------------------------------------------------------------------------------------------------------
// Server
//-------------------------------------------------------------------------------------------------------

CNetworkServer::CNetworkServer()
{
	m_lastclient.client_id = -1;
	m_pSocket = NULL;
}

CNetworkServer::~CNetworkServer()
{
	Shutdown();
}

bool CNetworkServer::Init( int portNumber )
{
	m_pSocket = PPNew CEqRDPSocket();

	if(!m_pSocket->Init( portNumber ))
	{
		MsgError("Could not initialize socket.\n");
		return false;
	}

	return true;
}

void CNetworkServer::Shutdown()
{
	if(m_pSocket)
		delete m_pSocket;

	m_pSocket = NULL;
}

netPeer_t* CNetworkServer::GetLastClient()
{
	return &m_lastclient;
}

netPeer_t* CNetworkServer::GetClientByAddr(sockaddr_in* addr)
{
	for(int i = 0; i < m_clients.numElem(); i++)
	{
		if(!m_clients[i])
			continue;

		if(NETCompareAdr(m_clients[i]->client_addr, *addr))
			return m_clients[i];
	}

	return NULL;
}

netPeer_t* CNetworkServer::GetClientById(int id)
{
	for(int i = 0; i < m_clients.numElem(); i++)
	{
		if(!m_clients[i])
			continue;

		if(m_clients[i]->client_id == id)
			return m_clients[i];
	}

	return NULL;
}

void CNetworkServer::RemoveClientById(int id)
{
	for(int i = 0; i < m_clients.numElem(); i++)
	{
		if(!m_clients[i])
			continue;

		if(m_clients[i]->client_id == id)
		{
			delete m_clients[i];
			m_clients[i] = NULL;
		}
	}
}

bool CNetworkServer::HasPeersToSend() const
{
	for(int i = 0; i < m_clients.numElem(); i++)
	{
		if(!m_clients[i])
			continue;

		return true;
	}

	return false;
}

bool CNetworkServer::OnRecieved( netMessage_t* msg, const sockaddr_in& from )
{
	bool bResult = false;

	// this is only used if client is not connected
	if( msg->header.clientid == -1 )
	{
		m_lastclient.client_addr = from;
		m_lastclient.client_id = -1;
		bResult = true;
	}
	else
	{
		if(msg->header.clientid >= 0 && (msg->header.clientid < m_clients.numElem()))
		{
			for(int i = 0; i < m_clients.numElem(); i++)
			{
				if(!m_clients[i])
					continue;

				if(NETCompareAdr(m_clients[i]->client_addr, from) && m_clients[i]->client_id == msg->header.clientid)
				{
					m_lastclient.client_addr = m_clients[i]->client_addr;
					m_lastclient.client_id = m_clients[i]->client_id;
					bResult = true;
					break;
				}
			}
		}
	}

	return bResult;
}

bool CNetworkServer::InternalSend( netMessage_t* msg, int msg_size, short& msg_id, int flags)
{
	netPeer_t* pClient;

	if(msg->header.clientid == NM_SENDTOALL)
	{
		int numClientsToSend = 0;
		int numSent = 0;

		for(int i = 0; i < m_clients.numElem(); i++)
		{
			if(!m_clients[i])
				continue;

			msg->header.clientid = m_clients[i]->client_id;
			numSent += InternalSend(msg, msg_size, msg_id, flags) ? 1 : 0;

			numClientsToSend++;
		}

		return (numClientsToSend > 0) && (numSent > 0);
	}
	else
	{
		if(msg->header.clientid != NM_SENDTOLAST)
		{
			for(int i = 0; i < m_clients.numElem(); i++)
			{
				if(!m_clients[i])
					continue;

				if(m_clients[i]->client_id == msg->header.clientid)
				{
					pClient = m_clients[i];
					break;
				}
			}
		}
		else
			pClient = &m_lastclient;

		if(!pClient)
		{
			MsgError("CNetworkServer::Send: no client\n");
			return false;
		}

		int sended = m_pSocket->Send((char *)msg, msg_size + NETMESSAGE_HDR, &pClient->client_addr, msg_id, flags );

		sended -= NETMESSAGE_HDR;

		if( sended != msg_size )
		{
			MsgError("Bad sent message (%d versus %d)\n", msg_size, sended);
			return false;
		}

		if(flags & NETSERVER_FLAG_REMOVECLIENT)
		{
			RemoveClientById(pClient->client_id);
		}
	}

	return true;
}

int CNetworkServer::AddClient( netPeer_t* client_from_msg )
{
	// try find free slot
	for(int i = 0; i < m_clients.numElem(); i++)
	{
		if(!m_clients[i])
		{
			m_clients[i] = client_from_msg;
			m_clients[i]->client_id = i;
			return i;
		}
	}

	// insert to new slot
	int clid = m_clients.append( client_from_msg );

	client_from_msg->client_id = clid;

	return clid;
}

int CNetworkServer::GetClientCount()
{
	return m_clients.numElem();
}

//-------------------------------------------------------------------------------------------------------
// Client
//-------------------------------------------------------------------------------------------------------

CNetworkClient::CNetworkClient()
{
	m_nClientID = -1;
	m_pSocket = NULL;
}

CNetworkClient::~CNetworkClient()
{
	Shutdown();
}

bool CNetworkClient::Connect( const char* pszAddress, int portNumber, int clientPort )
{
	m_pSocket = PPNew CEqRDPSocket();

	bool bState = false;

	for(int i = 0; i < 100; i++)
	{
		if(m_pSocket->Init( clientPort+i ))
		{
			bState = true;
			break;
		}
	}

	if(!bState)
	{
		MsgError("Could not initialize socket.\n");
		return false;
	}

	in_addr hostaddress;

	int a1, a2, a3, a4;
	if(sscanf(pszAddress, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4)
	{
#ifndef _WIN32
		uint8* host_ip = (uint8*)&hostaddress.s_addr;
		host_ip[0] = (uint8)a1;
		host_ip[1] = (uint8)a2;
		host_ip[2] = (uint8)a3;
		host_ip[3] = (uint8)a4;
#else
		hostaddress.S_un.S_un_b.s_b1 = (unsigned char)a1;
		hostaddress.S_un.S_un_b.s_b2 = (unsigned char)a2;
		hostaddress.S_un.S_un_b.s_b3 = (unsigned char)a3;
		hostaddress.S_un.S_un_b.s_b4 = (unsigned char)a4;
#endif // !_WIN32
	}
	else
	{
		hostent* hn;
		hn = gethostbyname( pszAddress );

		if(!hn)
		{
			MsgError("Cannot resolve hostname '%s'\n", pszAddress);
			Shutdown();
			return false;
		}

		hostaddress = *((struct in_addr *)(hn->h_addr));

#ifndef _WIN32
		uint8* host_ip = (uint8*)&hostaddress.s_addr;
		Msg("host address: %d.%d.%d.%d\n",	host_ip[0],
											host_ip[1],
											host_ip[2],
											host_ip[3]);
#else
		Msg("host address: %d.%d.%d.%d\n",	hostaddress.S_un.S_un_b.s_b1,
											hostaddress.S_un.S_un_b.s_b2,
											hostaddress.S_un.S_un_b.s_b3,
											hostaddress.S_un.S_un_b.s_b4);
#endif // !_WIN32


	}

	//m_szConnAddr = pszAddress;

	// Clear out server struct
	memset((void *)&m_Server, 0, sizeof(sockaddr_in));

	// Set family and port
	m_Server.sin_family = AF_INET;
	m_Server.sin_port = htons( portNumber );

	// Set server address
	m_Server.sin_addr = hostaddress;

	return true;
}

void CNetworkClient::Shutdown()
{
	if(m_pSocket)
		delete m_pSocket;

	m_pSocket = NULL;
}

bool CNetworkClient::OnRecieved( netMessage_t* msg, const sockaddr_in& from )
{
	// set to server
	msg->header.clientid = NM_SERVER;

	return true;
}

bool CNetworkClient::InternalSend( netMessage_t* msg, int msg_size, short& msg_id, int flags )
{
	msg->header.clientid = m_nClientID;

	int sended = m_pSocket->Send((char *)msg, msg_size + NETMESSAGE_HDR, &m_Server, msg_id, flags );

	sended -= NETMESSAGE_HDR;

	if( sended != msg_size )
	{
		MsgError("Bad sent message (%d versus %d)\n", msg_size, sended);
		return false;
	}

	return true;
}

void CNetworkClient::SetClientID(int nClientID)
{
	m_nClientID = nClientID;
}

int CNetworkClient::GetClientID()
{
	return m_nClientID;
}

}; // namespace Networking
