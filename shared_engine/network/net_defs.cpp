//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network definitions for Equilibrium Engine
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "net_defs.h"

namespace Networking
{

bool NETCompareAdr(const sockaddr_in& a, const sockaddr_in& b)
{
#ifdef _WIN32
	if (a.sin_addr.S_un.S_un_b.s_b1 == b.sin_addr.S_un.S_un_b.s_b1 &&
		a.sin_addr.S_un.S_un_b.s_b2 == b.sin_addr.S_un.S_un_b.s_b2 &&
		a.sin_addr.S_un.S_un_b.s_b3 == b.sin_addr.S_un.S_un_b.s_b3 &&
		a.sin_addr.S_un.S_un_b.s_b4 == b.sin_addr.S_un.S_un_b.s_b4 &&
		a.sin_port == b.sin_port)
		return true;
#else
	uint8* ipA = (uint8*)&a.sin_addr.s_addr;
	uint8* ipB = (uint8*)&b.sin_addr.s_addr;

	if (ipA[0] == ipB[3] &&
		ipA[1] == ipB[3] &&
		ipA[2] == ipB[3] &&
		ipA[3] == ipB[3] &&
		a.sin_port == b.sin_port)
		return true;
#endif

	return false;
}

const char* NETErrorString(int code)
{
	switch (code)
	{
#ifdef _WIN32
		case WSAEINTR: return "WSAEINTR";
		case WSAEBADF: return "WSAEBADF";
		case WSAEACCES: return "WSAEACCES";
		case WSAEDISCON: return "WSAEDISCON";
		case WSAEFAULT: return "WSAEFAULT";
		case WSAEINVAL: return "WSAEINVAL";
		case WSAEMFILE: return "WSAEMFILE";
		case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS: return "WSAEINPROGRESS";
		case WSAEALREADY: return "WSAEALREADY";
		case WSAENOTSOCK: return "WSAENOTSOCK";
		case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE: return "WSAEMSGSIZE";
		case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE: return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN: return "WSAENETDOWN";
		case WSAENETUNREACH: return "WSAENETUNREACH";
		case WSAENETRESET: return "WSAENETRESET";
		case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
		case WSAECONNRESET: return "WSAECONNRESET";
		case WSAENOBUFS: return "WSAENOBUFS";
		case WSAEISCONN: return "WSAEISCONN";
		case WSAENOTCONN: return "WSAENOTCONN";
		case WSAESHUTDOWN: return "WSAESHUTDOWN";
		case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
		case WSAETIMEDOUT: return "WSAETIMEDOUT";
		case WSAECONNREFUSED: return "WSAECONNREFUSED";
		case WSAELOOP: return "WSAELOOP";
		case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
		case WSASYSNOTREADY: return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED: return "WSANOTINITIALISED";
		case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN: return "WSATRY_AGAIN";
		case WSANO_RECOVERY: return "WSANO_RECOVERY";
		case WSANO_DATA: return "WSANO_DATA";
#else
		case EINTR: return "WSAEINTR";
		case EBADF: return "WSAEBADF";
		case EACCES: return "WSAEACCES";
		case EFAULT: return "WSAEFAULT";
		case EINVAL: return "WSAEINVAL";
		case EMFILE: return "WSAEMFILE";
		case EWOULDBLOCK: return "WSAEWOULDBLOCK";
		case EINPROGRESS: return "WSAEINPROGRESS";
		case EALREADY: return "WSAEALREADY";
		case ENOTSOCK: return "WSAENOTSOCK";
		case EDESTADDRREQ: return "WSAEDESTADDRREQ";
		case EMSGSIZE: return "WSAEMSGSIZE";
		case EPROTOTYPE: return "WSAEPROTOTYPE";
		case ENOPROTOOPT: return "WSAENOPROTOOPT";
		case EPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
		case ESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
		case EOPNOTSUPP: return "WSAEOPNOTSUPP";
		case EPFNOSUPPORT: return "WSAEPFNOSUPPORT";
		case EAFNOSUPPORT: return "WSAEAFNOSUPPORT";
		case EADDRINUSE: return "WSAEADDRINUSE";
		case EADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
		case ENETDOWN: return "WSAENETDOWN";
		case ENETUNREACH: return "WSAENETUNREACH";
		case ENETRESET: return "WSAENETRESET";
		case ECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
		case ECONNRESET: return "WSAECONNRESET";
		case ENOBUFS: return "WSAENOBUFS";
		case EISCONN: return "WSAEISCONN";
		case ENOTCONN: return "WSAENOTCONN";
		case ESHUTDOWN: return "WSAESHUTDOWN";
		case ETOOMANYREFS: return "WSAETOOMANYREFS";
		case ETIMEDOUT: return "WSAETIMEDOUT";
		case ECONNREFUSED: return "WSAECONNREFUSED";
		case ELOOP: return "WSAELOOP";
		case ENAMETOOLONG: return "WSAENAMETOOLONG";
		case EHOSTDOWN: return "WSAEHOSTDOWN";
		case HOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
		case TRY_AGAIN: return "WSATRY_AGAIN";
		case NO_RECOVERY: return "WSANO_RECOVERY";
#endif
		default: return "NO ERROR";
	}
}

#ifdef _WIN32

WSADATA	g_wsa;

bool InitNetworking()
{
	if( WSAStartup(MAKEWORD(2, 0), &g_wsa) != 0 )
	{
		MsgError("Could not open Windows connection.\n");
		return false;
	}

	return true;
}

void ShutdownNetworking()
{
	WSACleanup();
}

#else

bool InitNetworking()
{
	return true;
}

void ShutdownNetworking()
{
}

#endif // WIN32

}; // namespace Networking
