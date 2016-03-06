//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network definitions for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef NET_DEFS_H
#define NET_DEFS_H

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

// returns text associated with error code
const char*	NETErrorString(int code);

bool		NETCompareAdr(const sockaddr_in& a, const sockaddr_in& b);

// initializes network subsystem
bool		InitNetworking();

// shutdowns network subsystem
void		ShutdownNetworking();

}; // namespace Networking

#endif // NET_DEFS_H
