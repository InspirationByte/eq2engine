//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: RPC protocols: named methods, remote calls and notifies
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "utils/KeyValues.h"
#include "network/transport/NetTransport.h"

class RPCEndpoint;

// errors of the RPC layer; methods reply with their own positive codes
enum ERPCError : int
{
	RPC_ERROR_UNKNOWN_METHOD	= -1,	// no such protocol or method on the peer
	RPC_ERROR_DISCONNECTED		= -2,	// the connection closed before the reply
	RPC_ERROR_CANCELLED			= -3,	// the payload stream was cancelled, or the protocol removed
	RPC_ERROR_PAYLOAD_TOO_LARGE	= -4,	// the peer refused the payload size
	RPC_ERROR_NOT_CONNECTED		= -5,	// no connection to send the call to
	RPC_ERROR_SEND_FAILED		= -6,	// the message is larger than the transport allows
};

// a received message: the KeyValues root and the payload stream when it has one
class RPCMessage : public RefCountedObject<RPCMessage>
{
public:
	KVSection				root;
	CRefPtr<CMemoryStream>	payload;
};
using RPCMessagePtr = CRefPtr<RPCMessage>;

struct RPCResponse
{
	// the "result" section of the reply, an empty section when it has none
	const KVSection&		GetResult() const;
	CRefPtr<CMemoryStream>	GetPayload() const { return message ? message->payload : nullptr; }

	RPCMessagePtr			message;
};

// a reply sent later, after the method returned; valid while its connection is open.
// Sent from the thread of the endpoint, once
class RPCDeferredReply
{
	friend class RPCCallContext;
public:
	RPCDeferredReply() = default;

	bool			IsValid() const;
	void			Reply(const KVSection& result, IFileStreamPtr payload = nullptr) const;
	void			ReplyError(int code, const char* message) const;

private:
	CWeakPtr<RPCEndpoint>	m_endpoint;
	NetConnectionId			m_conn{ NET_CONNECTION_NONE };
	uint					m_callId{ 0 };
};

// the call being handled by a method; a call the method neither replies nor defers
// gets an empty result when the method returns
class RPCCallContext
{
	friend class RPCEndpoint;
public:
	RPCEndpoint&		GetEndpoint() const { return m_endpoint; }
	NetConnectionId		GetConnectionId() const { return m_conn; }
	bool				IsNotify() const { return m_callId == 0; }

	void				Reply(const KVSection& result, IFileStreamPtr payload = nullptr);
	void				ReplyError(int code, const char* message);
	RPCDeferredReply	Defer();

private:
	RPCCallContext(RPCEndpoint& endpoint, NetConnectionId conn, uint callId);

	RPCEndpoint&		m_endpoint;
	NetConnectionId		m_conn{ NET_CONNECTION_NONE };
	uint				m_callId{ 0 };
	bool				m_answered{ false };
};

// payload is null when the message has none
using RPCMethodFunc = EqFunction<void(RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload)>;

// a named set of methods bound to one endpoint
class RPCProtocol
{
	friend class RPCEndpoint;
public:
	RPCProtocol(const char* name, int version = 1);
	~RPCProtocol();

	const char*			GetName() const { return m_name; }
	int					GetVersion() const { return m_version; }
	RPCEndpoint*		GetEndpoint() const { return m_endpoint; }

	RPCProtocol&		Method(const char* name, RPCMethodFunc func);

	// client: to its connection; server: the connection is required
	Future<RPCResponse>	RemoteCall(const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);
	Future<RPCResponse>	RemoteCall(NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);

	// client: to its connection; server: to all connections when conn is NET_CONNECTION_NONE
	void				RemoteNotify(const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);
	void				RemoteNotify(NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);

private:
	const RPCMethodFunc*	FindMethod(const char* name) const;

	Map<int, RPCMethodFunc>	m_methods{ PP_SL };
	EqString			m_name;
	int					m_version{ 1 };
	RPCEndpoint*		m_endpoint{ nullptr };
};
