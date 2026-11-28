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

// returns text associated with error code
const char*	NETErrorString(int code);

// initializes network subsystem
bool		InitNetworking();

// shutdowns network subsystem
void		ShutdownNetworking();

}; // namespace Networking
