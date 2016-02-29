//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network components for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETINTERFACES_H
#define NETINTERFACES_H

#include "net_defs.h"
#include "platform/Platform.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"

#define USE_CUDP_SOCKET

namespace Networking
{

enum ESendFlags
{
	NSFLAG_GUARANTEED	= (1 << 0),
	NSFLAG_IMMEDIATE	= (1 << 1),
};

struct netmessage_s
{
	ubyte	nfragments;
	ubyte	fragmentid;

	short	messageid;
	short	clientid;

	ushort	message_size;
	ushort	compressed_size;

	// data
	ubyte	message_bytes[MAX_MESSAGE_LENGTH];
};

ALIGNED_TYPE(netmessage_s,2) netmessage_t;

#define NETMESSAGE_HDR	12	// the network message header size

#define NM_SENDTOLAST	(-1)
#define NM_SENDTOALL	(-2)
#define NM_SERVER		(-3)

// client info for server
struct svclient_t
{
	int				client_id;
	sockaddr_in		client_addr;
};

//-------------------------------------------------------------------------------------------------------

class CUDPSocket;

// a shared networking interface
class INetworkInterface
{
public:
	virtual				~INetworkInterface() {}

	virtual	void		Shutdown() = 0;
	virtual int			GetIncommingMessages() = 0;

	// basic message sending
	bool				Receive( netmessage_t* msg, int &msg_size, sockaddr_in* from = NULL  );
	bool				Send( netmessage_t* msg, int msg_size, short& msg_id, int flags = 0 );


	void				Update( int timeMs );

	CUDPSocket*			GetSocket();

	virtual sockaddr_in	GetSocketAddress() = 0;

protected:

	// basic message sending
	virtual bool		InternalReceive( netmessage_t* msg, int &msg_size, sockaddr_in* from = NULL  ) = 0;
	virtual bool		InternalSend( netmessage_t* msg, int msg_size, short& msg_id, int flags = 0 ) = 0;

#ifdef USE_CUDP_SOCKET
	CUDPSocket*			m_pSocket;
#endif // USE_CUDP_SOCKET

	hostent*			m_hostinfo;

};

//-------------------------------------------------------------------------------------------------------

// Network server class
class CNetworkServer : public INetworkInterface
{
public:
	CNetworkServer();
	~CNetworkServer();

	bool				Init( int portNumber );
	void				Shutdown();

	int					GetIncommingMessages();
	bool				InternalReceive( netmessage_t* msg, int &msg_size, sockaddr_in* from = NULL );
	bool				InternalSend( netmessage_t* msg, int msg_size, short& msg_id, int flags = 0 );

	int					GetClientCount();

	int					AddClient( svclient_t* client_from_msg );
	svclient_t*			GetLastClient();

	svclient_t*			GetClientByAddr(sockaddr_in* addr);
	svclient_t*			GetClientById(int id);
	void				RemoveClientById(int id);

	sockaddr_in			GetSocketAddress();

private:
#ifndef USE_CUDP_SOCKET
	SOCKET				m_socket;
	sockaddr_in			m_Server;
#endif // #ifdef USE_CUDP_SOCKET


	svclient_t			m_lastclient;

	DkList<svclient_t*>	m_clients;
};

//-------------------------------------------------------------------------------------------------------

// network client
class CNetworkClient : public INetworkInterface
{
public:
	CNetworkClient();
	~CNetworkClient();

	bool				Connect( const char* pszAddress, int portNumber, int clientPort );
	void				Shutdown();

	bool				Heartbeat();

	int					GetIncommingMessages();
	bool				InternalReceive( netmessage_t* msg, int &msg_size, sockaddr_in* from = NULL );
	bool				InternalSend( netmessage_t* msg, int msg_size, short& msg_id, int flags = 0 );

	void				SetClientID(int nID);
	int					GetClientID();

	sockaddr_in			GetSocketAddress();

private:

#ifndef USE_CUDP_SOCKET
	SOCKET				m_socket;
	sockaddr_in			m_Client;
#endif

	sockaddr_in			m_Server;

	int				m_nClientID;
};

}; // namespace Networking

#endif // NETINTERFACES_H
