//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: TCP message transport for local tool links
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "NetTransport.h"

static constexpr int NET_TCP_DEFAULT_MAX_MESSAGE = 1024 * 1024;

// a socket handle (the winsock SOCKET or a descriptor): the header does not include the system
// socket headers, whose macros (near, far, ...) break the code that includes it
using NetTCPSocket = intptr;
static constexpr NetTCPSocket NET_TCP_INVALID_SOCKET = -1;

class CNetTransportTCP : public INetTransport
{
public:
	CNetTransportTCP(int maxMessageSize = NET_TCP_DEFAULT_MAX_MESSAGE);
	~CNetTransportTCP();

	int				GetCaps() const override { return 0; }
	int				GetMaxMessageSize() const override { return m_maxMessageSize; }

	bool			Listen(const char* address) override;
	NetConnectionId	Connect(const char* address) override;
	void			Close(NetConnectionId conn, int reason = 0) override;

	bool			ConfigureLanes(NetConnectionId conn, ArrayCRef<NetLaneConfig> lanes) override;
	bool			Send(NetConnectionId conn, int lane, const void* data, int size, int flags) override;
	int				GetQueuedBytes(NetConnectionId conn, int lane) const override;

	void			Poll(NetTransportEventList& events) override;

	// the port the transport listens on, useful when it listens on port 0
	int				GetListenPort() const;

private:
	struct Frame
	{
		ubyte*	data{ nullptr };	// header and message
		int		size{ 0 };
	};

	struct Lane
	{
		NetLaneConfig	config;
		List<Frame>		queue{ PP_SL };
		int64			sentBytes{ 0 };
		int				queuedBytes{ 0 };
	};

	struct Connection
	{
		NetConnectionId	id{ NET_CONNECTION_NONE };
		NetTCPSocket	socket{ NET_TCP_INVALID_SOCKET };
		bool			connecting{ false };
		bool			closing{ false };

		Array<Lane*>	lanes{ PP_SL };

		Frame			sendFrame;
		int				sendOffset{ 0 };

		ubyte			recvHeader[8];
		int				recvHeaderSize{ 0 };
		int				recvLane{ 0 };
		CRefPtr<CMemoryStream>	recvMessage;
		int				recvRemaining{ 0 };
	};

	Connection*		AddConnection(NetTCPSocket socket, bool connecting);
	void			DestroyConnection(Connection* conn);

	bool			UpdateConnecting(Connection* conn, NetTransportEventList& events);
	int				Receive(Connection* conn, NetTransportEventList& events);
	bool			Flush(Connection* conn);
	Lane*			PickSendLane(Connection* conn) const;

	Map<NetConnectionId, Connection*>	m_connections{ PP_SL };
	NetTCPSocket	m_listenSocket{ NET_TCP_INVALID_SOCKET };
	NetConnectionId	m_idCounter{ NET_CONNECTION_NONE };
	int				m_maxMessageSize{ NET_TCP_DEFAULT_MAX_MESSAGE };
};
