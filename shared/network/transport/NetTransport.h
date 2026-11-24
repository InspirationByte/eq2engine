//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Message transport interface: connections, lanes, messages
//////////////////////////////////////////////////////////////////////////////////

#pragma once

using NetConnectionId = uint;
static constexpr NetConnectionId NET_CONNECTION_NONE = 0;

enum ENetSendFlags : int
{
	NET_SEND_RELIABLE	= (1 << 0),		// delivered once, in order within its lane
	NET_SEND_NO_DELAY	= (1 << 1),		// send now, do not wait to merge with other messages
};

enum ENetTransportCaps : int
{
	NET_TRANSPORT_UNRELIABLE	= (1 << 0),		// unreliable messages are supported
	NET_TRANSPORT_LANE_PRIORITY	= (1 << 1),		// lanes never wait for each other
};

enum ENetDisconnectReason : int
{
	NET_DISCONNECT_REMOTE = 1,			// the peer closed the connection
	NET_DISCONNECT_CONNECT_FAILED,		// the connection was never established
	NET_DISCONNECT_ERROR,				// socket or backend error
	NET_DISCONNECT_PROTOCOL,			// the peer sent broken data (bad frame, message too large)
};

struct NetLaneConfig
{
	int		priority{ 0 };		// lower first
	int		weight{ 1 };		// share among the lanes of one priority
};

enum ENetTransportEventType : int
{
	NET_EVENT_CONNECTED,
	NET_EVENT_DISCONNECTED,
	NET_EVENT_MESSAGE,
};

struct NetTransportEvent
{
	ENetTransportEventType	type{ NET_EVENT_MESSAGE };
	NetConnectionId			conn{ NET_CONNECTION_NONE };
	int						lane{ 0 };
	int						reason{ 0 };		// ENetDisconnectReason of NET_EVENT_DISCONNECTED
	CRefPtr<CMemoryStream>	message;			// NET_EVENT_MESSAGE, positioned at its start
};

using NetTransportEventList = Array<NetTransportEvent>;

class INetTransport : public RefCountedObject<INetTransport>
{
public:
	virtual int				GetCaps() const = 0;
	virtual int				GetMaxMessageSize() const = 0;

	// server: accepts connections on the address; client: connects to it.
	// Connect returns the id at once, NET_EVENT_CONNECTED or NET_EVENT_DISCONNECTED follows in Poll;
	// messages sent before that are queued
	virtual bool			Listen(const char* address) = 0;
	virtual NetConnectionId	Connect(const char* address) = 0;

	// sends the queued messages and closes; no event comes for a connection closed locally
	virtual void			Close(NetConnectionId conn, int reason = 0) = 0;

	// sets the send lanes of the connection before its first Send; one lane by default
	virtual bool			ConfigureLanes(NetConnectionId conn, ArrayCRef<NetLaneConfig> lanes) = 0;

	// copies the data; false when the connection is closed, the lane is not configured
	// or the message is too large
	virtual bool			Send(NetConnectionId conn, int lane, const void* data, int size, int flags) = 0;

	// bytes of the messages of the lane not sent yet; lets the users stream large data
	// without queueing all of it
	virtual int				GetQueuedBytes(NetConnectionId conn, int lane) const = 0;

	// runs the backend: accepts, receives, sends queued data; appends the events since the last call
	virtual void			Poll(NetTransportEventList& events) = 0;
};

using INetTransportPtr = CRefPtr<INetTransport>;
