//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: RPC endpoint: connections, call ids, futures, payload streams
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RPCEndpoint.h"

static_assert(sizeof(RPCPayloadChunkHeader) == 8, "RPCPayloadChunkHeader must be 8 bytes");

// payload chunks are read from the source while the lane has less than this queued,
// so a large payload is not copied into the transport all at once
static constexpr int RPC_PAYLOAD_QUEUE_LIMIT = 4 * RPC_PAYLOAD_CHUNK_SIZE;

// the id of a call or a reply, 0 for a notify
static uint RPCGetCallId(const KVSection& root)
{
	uint callId = 0;
	root.Get("id").GetValues(callId);
	return callId;
}

static void RPCCopySection(KVSection& root, const char* name, const KVSection& from)
{
	KVSection& section = root.CreateSection(name);
	from.CopyTo(section);
}

//-------------------------------------------------------------------------------------------------

RPCEndpoint::RPCEndpoint(ERole role, INetTransportPtr transport, RPCLanes lanes)
	: m_ownedTransport(transport)
	, m_transport(*transport)
	, m_lanes(lanes)
	, m_role(role)
{
	Init();
}

RPCEndpoint::RPCEndpoint(ERole role, INetTransport& sharedTransport, RPCLanes lanes)
	: m_transport(sharedTransport)
	, m_lanes(lanes)
	, m_role(role)
{
	Init();
}

RPCEndpoint::~RPCEndpoint()
{
	FailedCallList failed{ PP_SL };
	for (auto it = m_connections.begin(); !it.atEnd(); ++it)
	{
		Connection* conn = *it;
		for (auto callIt = conn->pendingCalls.begin(); !callIt.atEnd(); ++callIt)
			failed.append({ (*callIt).promise, RPC_ERROR_DISCONNECTED, "endpoint destroyed" });

		if (m_ownedTransport)
			m_transport.Close(conn->id);
		delete conn;
	}
	m_connections.clear(true);

	for (auto it = m_protocols.begin(); !it.atEnd(); ++it)
		(*it)->m_endpoint = nullptr;
	m_protocols.clear(true);

	FailCalls(failed);
}

void RPCEndpoint::Init()
{
	m_rpcProtocol
		.Method("Hello", [this](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr) {
			Connection* conn = FindConnection(ctx.GetConnectionId());
			if (!conn)
				return;

			conn->peerProtocols.clear();
			for (const KVSection& protocolSec : params.Get("protocols").Keys())
			{
				int version = 0;
				protocolSec.GetValues(version);
				conn->peerProtocols.insert(StringId(protocolSec.GetName()), version);

				auto it = m_protocols.find(StringId(protocolSec.GetName()));
				if (!it.atEnd() && (*it)->GetVersion() != version)
				{
					MsgWarning("RPC: protocol '%s' version %d, the peer has version %d\n",
						protocolSec.GetName(), (*it)->GetVersion(), version);
				}
			}
		})
		.Method("Ping", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr) {
			ctx.Reply(KVSection());
		})
		.Method("CancelStream", [this](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr) {
			Connection* conn = FindConnection(ctx.GetConnectionId());
			if (!conn)
				return;

			FailedCallList failed{ PP_SL };
			uint streamId = 0;
			params.Get("stream").GetValues(streamId);
			CancelOutStream(*conn, streamId, failed);
			FailCalls(failed);
		});

	AddProtocol(m_rpcProtocol);
}

void RPCEndpoint::SetupLanes(NetConnectionId conn)
{
	NetLaneConfig lanes[2];
	const int laneCount = max(m_lanes.calls, m_lanes.payload) + 1;
	if (laneCount != 2 || m_lanes.calls == m_lanes.payload)
	{
		ASSERT_FAIL("RPCEndpoint with an owned transport uses lanes 0 and 1");
		return;
	}

	// the calls go before the payload chunks, a large payload does not hold small calls back
	lanes[m_lanes.calls].priority = 0;
	lanes[m_lanes.payload].priority = 1;
	m_transport.ConfigureLanes(conn, ArrayCRef<NetLaneConfig>(lanes, laneCount));
}

bool RPCEndpoint::Listen(const char* address)
{
	ASSERT_MSG(m_role == SERVER, "RPCEndpoint::Listen is for the server");
	ASSERT_MSG(m_ownedTransport, "RPCEndpoint::Listen needs an owned transport");
	return m_transport.Listen(address);
}

bool RPCEndpoint::Connect(const char* address)
{
	ASSERT_MSG(m_role == CLIENT, "RPCEndpoint::Connect is for the client");
	ASSERT_MSG(m_ownedTransport, "RPCEndpoint::Connect needs an owned transport");

	if (m_clientConn != NET_CONNECTION_NONE)
		Disconnect(m_clientConn);

	const NetConnectionId connId = m_transport.Connect(address);
	if (connId == NET_CONNECTION_NONE)
		return false;

	SetupLanes(connId);

	// the transport queues the messages until the connection is established, Hello goes first
	m_clientConn = connId;
	SendHello(*AddConnection(connId));
	return true;
}

void RPCEndpoint::Disconnect(NetConnectionId connId)
{
	if (connId == NET_CONNECTION_NONE)
		connId = m_clientConn;

	if (!FindConnection(connId))
		return;

	if (m_ownedTransport)
		m_transport.Close(connId);

	RemoveConnection(connId, true);
}

void RPCEndpoint::AttachConnection(NetConnectionId connId)
{
	ASSERT_MSG(!m_ownedTransport, "RPCEndpoint::AttachConnection is for a shared transport");
	if (FindConnection(connId))
		return;

	Connection* conn = AddConnection(connId);
	conn->connected = true;
	if (m_role == CLIENT)
		m_clientConn = connId;

	SendHello(*conn);

	if (m_connectionCb)
		m_connectionCb(connId, true);
}

void RPCEndpoint::AddProtocol(RPCProtocol& protocol)
{
	ASSERT_MSG(!protocol.m_endpoint, "RPCProtocol '%s' is already added to an endpoint", protocol.GetName());
	if (protocol.m_endpoint)
		return;

	const int nameHash = StringId(protocol.GetName());
	ASSERT_MSG(!m_protocols.contains(nameHash), "RPCEndpoint already has protocol '%s'", protocol.GetName());
	if (m_protocols.contains(nameHash))
		return;

	m_protocols.insert(nameHash, &protocol);
	protocol.m_endpoint = this;
}

void RPCEndpoint::RemoveProtocol(RPCProtocol& protocol)
{
	if (protocol.m_endpoint != this)
		return;

	m_protocols.remove(StringId(protocol.GetName()));
	protocol.m_endpoint = nullptr;

	FailedCallList failed{ PP_SL };
	for (auto connIt = m_connections.begin(); !connIt.atEnd(); ++connIt)
	{
		Connection* conn = *connIt;
		for (auto it = conn->pendingCalls.begin(); !it.atEnd();)
		{
			if ((*it).protocol == &protocol)
			{
				failed.append({ (*it).promise, RPC_ERROR_CANCELLED, "protocol removed" });
				it = conn->pendingCalls.remove(it);
			}
			else
			{
				++it;
			}
		}
	}
	FailCalls(failed);
}

bool RPCEndpoint::IsConnected() const
{
	const Connection* conn = FindConnection(m_clientConn);
	return conn && conn->connected;
}

int RPCEndpoint::GetPeerProtocolVersion(NetConnectionId connId, const char* protocol) const
{
	const Connection* conn = FindConnection(connId);
	if (!conn)
		return 0;

	auto it = conn->peerProtocols.find(StringId(protocol));
	return it.atEnd() ? 0 : *it;
}

//-------------------------------------------------------------------------------------------------

void RPCEndpoint::Update()
{
	if (m_ownedTransport)
	{
		m_events.clear();
		m_transport.Poll(m_events);

		for (const NetTransportEvent& event : m_events)
			HandleEvent(event);
		m_events.clear();
	}

	FailedCallList failed{ PP_SL };
	for (auto it = m_connections.begin(); !it.atEnd(); ++it)
		SendPayloadChunks(**it, failed);
	FailCalls(failed);
}

void RPCEndpoint::HandleEvent(const NetTransportEvent& event)
{
	switch (event.type)
	{
		case NET_EVENT_CONNECTED:
		{
			if (!m_ownedTransport)
				break;

			if (m_role == CLIENT)
			{
				Connection* conn = FindConnection(event.conn);
				if (!conn)
					break;
				conn->connected = true;
			}
			else
			{
				SetupLanes(event.conn);
				Connection* conn = AddConnection(event.conn);
				conn->connected = true;
				SendHello(*conn);
			}

			if (m_connectionCb)
				m_connectionCb(event.conn, true);
			break;
		}
		case NET_EVENT_DISCONNECTED:
		{
			if (FindConnection(event.conn))
				RemoveConnection(event.conn, true);
			break;
		}
		case NET_EVENT_MESSAGE:
		{
			if (!event.message || !FindConnection(event.conn))
				break;

			if (event.lane == m_lanes.calls)
				ReceiveMessage(event.conn, *event.message);
			else if (event.lane == m_lanes.payload)
				ReceiveChunk(event.conn, *event.message);
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------------

RPCEndpoint::Connection* RPCEndpoint::FindConnection(NetConnectionId connId) const
{
	if (connId == NET_CONNECTION_NONE)
		return nullptr;

	auto it = m_connections.find(connId);
	return it.atEnd() ? nullptr : *it;
}

RPCEndpoint::Connection* RPCEndpoint::AddConnection(NetConnectionId connId)
{
	Connection* conn = PPNew Connection();
	conn->id = connId;
	m_connections.insert(connId, conn);
	return conn;
}

// the pending calls fail after the connection is removed, so their callbacks see the endpoint without it
void RPCEndpoint::RemoveConnection(NetConnectionId connId, bool notify)
{
	auto it = m_connections.find(connId);
	if (it.atEnd())
		return;

	Connection* conn = *it;
	m_connections.remove(it);

	if (m_clientConn == connId)
		m_clientConn = NET_CONNECTION_NONE;

	FailedCallList failed{ PP_SL };
	for (auto callIt = conn->pendingCalls.begin(); !callIt.atEnd(); ++callIt)
		failed.append({ (*callIt).promise, RPC_ERROR_DISCONNECTED, "disconnected" });

	const bool wasConnected = conn->connected;
	delete conn;

	if (notify && wasConnected && m_connectionCb)
		m_connectionCb(connId, false);

	FailCalls(failed);
}

void RPCEndpoint::FailCalls(FailedCallList& failed)
{
	for (const FailedCall& call : failed)
		call.promise.SetError(call.code, call.message);
	failed.clear();
}

//-------------------------------------------------------------------------------------------------

Future<RPCResponse> RPCEndpoint::Call(RPCProtocol& protocol, NetConnectionId connId, const char* method, const KVSection& params, IFileStreamPtr payload)
{
	if (connId == NET_CONNECTION_NONE)
	{
		ASSERT_MSG(m_role == CLIENT, "RPCProtocol::RemoteCall of a server needs a connection");
		connId = m_clientConn;
	}

	Connection* conn = FindConnection(connId);
	if (!conn)
		return Future<RPCResponse>::Failure(RPC_ERROR_NOT_CONNECTED, "not connected");

	const uint callId = ++conn->callIdCounter;

	KVSection root;
	root.SetKey("id", static_cast<int>(callId));
	root.SetKey("protocol", protocol.GetName());
	root.SetKey("method", method);
	RPCCopySection(root, "params", params);
	AttachPayload(*conn, root, payload, callId);

	if (!SendRoot(*conn, root))
	{
		// the peer never heard of the stream, it is only dropped here
		if (root.FindSection("payload") && conn->outStreams.getCount() && conn->outStreams.back().callId == callId)
			conn->outStreams.popBack();
		return Future<RPCResponse>::Failure(RPC_ERROR_SEND_FAILED, "message is too large");
	}

	PendingCall& call = conn->pendingCalls[callId];
	call.protocol = &protocol;
	return call.promise.CreateFuture();
}

void RPCEndpoint::Notify(RPCProtocol& protocol, NetConnectionId connId, const char* method, const KVSection& params, IFileStreamPtr payload)
{
	KVSection root;
	root.SetKey("protocol", protocol.GetName());
	root.SetKey("method", method);
	RPCCopySection(root, "params", params);

	if (connId == NET_CONNECTION_NONE && m_role == SERVER)
	{
		for (auto it = m_connections.begin(); !it.atEnd(); ++it)
		{
			Connection& conn = **it;

			// each connection reads the payload from the start, a shared source is read once per connection
			root.RemoveSectionByName("payload");
			AttachPayload(conn, root, payload, 0);
			SendRoot(conn, root);
		}
		return;
	}

	if (connId == NET_CONNECTION_NONE)
		connId = m_clientConn;

	Connection* conn = FindConnection(connId);
	if (!conn)
		return;

	AttachPayload(*conn, root, payload, 0);
	SendRoot(*conn, root);
}

void RPCEndpoint::SendReply(NetConnectionId connId, uint callId, const KVSection* result, int errorCode, const char* errorMessage, IFileStreamPtr payload)
{
	Connection* conn = FindConnection(connId);
	if (!conn)
		return;

	KVSection root;
	root.SetKey("id", static_cast<int>(callId));
	if (result)
	{
		RPCCopySection(root, "result", *result);
		AttachPayload(*conn, root, payload, 0);
	}
	else
	{
		KVSection& errorSec = root.CreateSection("error");
		errorSec.SetKey("code", errorCode);
		errorSec.SetKey("message", errorMessage ? errorMessage : "");
	}

	SendRoot(*conn, root);
}

void RPCEndpoint::SendHello(Connection& conn)
{
	KVSection params;
	KVSection& protocolsSec = params.CreateSection("protocols");
	for (auto it = m_protocols.begin(); !it.atEnd(); ++it)
		protocolsSec.SetKey((*it)->GetName(), (*it)->GetVersion());

	KVSection root;
	root.SetKey("protocol", m_rpcProtocol.GetName());
	root.SetKey("method", "Hello");
	RPCCopySection(root, "params", params);
	SendRoot(conn, root);
}

//-------------------------------------------------------------------------------------------------

void RPCEndpoint::AttachPayload(Connection& conn, KVSection& root, IFileStreamPtr payload, uint callId)
{
	if (!payload)
		return;

	const int64 size = payload->GetSize();
	ASSERT_MSG(size <= INT_MAX, "RPC payload must be smaller than 2 GB");

	// an empty payload has no stream and no chunks
	KVSection& payloadSec = root.CreateSection("payload");
	if (size <= 0)
	{
		payloadSec.SetKey("stream", 0);
		payloadSec.SetKey("size", 0);
		return;
	}

	OutStream& stream = conn.outStreams.append();
	stream.id = ++conn.streamIdCounter;
	stream.source = payload;
	stream.remaining = size;
	stream.callId = callId;

	payloadSec.SetKey("stream", static_cast<int>(stream.id));
	payloadSec.SetKey("size", static_cast<int>(size));
}

bool RPCEndpoint::SendRoot(Connection& conn, const KVSection& root)
{
	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 512);
	KeyValues::WriteBinary(&stream, root);

	if (stream.GetSize() > m_transport.GetMaxMessageSize())
	{
		MsgError("RPC: message of %d bytes is larger than the transport allows, send the data as payload\n", static_cast<int>(stream.GetSize()));
		return false;
	}

	return m_transport.Send(conn.id, m_lanes.calls, stream.GetBasePointer(), static_cast<int>(stream.GetSize()), NET_SEND_RELIABLE);
}

void RPCEndpoint::SendPayloadChunks(Connection& conn, FailedCallList& failed)
{
	const int chunkSize = min(RPC_PAYLOAD_CHUNK_SIZE, m_transport.GetMaxMessageSize() - static_cast<int>(sizeof(RPCPayloadChunkHeader)));
	ubyte* chunkData = nullptr;
	defer{
		if (chunkData)
			PPFree(chunkData);
	};

	while (conn.outStreams.getCount() && m_transport.GetQueuedBytes(conn.id, m_lanes.payload) < RPC_PAYLOAD_QUEUE_LIMIT)
	{
		OutStream& stream = conn.outStreams.front();

		// several streams may read one source (a notify to all connections), each keeps its offset
		stream.source->Seek(stream.offset, FS_SEEK_SET);

		const int size = static_cast<int>(min(static_cast<int64>(chunkSize), stream.remaining));
		if (!chunkData)
			chunkData = static_cast<ubyte*>(PPAlloc(chunkSize));

		if (stream.source->Read(chunkData, size, 1) != size)
		{
			MsgError("RPC: can't read payload stream '%s'\n", stream.source->GetName());
			CancelOutStream(conn, stream.id, failed);
			continue;
		}

		stream.offset += size;
		stream.remaining -= size;
		const bool last = stream.remaining <= 0;
		SendChunk(conn, stream.id, last ? RPC_PAYLOAD_LAST : 0, chunkData, size);

		if (last)
			conn.outStreams.popFront();
	}
}

void RPCEndpoint::SendChunk(Connection& conn, uint streamId, int flags, const void* data, int size)
{
	CMemoryStream chunk(PP_SL);
	chunk.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, sizeof(RPCPayloadChunkHeader) + size);

	RPCPayloadChunkHeader header;
	memset(&header, 0, sizeof(header));
	header.streamId = streamId;
	header.flags = static_cast<uint8>(flags);
	chunk.Write(&header, 1, sizeof(header));
	if (size)
		chunk.Write(data, 1, size);

	m_transport.Send(conn.id, m_lanes.payload, chunk.GetBasePointer(), static_cast<int>(chunk.GetSize()), NET_SEND_RELIABLE);
}

// drops the outgoing stream, tells the peer, and fails our call that had it as payload
void RPCEndpoint::CancelOutStream(Connection& conn, uint streamId, FailedCallList& failed)
{
	for (auto it = conn.outStreams.begin(); !it.atEnd(); ++it)
	{
		OutStream& stream = *it;
		if (stream.id != streamId)
			continue;

		const uint callId = stream.callId;
		conn.outStreams.remove(it);
		SendChunk(conn, streamId, RPC_PAYLOAD_CANCEL, nullptr, 0);

		auto callIt = conn.pendingCalls.find(callId);
		if (callId && !callIt.atEnd())
		{
			failed.append({ (*callIt).promise, RPC_ERROR_CANCELLED, "payload cancelled" });
			conn.pendingCalls.remove(callIt);
		}
		return;
	}
}

//-------------------------------------------------------------------------------------------------

void RPCEndpoint::ReceiveMessage(NetConnectionId connId, CMemoryStream& messageStream)
{
	RPCMessagePtr message = CRefPtr_new(RPCMessage);
	if (!KeyValues::ParseBinary(&messageStream, message->root))
	{
		MsgError("RPC: broken message from connection %u, disconnecting\n", connId);
		Disconnect(connId);
		return;
	}

	const KVSection* payloadSec = message->root.FindSection("payload");
	if (!payloadSec)
	{
		ProcessMessage(connId, message);
		return;
	}

	Connection* conn = FindConnection(connId);
	uint streamId = 0;
	int size = 0;
	payloadSec->Get("stream").GetValues(streamId);
	payloadSec->Get("size").GetValues(size);
	const uint callId = RPCGetCallId(message->root);

	if (size == 0)
	{
		message->payload = CRefPtr_new(CMemoryStream);
		message->payload->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
		ProcessMessage(connId, message);
		return;
	}

	InStream& stream = conn->inStreams[streamId];
	if (size < 0 || size > m_maxPayloadSize || (stream.data && stream.data->GetSize() > size))
	{
		// chunks still coming for the stream are dropped until its last or cancel chunk
		if (!stream.complete)
			conn->droppedStreams.insert(streamId, true);
		conn->inStreams.remove(streamId);

		// a reply with a payload we can't take fails its call, a call is refused
		const bool isReply = message->root.FindSection("result") != nullptr;
		if (isReply)
		{
			auto callIt = conn->pendingCalls.find(callId);
			if (!callIt.atEnd())
			{
				FailedCallList failed{ PP_SL };
				failed.append({ (*callIt).promise, RPC_ERROR_PAYLOAD_TOO_LARGE, "reply payload is too large" });
				conn->pendingCalls.remove(callIt);
				FailCalls(failed);
			}
			RefusePayload(connId, 0, streamId);
		}
		else
		{
			RefusePayload(connId, callId, streamId);
		}
		return;
	}

	stream.message = message;
	stream.size = size;
	if (!stream.data)
	{
		stream.data = CRefPtr_new(CMemoryStream);
		stream.data->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, size);
	}

	if (!stream.complete)
		return;

	// all chunks came before the message
	CRefPtr<CMemoryStream> data = stream.data;
	conn->inStreams.remove(streamId);
	if (data->GetSize() != size)
	{
		MsgError("RPC: payload stream %u of connection %u is shorter than announced, disconnecting\n", streamId, connId);
		Disconnect(connId);
		return;
	}

	data->Seek(0, FS_SEEK_SET);
	message->payload = data;
	ProcessMessage(connId, message);
}

void RPCEndpoint::ReceiveChunk(NetConnectionId connId, CMemoryStream& chunk)
{
	Connection* conn = FindConnection(connId);

	RPCPayloadChunkHeader header;
	if (chunk.GetSize() < static_cast<VSSize>(sizeof(header)))
	{
		MsgError("RPC: broken payload chunk from connection %u, disconnecting\n", connId);
		Disconnect(connId);
		return;
	}
	chunk.Read(&header, 1, sizeof(header));

	auto droppedIt = conn->droppedStreams.find(header.streamId);
	if (!droppedIt.atEnd())
	{
		if (header.flags & (RPC_PAYLOAD_LAST | RPC_PAYLOAD_CANCEL))
			conn->droppedStreams.remove(droppedIt);
		return;
	}

	auto it = conn->inStreams.find(header.streamId);
	if (header.flags & RPC_PAYLOAD_CANCEL)
	{
		if (it.atEnd())
			return;

		// the peer could not send its reply payload
		RPCMessagePtr message = (*it).message;
		conn->inStreams.remove(it);

		if (message && message->root.FindSection("result"))
		{
			auto callIt = conn->pendingCalls.find(RPCGetCallId(message->root));
			if (!callIt.atEnd())
			{
				FailedCallList failed{ PP_SL };
				failed.append({ (*callIt).promise, RPC_ERROR_CANCELLED, "reply payload cancelled" });
				conn->pendingCalls.remove(callIt);
				FailCalls(failed);
			}
		}
		return;
	}

	if (it.atEnd())
	{
		// a chunk may come before its message on backends with independent lanes
		InStream& newStream = conn->inStreams[header.streamId];
		newStream.data = CRefPtr_new(CMemoryStream);
		newStream.data->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
		it = conn->inStreams.find(header.streamId);
	}

	InStream& stream = *it;
	const int dataSize = static_cast<int>(chunk.GetSize() - chunk.Tell());
	const int64 limit = stream.size >= 0 ? stream.size : m_maxPayloadSize;
	if (stream.data->GetSize() + dataSize > limit)
	{
		MsgError("RPC: payload stream %u of connection %u is larger than announced, disconnecting\n", header.streamId, connId);
		Disconnect(connId);
		return;
	}

	stream.data->Write(chunk.GetCurrentPointer(), 1, dataSize);
	if (!(header.flags & RPC_PAYLOAD_LAST))
		return;

	stream.complete = true;
	if (!stream.message)
		return;

	if (stream.data->GetSize() != stream.size)
	{
		MsgError("RPC: payload stream %u of connection %u is shorter than announced, disconnecting\n", header.streamId, connId);
		Disconnect(connId);
		return;
	}

	RPCMessagePtr message = stream.message;
	message->payload = stream.data;
	conn->inStreams.remove(it);

	message->payload->Seek(0, FS_SEEK_SET);
	ProcessMessage(connId, message);
}

void RPCEndpoint::RefusePayload(NetConnectionId connId, uint callId, uint streamId)
{
	MsgError("RPC: payload stream %u of connection %u is too large, refused\n", streamId, connId);

	if (callId)
		SendReply(connId, callId, nullptr, RPC_ERROR_PAYLOAD_TOO_LARGE, "payload is too large", nullptr);

	KVSection params;
	params.SetKey("stream", static_cast<int>(streamId));
	Notify(m_rpcProtocol, connId, "CancelStream", params, nullptr);
}

void RPCEndpoint::ProcessMessage(NetConnectionId connId, RPCMessagePtr message)
{
	const KVSection& root = message->root;
	if (root.FindSection("result") || root.FindSection("error"))
		DispatchReply(connId, message);
	else
		DispatchCall(connId, message);
}

void RPCEndpoint::DispatchReply(NetConnectionId connId, RPCMessagePtr message)
{
	Connection* conn = FindConnection(connId);
	const uint callId = RPCGetCallId(message->root);

	auto it = conn->pendingCalls.find(callId);
	if (it.atEnd())
		return;

	const Promise<RPCResponse> promise = (*it).promise;
	conn->pendingCalls.remove(it);

	const KVSection* errorSec = message->root.FindSection("error");
	if (errorSec)
	{
		int code = 0;
		const char* errorMessage = "";
		errorSec->Get("code").GetValues(code);
		errorSec->Get("message").GetValues(errorMessage);
		promise.SetError(code, errorMessage);
		return;
	}

	RPCResponse response;
	response.message = message;
	promise.SetResult(std::move(response));
}

void RPCEndpoint::DispatchCall(NetConnectionId connId, RPCMessagePtr message)
{
	const KVSection& root = message->root;
	const uint callId = RPCGetCallId(root);
	const char* protocolName = "";
	const char* methodName = "";
	root.Get("protocol").GetValues(protocolName);
	root.Get("method").GetValues(methodName);

	const RPCMethodFunc* method = nullptr;
	auto it = m_protocols.find(StringId(protocolName));
	if (!it.atEnd())
		method = (*it)->FindMethod(methodName);

	if (!method)
	{
		if (callId)
		{
			const EqString errorMessage = EqString::Format("unknown method %s.%s", protocolName, methodName);
			SendReply(connId, callId, nullptr, RPC_ERROR_UNKNOWN_METHOD, errorMessage, nullptr);
		}
		else
		{
			DevMsg(DEVMSG_NETWORK, "RPC: notify of unknown method %s.%s\n", protocolName, methodName);
		}
		return;
	}

	RPCCallContext ctx(*this, connId, callId);
	const KVSection* params = root.FindSection("params");
	static const KVSection s_noParams;

	// the method is copied, it may remove its protocol while it runs
	const RPCMethodFunc func = *method;
	func(ctx, params ? *params : s_noParams, message->payload ? IFileStreamPtr(message->payload.Ptr()) : nullptr);

	if (callId && !ctx.m_answered)
		SendReply(connId, callId, &s_noParams, 0, nullptr, nullptr);
}
