//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network definitions for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32

#include <winsock.h>

#define sock_errno WSAGetLastError()
typedef int socklen_t;

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#define sock_errno errno

typedef int SOCKET;

#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#endif // _WIN32

namespace Networking
{

// equilibrium net protocol defaults
const int	DEFAULT_CLIENTPORT					= 12510;
const int	DEFAULT_SERVERPORT					= 12500;

const int	MAX_MESSAGE_LENGTH					= (16*1024);
const int	EQUILIBRIUM_NETPROTOCOL_VERSION		= 4;

enum ECDPSendFlags
{
	CDPSEND_GUARANTEED = (1 << 0),		// just simply send UDP message
	CDPSEND_IMMEDIATE = (1 << 1),		// send message immediately as it comes to buffer. It kills order preserving
	CDPSEND_IS_RESPONSE = (1 << 2),		// contains response header
};

enum EDeliveryStatus
{
	DELIVERY_IN_PROGRESS = -1,
	DELIVERY_SUCCESS = 0,
	DELIVERY_FAILED,
};

enum ERecvMessageKind
{
	RECV_MSG_DATA = 0,
	RECV_MSG_RESPONSE_DATA,
	RECV_MSG_STATUS,
};

// recv callback
// when it's @type is RECV_MSG_STATUS, @size is EDeliveryStatus
typedef void (*CDPRecvPipe_fn)(void* thisptr, ubyte* data, int size, const sockaddr_in& from, short msgId, ERecvMessageKind type);

// returns text associated with error code
const char*	NETErrorString(int code);

bool		NETCompareAdr(const sockaddr_in& a, const sockaddr_in& b);

// initializes network subsystem
bool		InitNetworking();

// shutdowns network subsystem
void		ShutdownNetworking();

}; // namespace Networking
