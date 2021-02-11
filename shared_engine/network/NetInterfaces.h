//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network components for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETINTERFACES_H
#define NETINTERFACES_H

#include "c_udp.h"
#include "core/dktypes.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"

namespace Networking
{

#define NM_SENDTOLAST	(-1)
#define NM_SENDTOALL	(-2)
#define NM_SERVER		(-3)

enum NetServerSendFlags
{
	NETSERVER_FLAG_REMOVECLIENT = (1 << 8),
};

struct netMessage_s
{
	struct hdr_t
	{
		ubyte	nfragments;
		ubyte	fragmentid;

		short	messageid;
		short	clientid;

		ushort	message_size;
		ushort	compressed_size;
	} header;

	// data
	ubyte	data[MAX_MESSAGE_LENGTH];
};

ALIGNED_TYPE(netMessage_s,2) netMessage_t;

#define NETMESSAGE_HDR	sizeof(netMessage_t::hdr_t)	// the network message header size

// client info for server
struct netPeer_t
{
	int				client_id;
	sockaddr_in		client_addr;
};

//-------------------------------------------------------------------------------------------------------

class CEqRDPSocket;

// a shared networking interface
class INetworkInterface
{
public:
	virtual				~INetworkInterface() {}

	virtual	void		Shutdown() = 0;

	bool				PreProcessRecievedMessage( ubyte* data, int size, netMessage_t& out );
	bool				Send( netMessage_t* msg, int msg_size, short& msg_id, int flags = 0 );

	void				Update( int timeMs, CDPRecvPipe_fn func, void* recvObj );

	CEqRDPSocket*		GetSocket() const {return m_pSocket;}
	sockaddr_in			GetAddress() const {return m_pSocket->GetAddress();}

	virtual bool		HasPeersToSend() const = 0;

protected:

	// basic message sending
	virtual bool		OnRecieved( netMessage_t* msg, const sockaddr_in& from ) = 0;
	virtual bool		InternalSend( netMessage_t* msg, int msg_size, short& msg_id, int flags = 0 ) = 0;

	CEqRDPSocket*		m_pSocket;

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

	bool				OnRecieved( netMessage_t* msg, const sockaddr_in& from  );
	bool				InternalSend( netMessage_t* msg, int msg_size, short& msg_id, int flags = 0 );

	int					GetClientCount();

	int					AddClient( netPeer_t* client_from_msg );
	netPeer_t*			GetLastClient();

	netPeer_t*			GetClientByAddr(sockaddr_in* addr);
	netPeer_t*			GetClientById(int id);
	void				RemoveClientById(int id);

	bool				HasPeersToSend() const;

private:
	netPeer_t			m_lastclient;

	DkList<netPeer_t*>	m_clients;
};

//-------------------------------------------------------------------------------------------------------

// network client
class CNetworkClient : public INetworkInterface
{
public:
	CNetworkClient();
	~CNetworkClient();

	bool			Connect( const char* pszAddress, int portNumber, int clientPort );
	void			Shutdown();

	bool			OnRecieved( netMessage_t* msg, const sockaddr_in& from );
	bool			InternalSend( netMessage_t* msg, int msg_size, short& msg_id, int flags = 0 );

	void			SetClientID(int nID);
	int				GetClientID();

	bool			HasPeersToSend() const {return true;}

private:
	sockaddr_in		m_Server;

	int				m_nClientID;
};

//-------------------------------------------------------------------------------------------------------

//					TODO: CNetworkPeerClient

}; // namespace Networking

#endif // NETINTERFACES_H
