//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network components for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#include "NetInterfaces.h"
#include "DebugInterface.h"
#include "c_udp.h"
#include "IConCommandFactory.h"

#include <zlib.h>

namespace Networking
{

DECLARE_CMD(net_info, "Print networking information", 0)
{
	Msg("Equilibrium Engine network layer\n");
	Msg("  Protocol version: %d\n", EQUILIBRIUM_NETPROTOCOL_VERSION);
	Msg("  Uses IPv4\n");
	Msg("  Compression is not supported\n");
}

DECLARE_CVAR(net_compress, 1, "Compress network traffic", CV_ARCHIVE)
DECLARE_CVAR_CLAMP(net_compress_level, 1, 1, 9, "Compression ratio", CV_ARCHIVE)

//-------------------------------------------------------------------------------------------------------

void INetworkInterface::Update( int timeMs )
{
#ifdef USE_CUDP_SOCKET
	ASSERT(m_pSocket);

	m_pSocket->UpdateRecv( timeMs );
	m_pSocket->UpdateSend( timeMs );
#endif // USE_CUDP_SOCKET
}

CUDPSocket* INetworkInterface::GetSocket()
{
#ifdef USE_CUDP_SOCKET
	return m_pSocket;
#endif // USE_CUDP_SOCKET

	return NULL;
}

// basic message sending
bool INetworkInterface::Receive( netmessage_t* msg, int &msg_size, sockaddr_in* from )
{
	bool result = InternalReceive(msg, msg_size, from);

	if( result && msg->compressed_size > 0 )
	{
		uLongf decompSize = msg->message_size;
		Bytef* decompData = (Bytef*)malloc( decompSize );

		int z_result = uncompress(decompData, &decompSize, msg->message_bytes, msg->compressed_size );

		if(z_result == Z_OK)
		{
			msg_size = decompSize;
			msg->compressed_size = 0;
			memcpy(msg->message_bytes, decompData, decompSize);
		}
		else
		{
			result = false;
		}

		free( decompData );
	}

	return result;
}

bool INetworkInterface::Send( netmessage_t* msg, int msg_size, short& msg_id, int flags )
{
	msg->compressed_size = 0;

	if(net_compress.GetBool())
	{
		uLongf compSize = (msg->message_size * 1.1) + 12;
		Bytef* compData = (Bytef*)malloc( compSize );

		int z_result = compress2(compData,&compSize,(ubyte*)msg->message_bytes, msg->message_size, net_compress_level.GetInt());

		if(z_result == Z_OK)
		{
			msg_size = compSize;
			msg->compressed_size = compSize;
			memcpy(msg->message_bytes, compData, compSize);
		}
		else
		{
			free( compData );
			return false;
		}

		free( compData );
	}

	return InternalSend(msg, msg_size, msg_id, flags);
}

//-------------------------------------------------------------------------------------------------------
// Server
//-------------------------------------------------------------------------------------------------------

CNetworkServer::CNetworkServer()
{
	m_lastclient.client_id = -1;
#ifdef USE_CUDP_SOCKET
	m_pSocket = NULL;
#endif // USE_CUDP_SOCKET
}

CNetworkServer::~CNetworkServer()
{
	Shutdown();
}

bool CNetworkServer::Init( int portNumber )
{
#ifdef USE_CUDP_SOCKET
	m_pSocket = new CUDPSocket();

	if(!m_pSocket->Init( portNumber ))
	{
		MsgError("Could not initialize socket.\n");
		return false;
	}
#else

	// Open a datagram socket
	m_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if (m_socket == INVALID_SOCKET)
	{
		MsgError("Could not create socket.\n");
		return false;
	}

	unsigned long _true = 1;
	int	i = 1;

	if(ioctlsocket (m_socket, FIONBIO, &_true) == -1)
	{
		MsgError("cannot make socket non-blocking!\n");
		return false;
	}


	if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		MsgError("Cannot make socket broadcasting!\n");
		return false;
	}

	memset((void *)&m_Server, NULL, sizeof(struct sockaddr_in));

	m_Server.sin_family = AF_INET;
	m_Server.sin_port = htons(portNumber);

	// Set address automatically
	// Get host name of this computer
	char host_name[256];
	gethostname(host_name, sizeof(host_name));
	m_hostinfo = gethostbyname(host_name);

	if (m_hostinfo == NULL)
	{
		Msg("Unable to get hostinfo!\n");
		return false;
	}

	in_addr hostaddress;

	hostaddress = *((in_addr *)( m_hostinfo->h_addr ));

	m_Server.sin_addr.S_un.S_addr = INADDR_ANY;

	// Bind address to socket
	if( bind(m_socket, (struct sockaddr *)&m_Server, sizeof(struct sockaddr_in)) == SOCKET_ERROR )
	{
		closesocket(m_socket);

		int nErrorMsg = WSAGetLastError();
		MsgError( "Failed to bind socket to port %d (%s)!!!\n", portNumber, NETErrorString( WSAGetLastError() ) );
		return false;
	}

	Msg("\nServer Address: %u.%u.%u.%u:%d\n\n",	(unsigned char)hostaddress.S_un.S_un_b.s_b1,
												(unsigned char)hostaddress.S_un.S_un_b.s_b2,
												(unsigned char)hostaddress.S_un.S_un_b.s_b3,
												(unsigned char)hostaddress.S_un.S_un_b.s_b4,
												portNumber);
#endif // USE_CUDP_SOCKET
	return true;
}

void CNetworkServer::Shutdown()
{
#ifdef USE_CUDP_SOCKET
	if(m_pSocket)
		delete m_pSocket;

	m_pSocket = NULL;
#else
	closesocket(m_socket);
#endif // USE_CUDP_SOCKET
}

svclient_t* CNetworkServer::GetLastClient()
{
	return &m_lastclient;
}

svclient_t* CNetworkServer::GetClientByAddr(sockaddr_in* addr)
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

svclient_t* CNetworkServer::GetClientById(int id)
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

int CNetworkServer::GetIncommingMessages()
{
#ifdef USE_CUDP_SOCKET
	return m_pSocket->GetReceivedMessageCount();
#else
	struct timeval stTimeOut;

	fd_set stReadFDS;

	FD_ZERO(&stReadFDS);

	// Timeout of one second
	stTimeOut.tv_sec = 0;
	stTimeOut.tv_usec = 20 * 1000;

	FD_SET(m_socket, &stReadFDS);

	return select( m_socket+1, &stReadFDS, NULL, NULL, &stTimeOut);
#endif // USE_CUDP_SOCKET
}

bool CNetworkServer::InternalReceive( netmessage_t* msg, int& msg_size, sockaddr_in* from )
{
	sockaddr_in client;
	int cl_len = sizeof(struct sockaddr_in);

#ifdef USE_CUDP_SOCKET

	msg_size = m_pSocket->Recv((char*)msg, sizeof(netmessage_t), &client);
	msg_size -= NETMESSAGE_HDR;

#else
	msg_size = recvfrom(m_socket, (char*)msg, MAX_MESSAGE_LENGTH+NETMESSAGE_HDR, 0, (struct sockaddr *)&client, &cl_len);
	if( msg_size < 0 )
	{
		int nErrorMsg = WSAGetLastError();

		// skip WSAEWOULDBLOCK, WSAENOTCONN and WSAECONNRESET to avoid receiving ICMP error
		if( nErrorMsg != WSAEWOULDBLOCK && nErrorMsg != WSAENOTCONN && nErrorMsg != WSAECONNRESET )
		{
			Msg("receive error (%s)\n", NETErrorString(nErrorMsg));
			return false;
		}

		return false;
	}
#endif // USE_CUDP_SOCKET

	if(from)
		(*from) = client;

	bool bResult = false;

	// this is only used if client is not connected
	if( msg->clientid == -1 )
	{
		m_lastclient.client_addr = client;
		m_lastclient.client_id = -1;
		bResult = true;
	}
	else
	{
		if(msg->clientid >= 0 && (msg->clientid < m_clients.numElem()))
		{
			for(int i = 0; i < m_clients.numElem(); i++)
			{
				if(!m_clients[i])
					continue;

				if(NETCompareAdr(m_clients[i]->client_addr, client) && m_clients[i]->client_id == msg->clientid)
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

bool CNetworkServer::InternalSend( netmessage_t* msg, int msg_size, short& msg_id, int flags)
{
	svclient_t* pClient;

	if(msg->clientid == NM_SENDTOALL)
	{
		int numClientsToSend = 0;
		int numSent = 0;

		for(int i = 0; i < m_clients.numElem(); i++)
		{
			if(!m_clients[i])
				continue;

			msg->clientid = m_clients[i]->client_id;
			numSent += InternalSend(msg, msg_size, msg_id, flags) ? 1 : 0;

			numClientsToSend++;
		}

		return (numClientsToSend > 0) && (numSent > 0);
	}
	else
	{
		if(msg->clientid != NM_SENDTOLAST)
		{
			//pClient = m_clients[msg->clientid];

			for(int i = 0; i < m_clients.numElem(); i++)
			{
				if(!m_clients[i])
					continue;

				if(m_clients[i]->client_id == msg->clientid)
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

#ifdef USE_CUDP_SOCKET

		int nCUDPFlags = 0;
		nCUDPFlags |= (flags & NSFLAG_GUARANTEED) ? (CDPSEND_GUARANTEED | CDPSEND_PRESERVE_ORDER) : 0;
		nCUDPFlags |= (flags & NSFLAG_IMMEDIATE) ? CDPSEND_IMMEDIATE : 0;

		int sended = m_pSocket->Send((char *)msg, msg_size + NETMESSAGE_HDR, &pClient->client_addr, msg_id, nCUDPFlags );

		sended -= NETMESSAGE_HDR;

		if( sended != msg_size )
		{
			MsgError("Bad sent message (%d versus %d)\n", msg_size, sended);
			return false;
		}
#else
		int cl_len = sizeof(struct sockaddr_in);

		int sended = sendto(m_socket, (char *)msg, msg_size + NETMESSAGE_HDR, 0, (struct sockaddr *)&pClient->client_addr, cl_len);

		sended -= NETMESSAGE_HDR;

		if( sended != msg_size )
		{
			if( sended < 0 )
			{
				int nErrorMsg = WSAGetLastError();

				// skip WSAEWOULDBLOCK and WSAENOTCONN to avoid freeze
				if( nErrorMsg != WSAEWOULDBLOCK && nErrorMsg != WSAENOTCONN )
				{
					return false;
				}
			}

			MsgError("Bad sent message (%d versus %d)\n", msg_size, sended);
			return false;
		}
#endif // USE_CUDP_SOCKET
	}

	return true;
}

int CNetworkServer::AddClient( svclient_t* client_from_msg )
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

sockaddr_in CNetworkServer::GetSocketAddress()
{
#ifdef USE_CUDP_SOCKET
	return m_pSocket->GetAddress();
#else
	return m_Server;
#endif // USE_CUDP_SOCKET
}

//-------------------------------------------------------------------------------------------------------
// Client
//-------------------------------------------------------------------------------------------------------

CNetworkClient::CNetworkClient()
{
	m_nClientID = -1;

#ifdef USE_CUDP_SOCKET
	m_pSocket = NULL;
#endif // USE_CUDP_SOCKET
}

CNetworkClient::~CNetworkClient()
{
	Shutdown();
}

bool CNetworkClient::Connect( const char* pszAddress, int portNumber, int clientPort )
{
#ifdef USE_CUDP_SOCKET
	m_pSocket = new CUDPSocket();

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
#else
	// Open a datagram socket
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		MsgError("Could not create socket.\n");
		return false;
	}

	unsigned long _true = 1;
	int	i = 1;

	if(ioctlsocket (m_socket, FIONBIO, &_true) == -1)
	{
		Msg("cannot make socket non-locking!\n");
		return false;
	}


	if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		Msg("Set socket is broadcasting!\n");
		return false;
	}
	//---------------------------------------------------------------------------

	memset((void *)&m_Client, NULL, sizeof(struct sockaddr_in));

	// Set family and port
	m_Client.sin_family = AF_INET;
	m_Client.sin_port = htons(clientPort);

	// Set address automatically
	// Get host name of this computer
	char host_name[256];
	gethostname(host_name, sizeof(host_name));
	m_hostinfo = gethostbyname(host_name);

	// Check for NULL pointer
	if (m_hostinfo == NULL)
	{
		Msg("Unable to get hostinfo!\n");
		return false;
	}

	// Assign the address
	m_Client.sin_addr.S_un.S_un_b.s_b1 = m_hostinfo->h_addr_list[0][0];
	m_Client.sin_addr.S_un.S_un_b.s_b2 = m_hostinfo->h_addr_list[0][1];
	m_Client.sin_addr.S_un.S_un_b.s_b3 = m_hostinfo->h_addr_list[0][2];
	m_Client.sin_addr.S_un.S_un_b.s_b4 = m_hostinfo->h_addr_list[0][3];

	// Bind local address to socket
	if (bind(m_socket, (struct sockaddr *)&m_Client, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		MsgError("Failed to bind client socket!\n");
		return false;
	}

#endif // USE_CUDP_SOCKET

	in_addr hostaddress;

	int a1, a2, a3, a4;
	if(sscanf(pszAddress, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4)
	{
#ifdef LINUX
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
#endif // LINUX
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

#ifdef LINUX
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
#endif // LINUX


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
#ifdef USE_CUDP_SOCKET
	if(m_pSocket)
		delete m_pSocket;

	m_pSocket = NULL;
#else
	closesocket(m_socket);
#endif
}

bool CNetworkClient::Heartbeat()
{
	return false;
}

bool CNetworkClient::InternalReceive( netmessage_t* msg, int &msg_size, sockaddr_in* from)
{

	int cl_len = sizeof(struct sockaddr_in);

	sockaddr_in afrom;

	if(!from)
		from = &afrom;

#ifdef USE_CUDP_SOCKET
	msg_size = m_pSocket->Recv( (char*)msg, sizeof(netmessage_t), from );

	if( msg_size <= 0 )
		return false;

	msg_size -= NETMESSAGE_HDR;
#else
	sockaddr_in server;
	msg_size = recvfrom(m_socket, (char*)msg, MAX_MESSAGE_LENGTH+NETMESSAGE_HDR, 0, (struct sockaddr *)&server, &cl_len);
	if( msg_size < 0 )
	{
		int nErrorMsg = WSAGetLastError();

		// skip WSAEWOULDBLOCK, WSAENOTCONN and WSAECONNRESET to avoid receiving ICMP error
		if( nErrorMsg != WSAEWOULDBLOCK && nErrorMsg != WSAENOTCONN && nErrorMsg != WSAECONNRESET )
		{
			Msg("receive error (%s)\n", NETErrorString(nErrorMsg));
			return false;
		}

		return false;
	}

	if(msg_size == 0)
	{
		return false;
	}

#endif // USE_CUDP_SOCKET

	/*
	// not me?
	if(msg->clientid != m_nClientID)
	{
		Msg("invalid client id (%d vs %d)\n", msg->clientid, m_nClientID);
		msg_size = 0;
		return false;
	}
	*/

	// set to zero
	msg->clientid = NM_SERVER;

	return true;
}

bool CNetworkClient::InternalSend( netmessage_t* msg, int msg_size, short& msg_id, int flags )
{
	msg->clientid = m_nClientID;

#ifdef USE_CUDP_SOCKET
	int nCUDPFlags = 0;
	nCUDPFlags |= (flags & NSFLAG_GUARANTEED) ? (CDPSEND_GUARANTEED | CDPSEND_PRESERVE_ORDER) : 0;
	nCUDPFlags |= (flags & NSFLAG_IMMEDIATE) ? CDPSEND_IMMEDIATE : 0;

	int sended = m_pSocket->Send((char *)msg, msg_size + NETMESSAGE_HDR, &m_Server, msg_id, nCUDPFlags);

	sended -= NETMESSAGE_HDR;

	if( sended != msg_size )
	{
		MsgError("Bad sent message (%d versus %d)\n", msg_size, sended);
		return false;
	}
#else
	int cl_len = sizeof(struct sockaddr_in);

	int sended = sendto(m_socket, (char *)msg, msg_size, 0, (struct sockaddr *)&m_Server, cl_len);
	sended -= NETMESSAGE_HDR;

	if( sended != msg_size )
	{
		if( sended < 0 )
		{
			int nErrorMsg = WSAGetLastError();

			// skip WSAEWOULDBLOCK and WSAENOTCONN to avoid freeze
			if( nErrorMsg != WSAEWOULDBLOCK && nErrorMsg != WSAENOTCONN )
			{
				return false;
			}
		}

		MsgError("Bad sent message (%d versus %d)\n", msg_size, sended);
		return false;
	}
#endif // USE_CUDP_SOCKET

	return true;
}

int CNetworkClient::GetIncommingMessages()
{
#ifdef USE_CUDP_SOCKET
	return m_pSocket->GetReceivedMessageCount();
#else
	struct timeval stTimeOut;

	fd_set stReadFDS;

	FD_ZERO(&stReadFDS);

	// Timeout of one second
	stTimeOut.tv_sec = 0;
	stTimeOut.tv_usec = 20 * 1000;

	FD_SET(m_socket, &stReadFDS);

	return select( m_socket+1, &stReadFDS, NULL, NULL, &stTimeOut);
#endif
}

sockaddr_in CNetworkClient::GetSocketAddress()
{
#ifdef USE_CUDP_SOCKET
	return m_pSocket->GetAddress();
#else
	return m_Client;
#endif
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
