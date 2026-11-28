//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: RPC endpoint: connections, call ids, futures, payload streams
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RPCProtocol.h"

struct RPCLanes
{
	int		calls{ 0 };		// call, reply and notify messages
	int		payload{ 1 };	// payload stream chunks
};

enum ERPCPayloadFlags : int
{
	RPC_PAYLOAD_LAST	= (1 << 0),
	RPC_PAYLOAD_CANCEL	= (1 << 1),
};

// little-endian, followed by the chunk data
struct RPCPayloadChunkHeader
{
	uint32	streamId;
	uint8	flags;
	uint8	reserved[3];
};

static constexpr int	RPC_PAYLOAD_CHUNK_SIZE = 64 * 1024;
static constexpr int64	RPC_DEFAULT_MAX_PAYLOAD = 256 * 1024 * 1024;

// the built-in protocol of every endpoint: Hello, Ping, CancelStream
static constexpr const char*	RPC_PROTOCOL_NAME = "rpc";
static constexpr int			RPC_PROTOCOL_VERSION = 1;

class RPCEndpoint : public WeakRefObject<RPCEndpoint>
{
	friend class RPCProtocol;
	friend class RPCCallContext;
	friend class RPCDeferredReply;
public:
	enum ERole : int
	{
		CLIENT,
		SERVER
	};

	using ConnectionCb = EqFunction<void(NetConnectionId conn, bool connected)>;

	// owned transport: Update polls it
	RPCEndpoint(ERole role, INetTransportPtr transport, RPCLanes lanes = {});

	// shared transport: the owner polls it, configures the lanes and passes the events of the RPC
	// lanes and the disconnects of the attached connections to HandleEvent
	RPCEndpoint(ERole role, INetTransport& sharedTransport, RPCLanes lanes);
	~RPCEndpoint();

	ERole				GetRole() const { return m_role; }

	bool				Listen(const char* address);	// SERVER, owned transport
	bool				Connect(const char* address);	// CLIENT, owned transport

	// closes the connection (owned transport) or forgets it (shared); the pending calls fail.
	// The client connection when conn is NET_CONNECTION_NONE
	void				Disconnect(NetConnectionId conn = NET_CONNECTION_NONE);

	void				AttachConnection(NetConnectionId conn);
	void				HandleEvent(const NetTransportEvent& event);

	// protocols are added before Listen / Connect, the Hello message lists them
	void				AddProtocol(RPCProtocol& protocol);
	void				RemoveProtocol(RPCProtocol& protocol);

	// called when a connection is established or closed
	void				SetConnectionCallback(ConnectionCb callback) { m_connectionCb = callback; }

	// a larger payload of the peer is refused with RPC_ERROR_PAYLOAD_TOO_LARGE
	void				SetMaxPayloadSize(int64 size) { m_maxPayloadSize = size; }

	// client: the connection is established
	bool				IsConnected() const;
	NetConnectionId		GetConnectionId() const { return m_clientConn; }
	int					GetConnectionCount() const { return m_connections.size(); }

	// the version of the protocol the peer told in Hello, 0 when it has none or Hello did not come yet
	int					GetPeerProtocolVersion(NetConnectionId conn, const char* protocol) const;

	// receives, runs the methods, resolves the futures, sends the payload chunks
	void				Update();

private:
	struct PendingCall
	{
		Promise<RPCResponse>	promise;
		RPCProtocol*			protocol{ nullptr };
	};

	struct InStream
	{
		RPCMessagePtr			message;	// the call or reply, null while only chunks came
		CRefPtr<CMemoryStream>	data;
		int64					size{ -1 };	// announced by the message
		bool					complete{ false };
	};

	struct OutStream
	{
		uint			id{ 0 };
		IFileStreamPtr	source;
		int64			offset{ 0 };
		int64			remaining{ 0 };
		uint			callId{ 0 };		// our call that has this payload, 0 for replies and notifies
	};

	struct Connection
	{
		NetConnectionId			id{ NET_CONNECTION_NONE };
		bool					connected{ false };
		uint					callIdCounter{ 0 };
		uint					streamIdCounter{ 0 };

		Map<uint, PendingCall>	pendingCalls{ PP_SL };
		Map<uint, bool>			deferredCalls{ PP_SL };
		Map<uint, InStream>		inStreams{ PP_SL };
		Map<uint, bool>			droppedStreams{ PP_SL };	// refused streams whose chunks still come
		List<OutStream>			outStreams{ PP_SL };
		Map<int, int>			peerProtocols{ PP_SL };
	};

	struct FailedCall
	{
		Promise<RPCResponse>	promise;
		int						code{ 0 };
		const char*				message{ nullptr };
	};
	using FailedCallList = Array<FailedCall>;

	void				Init();
	void				SetupLanes(NetConnectionId conn);

	Connection*			FindConnection(NetConnectionId conn) const;
	Connection*			AddConnection(NetConnectionId conn);
	void				RemoveConnection(NetConnectionId conn, bool notify);

	Future<RPCResponse>	Call(RPCProtocol& protocol, NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload);
	void				Notify(RPCProtocol& protocol, NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload);
	void				SendReply(NetConnectionId conn, uint callId, const KVSection* result, int errorCode, const char* errorMessage, IFileStreamPtr payload);
	void				SendHello(Connection& conn);

	void				AttachPayload(Connection& conn, KVSection& root, IFileStreamPtr payload, uint callId);
	bool				SendRoot(Connection& conn, const KVSection& root);
	void				SendPayloadChunks(Connection& conn, FailedCallList& failed);
	void				SendChunk(Connection& conn, uint streamId, int flags, const void* data, int size);
	void				CancelOutStream(Connection& conn, uint streamId, FailedCallList& failed);

	void				ReceiveMessage(NetConnectionId conn, CMemoryStream& message);
	void				ReceiveChunk(NetConnectionId conn, CMemoryStream& message);
	void				ProcessMessage(NetConnectionId conn, RPCMessagePtr message);
	void				DispatchCall(NetConnectionId conn, RPCMessagePtr message);
	void				DispatchReply(NetConnectionId conn, RPCMessagePtr message);
	void				RefusePayload(NetConnectionId conn, uint callId, uint streamId);

	static void			FailCalls(FailedCallList& failed);

	RPCProtocol			m_rpcProtocol{ RPC_PROTOCOL_NAME, RPC_PROTOCOL_VERSION };
	Map<int, RPCProtocol*>			m_protocols{ PP_SL };
	Map<NetConnectionId, Connection*>	m_connections{ PP_SL };
	NetTransportEventList			m_events{ PP_SL };

	INetTransportPtr	m_ownedTransport;
	INetTransport&		m_transport;
	RPCLanes			m_lanes;
	ERole				m_role{ CLIENT };
	NetConnectionId		m_clientConn{ NET_CONNECTION_NONE };
	int64				m_maxPayloadSize{ RPC_DEFAULT_MAX_PAYLOAD };
	ConnectionCb		m_connectionCb;
};
