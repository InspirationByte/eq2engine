//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: TCP message transport for local tool links
//////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#endif

#include "core/core_common.h"
#include "NetTransportTCP.h"

#ifdef _WIN32
static constexpr int TCP_SEND_FLAGS = 0;
static constexpr int TCP_SHUTDOWN_SEND = 1;	// SD_SEND, winsock.h has no name for it
#else
static constexpr int TCP_SEND_FLAGS = MSG_NOSIGNAL;
static constexpr int TCP_SHUTDOWN_SEND = SHUT_WR;
#endif

// the receive loop stops after this much data in one Poll so one busy connection does not hold the others
static constexpr int TCP_RECV_BUFFER_SIZE = 16 * 1024;
static constexpr int TCP_MAX_RECV_PER_POLL = 1024 * 1024;

// little-endian, followed by the message
struct NetTCPFrameHeader
{
	uint32	size;
	uint16	lane;
	uint16	flags;		// reserved, 0
};
static_assert(sizeof(NetTCPFrameHeader) == 8, "NetTCPFrameHeader must be 8 bytes");

//-------------------------------------------------------------------------------------------------

static bool TCPSetNonBlocking(SOCKET socket)
{
#ifdef _WIN32
	u_long mode = 1;
	return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
	const int flags = fcntl(socket, F_GETFL, 0);
	return flags != -1 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

static void TCPSetNoDelay(SOCKET socket)
{
	// frames are already whole messages, merging them only adds latency
	int noDelay = 1;
	setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
}

static bool TCPWouldBlock(int error)
{
#ifdef _WIN32
	return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
	return error == EWOULDBLOCK || error == EAGAIN || error == EINPROGRESS;
#endif
}

static bool TCPIsReset(int error)
{
#ifdef _WIN32
	return error == WSAECONNRESET || error == WSAECONNABORTED;
#else
	return error == ECONNRESET || error == EPIPE;
#endif
}

// "host:port"; an empty host is any address for listening and the loopback address for connecting
static bool TCPParseAddress(const char* address, bool listen, sockaddr_in& out)
{
	memset(&out, 0, sizeof(out));
	out.sin_family = AF_INET;

	const char* portSep = strrchr(address, ':');
	if (!portSep)
		return false;

	const int port = atoi(portSep + 1);
	if (port < 0 || port > 65535)
		return false;

	out.sin_port = htons(static_cast<ushort>(port));

	const EqString host(address, static_cast<int>(portSep - address));
	if (host.Length() == 0)
	{
		out.sin_addr.s_addr = htonl(listen ? INADDR_ANY : INADDR_LOOPBACK);
		return true;
	}

	const unsigned long addr = inet_addr(host);
	if (addr != INADDR_NONE)
	{
		out.sin_addr.s_addr = addr;
		return true;
	}

	const hostent* hostEntry = gethostbyname(host);
	if (!hostEntry || hostEntry->h_addrtype != AF_INET || !hostEntry->h_addr_list[0])
		return false;

	memcpy(&out.sin_addr, hostEntry->h_addr_list[0], sizeof(out.sin_addr));
	return true;
}

static void TCPCloseSocket(SOCKET socket)
{
	if (socket == INVALID_SOCKET)
		return;

	shutdown(socket, TCP_SHUTDOWN_SEND);
	closesocket(socket);
}

//-------------------------------------------------------------------------------------------------

CNetTransportTCP::CNetTransportTCP(int maxMessageSize)
	: m_maxMessageSize(maxMessageSize)
{
	Networking::InitNetworking();
}

CNetTransportTCP::~CNetTransportTCP()
{
	for (auto it = m_connections.begin(); !it.atEnd(); ++it)
		DestroyConnection(*it);
	m_connections.clear(true);

	TCPCloseSocket(m_listenSocket);
	Networking::ShutdownNetworking();
}

bool CNetTransportTCP::Listen(const char* address)
{
	ASSERT_MSG(m_listenSocket == INVALID_SOCKET, "CNetTransportTCP is already listening");

	sockaddr_in addr;
	if (!TCPParseAddress(address, true, addr))
	{
		MsgError("CNetTransportTCP: bad listen address '%s'\n", address);
		return false;
	}

	const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		MsgError("CNetTransportTCP: socket() failed: %s\n", Networking::NETErrorString(sock_errno));
		return false;
	}

#ifndef _WIN32
	// on Windows the option lets another process take the port, a restarted server binds again anyway
	int reuseAddr = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
#endif

	if (bind(listenSocket, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR
		|| listen(listenSocket, SOMAXCONN) == SOCKET_ERROR
		|| !TCPSetNonBlocking(listenSocket))
	{
		MsgError("CNetTransportTCP: can't listen on '%s': %s\n", address, Networking::NETErrorString(sock_errno));
		closesocket(listenSocket);
		return false;
	}

	m_listenSocket = listenSocket;
	return true;
}

int CNetTransportTCP::GetListenPort() const
{
	if (m_listenSocket == INVALID_SOCKET)
		return 0;

	sockaddr_in addr;
	socklen_t addrLen = sizeof(addr);
	if (getsockname(m_listenSocket, reinterpret_cast<sockaddr*>(&addr), &addrLen) == SOCKET_ERROR)
		return 0;

	return ntohs(addr.sin_port);
}

NetConnectionId CNetTransportTCP::Connect(const char* address)
{
	sockaddr_in addr;
	if (!TCPParseAddress(address, false, addr))
	{
		MsgError("CNetTransportTCP: bad address '%s'\n", address);
		return NET_CONNECTION_NONE;
	}

	const SOCKET connSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connSocket == INVALID_SOCKET || !TCPSetNonBlocking(connSocket))
	{
		MsgError("CNetTransportTCP: socket() failed: %s\n", Networking::NETErrorString(sock_errno));
		if (connSocket != INVALID_SOCKET)
			closesocket(connSocket);
		return NET_CONNECTION_NONE;
	}

	TCPSetNoDelay(connSocket);

	// the result comes in Poll, a refused connection gives NET_DISCONNECT_CONNECT_FAILED there
	if (connect(connSocket, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
	{
		const int error = sock_errno;
		if (!TCPWouldBlock(error))
		{
			MsgError("CNetTransportTCP: can't connect to '%s': %s\n", address, Networking::NETErrorString(error));
			closesocket(connSocket);
			return NET_CONNECTION_NONE;
		}
	}

	return AddConnection(connSocket, true)->id;
}

void CNetTransportTCP::Close(NetConnectionId connId, int reason)
{
	auto it = m_connections.find(connId);
	if (it.atEnd())
		return;

	Connection* conn = *it;
	if (conn->closing)
		return;

	// the queued frames are still sent, Poll closes the socket after them
	conn->closing = true;
	if (!conn->connecting && !conn->sendFrame.data && !PickSendLane(conn))
	{
		m_connections.remove(it);
		DestroyConnection(conn);
	}
}

bool CNetTransportTCP::ConfigureLanes(NetConnectionId connId, ArrayCRef<NetLaneConfig> lanes)
{
	auto it = m_connections.find(connId);
	if (it.atEnd() || lanes.numElem() == 0)
		return false;

	Connection* conn = *it;
	for (const Lane* lane : conn->lanes)
	{
		if (lane->queue.getCount())
		{
			ASSERT_FAIL("CNetTransportTCP: ConfigureLanes after Send");
			return false;
		}
	}

	for (Lane* lane : conn->lanes)
		delete lane;
	conn->lanes.clear();

	for (const NetLaneConfig& config : lanes)
	{
		Lane* lane = PPNew Lane();
		lane->config = config;
		lane->config.weight = max(1, config.weight);
		conn->lanes.append(lane);
	}
	return true;
}

bool CNetTransportTCP::Send(NetConnectionId connId, int laneIdx, const void* data, int size, int flags)
{
	auto it = m_connections.find(connId);
	if (it.atEnd())
		return false;

	Connection* conn = *it;
	if (conn->closing || !conn->lanes.inRange(laneIdx) || size < 0 || size > m_maxMessageSize)
		return false;

	ASSERT_MSG(flags & NET_SEND_RELIABLE, "CNetTransportTCP has no unreliable messages");
	if (!(flags & NET_SEND_RELIABLE))
		return false;

	NetTCPFrameHeader header;
	header.size = size;
	header.lane = static_cast<uint16>(laneIdx);
	header.flags = 0;

	Frame frame;
	frame.size = sizeof(header) + size;
	frame.data = static_cast<ubyte*>(PPAlloc(frame.size));
	memcpy(frame.data, &header, sizeof(header));
	if (size)
		memcpy(frame.data + sizeof(header), data, size);

	Lane* lane = conn->lanes[laneIdx];
	lane->queue.append(frame);
	lane->queuedBytes += frame.size;

	return true;
}

int CNetTransportTCP::GetQueuedBytes(NetConnectionId connId, int laneIdx) const
{
	auto it = m_connections.find(connId);
	if (it.atEnd())
		return 0;

	const Connection* conn = *it;
	if (!conn->lanes.inRange(laneIdx))
		return 0;

	return conn->lanes[laneIdx]->queuedBytes;
}

void CNetTransportTCP::Poll(NetTransportEventList& events)
{
	while (m_listenSocket != INVALID_SOCKET)
	{
		sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		const SOCKET connSocket = accept(m_listenSocket, reinterpret_cast<sockaddr*>(&addr), &addrLen);
		if (connSocket == INVALID_SOCKET)
			break;

		if (!TCPSetNonBlocking(connSocket))
		{
			closesocket(connSocket);
			continue;
		}
		TCPSetNoDelay(connSocket);

		NetTransportEvent& event = events.append();
		event.type = NET_EVENT_CONNECTED;
		event.conn = AddConnection(connSocket, false)->id;
	}

	struct Drop
	{
		Connection*	conn;
		int			reason;		// 0 when closed locally
	};
	FixedArray<Drop, 32> drops;

	for (auto it = m_connections.begin(); !it.atEnd() && !drops.isFull(); ++it)
	{
		Connection* conn = *it;
		if (conn->connecting)
		{
			if (!UpdateConnecting(conn, events))
			{
				drops.append({ conn, conn->closing ? 0 : NET_DISCONNECT_CONNECT_FAILED });
				continue;
			}
			if (conn->connecting)
				continue;
		}

		if (!conn->closing)
		{
			const int reason = Receive(conn, events);
			if (reason)
			{
				drops.append({ conn, reason });
				continue;
			}
		}

		if (!Flush(conn))
		{
			drops.append({ conn, conn->closing ? 0 : NET_DISCONNECT_ERROR });
			continue;
		}

		if (conn->closing && !conn->sendFrame.data && !PickSendLane(conn))
			drops.append({ conn, 0 });
	}

	for (const Drop& drop : drops)
	{
		m_connections.remove(drop.conn->id);
		if (drop.reason)
		{
			NetTransportEvent& event = events.append();
			event.type = NET_EVENT_DISCONNECTED;
			event.conn = drop.conn->id;
			event.reason = drop.reason;
		}
		DestroyConnection(drop.conn);
	}
}

//-------------------------------------------------------------------------------------------------

CNetTransportTCP::Connection* CNetTransportTCP::AddConnection(SOCKET socket, bool connecting)
{
	Connection* conn = PPNew Connection();
	conn->id = ++m_idCounter;
	conn->socket = socket;
	conn->connecting = connecting;
	conn->lanes.append(PPNew Lane());

	m_connections.insert(conn->id, conn);
	return conn;
}

void CNetTransportTCP::DestroyConnection(Connection* conn)
{
	TCPCloseSocket(conn->socket);

	PPFree(conn->sendFrame.data);
	for (Lane* lane : conn->lanes)
	{
		for (const Frame& frame : lane->queue)
			PPFree(frame.data);
		delete lane;
	}
	delete conn;
}

// returns false when the connection failed; conn->connecting is cleared once it is established
bool CNetTransportTCP::UpdateConnecting(Connection* conn, NetTransportEventList& events)
{
	fd_set writeSet;
	fd_set exceptSet;
	FD_ZERO(&writeSet);
	FD_ZERO(&exceptSet);
	FD_SET(conn->socket, &writeSet);
	FD_SET(conn->socket, &exceptSet);

	timeval timeout{ 0, 0 };
	const int result = select(static_cast<int>(conn->socket) + 1, nullptr, &writeSet, &exceptSet, &timeout);
	if (result == 0)
		return true;

	if (result == SOCKET_ERROR || FD_ISSET(conn->socket, &exceptSet))
		return false;

	int error = 0;
	socklen_t errorLen = sizeof(error);
	if (getsockopt(conn->socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &errorLen) == SOCKET_ERROR || error != 0)
		return false;

	conn->connecting = false;
	if (!conn->closing)
	{
		NetTransportEvent& event = events.append();
		event.type = NET_EVENT_CONNECTED;
		event.conn = conn->id;
	}
	return true;
}

// returns the disconnect reason, 0 while the connection is fine
int CNetTransportTCP::Receive(Connection* conn, NetTransportEventList& events)
{
	ubyte buffer[TCP_RECV_BUFFER_SIZE];
	int totalReceived = 0;

	while (totalReceived < TCP_MAX_RECV_PER_POLL)
	{
		const int received = recv(conn->socket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
		if (received == 0)
			return NET_DISCONNECT_REMOTE;

		if (received == SOCKET_ERROR)
		{
			const int error = sock_errno;
			if (TCPWouldBlock(error))
				return 0;

			return TCPIsReset(error) ? NET_DISCONNECT_REMOTE : NET_DISCONNECT_ERROR;
		}

		totalReceived += received;

		int offset = 0;
		while (offset < received)
		{
			if (!conn->recvMessage)
			{
				const int headerBytes = min(static_cast<int>(sizeof(conn->recvHeader)) - conn->recvHeaderSize, received - offset);
				memcpy(conn->recvHeader + conn->recvHeaderSize, buffer + offset, headerBytes);
				conn->recvHeaderSize += headerBytes;
				offset += headerBytes;

				if (conn->recvHeaderSize < static_cast<int>(sizeof(conn->recvHeader)))
					break;

				NetTCPFrameHeader header;
				memcpy(&header, conn->recvHeader, sizeof(header));
				conn->recvHeaderSize = 0;

				if (header.size > static_cast<uint32>(m_maxMessageSize))
					return NET_DISCONNECT_PROTOCOL;

				conn->recvMessage = CRefPtr_new(CMemoryStream);
				conn->recvMessage->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, header.size);
				conn->recvRemaining = header.size;
				conn->recvLane = header.lane;
			}
			else
			{
				const int messageBytes = min(conn->recvRemaining, received - offset);
				conn->recvMessage->Write(buffer + offset, 1, messageBytes);
				conn->recvRemaining -= messageBytes;
				offset += messageBytes;
			}

			if (conn->recvRemaining == 0)
			{
				conn->recvMessage->Seek(0, FS_SEEK_SET);

				NetTransportEvent& event = events.append();
				event.type = NET_EVENT_MESSAGE;
				event.conn = conn->id;
				event.lane = conn->recvLane;
				event.message = std::move(conn->recvMessage);
				conn->recvMessage = nullptr;
			}
		}
	}

	return 0;
}

// writes the queued frames while the socket takes them; false on a socket error
bool CNetTransportTCP::Flush(Connection* conn)
{
	for (;;)
	{
		if (!conn->sendFrame.data)
		{
			Lane* lane = PickSendLane(conn);
			if (!lane)
				return true;

			conn->sendFrame = lane->queue.popFront();
			conn->sendOffset = 0;
			lane->sentBytes += conn->sendFrame.size;
			lane->queuedBytes -= conn->sendFrame.size;
		}

		const int toSend = conn->sendFrame.size - conn->sendOffset;
		const int sent = send(conn->socket, reinterpret_cast<const char*>(conn->sendFrame.data + conn->sendOffset), toSend, TCP_SEND_FLAGS);
		if (sent == SOCKET_ERROR)
			return TCPWouldBlock(sock_errno);

		conn->sendOffset += sent;
		if (conn->sendOffset < conn->sendFrame.size)
			return true;

		PPFree(conn->sendFrame.data);
		conn->sendFrame = Frame();
	}
}

// the lane of the highest priority with queued frames; equal priorities share by weight
CNetTransportTCP::Lane* CNetTransportTCP::PickSendLane(Connection* conn) const
{
	Lane* best = nullptr;
	for (Lane* lane : conn->lanes)
	{
		if (!lane->queue.getCount())
			continue;

		if (!best || lane->config.priority < best->config.priority)
		{
			best = lane;
			continue;
		}

		if (lane->config.priority == best->config.priority
			&& lane->sentBytes * best->config.weight < best->sentBytes * lane->config.weight)
		{
			best = lane;
		}
	}
	return best;
}
